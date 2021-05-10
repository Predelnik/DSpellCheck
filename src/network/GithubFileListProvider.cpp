// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "GithubFileListProvider.h"

#include "json.hpp"
#include "UrlHelpers.h"
#include "common/overload.h"
#include "common/ProgressData.h"
#include "common/string_utils.h"
#include "common/Win32Exception.h"
#include "common/WinApi.h"
#include "common/WinInet.h"
#include "plugin/Constants.h"
#include "plugin/Settings.h"

#include <fstream>
#include <iostream>
#include <string>
#include <variant>

constexpr auto dic_ext = ".dic"sv, aff_ext = ".aff"sv;
constexpr auto dic_ext_w = L".dic"sv;

GitHubFileListProvider::GitHubFileListProvider(HWND parent, const Settings &settings)
  : m_get_file_list_task(parent), m_download_file_task(parent), m_settings(settings) {
}

void GitHubFileListProvider::set_root_path(const std::wstring &root_path) { m_root_path = root_path; }

void GitHubFileListProvider::update_file_list() {
  constexpr auto default_timeout = 3000;
  std::wstring proxy_string;
  if (m_settings.data.use_proxy && m_settings.data.proxy_type == ProxyType::web_proxy)
    proxy_string = m_settings.data.proxy_host_name + L":" + std::to_wstring(m_settings.data.proxy_port);
  using task_result_t = std::variant<std::monostate, std::vector<FileDescription>, std::string /*error*/>;
  // TODO: possibly separate settings required for proxy connection to a struct to pass them instead of whole settings
  auto task = [root_path = m_root_path, proxy_string, default_timeout, settings_data = m_settings.data
      ](Concurrency::cancellation_token token) -> task_result_t {
    try {
      auto inet = create_global_handle(settings_data);
      const auto url_to_json = [&](const std::wstring &path) {
        auto url_handle = create_url_handle(settings_data, inet, path.c_str());
        const auto text = WinInet::download_text_file(url_handle);
        return nlohmann::json::parse(text);
      };

      auto rate_limit_data = url_to_json(L"https://api.github.com/rate_limit");
      auto core_limit_data = rate_limit_data["resources"]["core"];
      if (core_limit_data["remaining"] == 0) {
        auto reset_secs = core_limit_data["reset"].get<time_t>();
        std::wstringstream wss;
        wss << std::put_time(std::localtime(&reset_secs), L"%H:%M");
        return to_string(wstring_printf(rc_str(IDS_GITHUB_API_RATE_LIMIT_EXCEEDED_PD_PS).c_str(), core_limit_data["limit"].get<int>(),
                                        wss.str().c_str()));
      }
      nlohmann::json nodes;
      std::wstring download_url;
      for (auto branch_name : {L"master", L"main"}) {
        download_url = UrlHelpers::github_file_url_to_download_url(root_path, branch_name);
        nodes = url_to_json(UrlHelpers::github_url_to_api_recursive_tree_url(root_path, branch_name))["tree"];
        if (!nodes.empty())
          break;
      }
      std::vector<FileDescription> result;
      for (auto &node : nodes) {
        if (token.is_canceled())
          return {};
        if (!node.is_object() || node["type"] != "blob")
          continue;
        std::string path = node["path"];
        if (!ends_with(path, aff_ext))
          continue;

        auto dic_name = path.substr(0, path.length() - 4) + dic_ext.data();
        if (std::find_if(nodes.begin(), nodes.end(), [&](const auto &f) { return f["path"] == dic_name; }) == nodes.end())
          continue;
        auto start_pos = path.rfind('/') + 1;
        result.push_back({utf8_to_wstring(path.substr(start_pos, path.length() - start_pos - aff_ext.length()).c_str()),
                          download_url + utf8_to_wstring(node["path"].get<std::string>().c_str())});
      }
      return result;
    } catch (const std::exception &exception) {
      return exception.what();
    }
  };
  m_get_file_list_task.do_deferred(task, [this](task_result_t &&result) {
    std::visit(overload([](std::monostate) {
                        }, [this](std::vector<FileDescription> &&list) { file_list_received(std::move(list)); },
                        [this](std::string &&str) { error_happened(str); }),
               std::move(result));
  });
}

WinInet::GlobalHandle GitHubFileListProvider::create_global_handle(const Settings::Data &settings_data) {
  constexpr auto default_timeout = 3000;
  std::wstring proxy_string;
  if (settings_data.use_proxy && settings_data.proxy_type == ProxyType::web_proxy)
    proxy_string = settings_data.proxy_host_name + L":" + std::to_wstring(settings_data.proxy_port);

  WinInet::GlobalHandle inet = WinInet::CreateGlobalHandle()
                               .agent(static_plugin_name)
                               .proxy_string(proxy_string.empty() ? L"" : proxy_string.c_str())
                               .access_type(proxy_string.empty() ? INTERNET_OPEN_TYPE_PRECONFIG : INTERNET_OPEN_TYPE_PROXY);
  inet.set_connect_timeout(default_timeout);
  inet.set_receive_timeout(default_timeout);
  inet.set_send_timeout(default_timeout);
  return inet;
}

WinInet::UrlHandle GitHubFileListProvider::create_url_handle(const Settings::Data &settings_data, const WinInet::GlobalHandle &global_handle,
                                                             const wchar_t *url) {
  auto proxy_name = settings_data.proxy_user_name;
  auto proxy_pwd = settings_data.proxy_password;
  auto is_anon = settings_data.proxy_is_anonymous;
  WinInet::UrlHandle url_handle = WinInet::WinInetOpenUrl(global_handle, url);
  if (!is_anon) {
    url_handle.set_proxy_username(proxy_name);
    url_handle.set_proxy_password(proxy_pwd);
  }
  return url_handle;
}

std::optional<std::string> GitHubFileListProvider::download_dictionary_impl(const Concurrency::cancellation_token &token, const std::wstring &aff_filepath,
                                                                            const std::wstring &target_path,
                                                                            std::shared_ptr<ProgressData> progress_data,
                                                                            std::vector<std::wstring> &downloaded_file_names, const Settings::Data &
                                                                            settings_data, bool &cancelled) {
  auto do_download = [&](const std::wstring &path, bool &cancelled) -> std::optional<std::string> {

    auto inet = create_global_handle(settings_data);
    auto url_handle = create_url_handle(settings_data, inet, path.c_str());
    auto filename = path.substr(path.rfind(L'/'), std::wstring::npos);
    auto local_file_path = target_path + filename;
    downloaded_file_names.push_back(local_file_path);
    std::ofstream fs(local_file_path, std::ios_base::out | std::ios_base::binary);
    if (!fs.is_open())
      return "Failed to save a file locally";
    if (!WinInet::download_file(url_handle, fs, [&](int bytes_read, int total_bytes) {
      if (token.is_canceled())
        return false;

      auto percent = bytes_read * 100 / total_bytes;
      progress_data->set(percent, wstring_printf(rc_str(IDS_PD_OF_PD_BYTES_DOWNLOADED_PD).c_str(), bytes_read, total_bytes, percent));
      return true;
    })) {
      cancelled = true;
      return "Cancelled";
    }

    return {};
  };
  if (auto ret = do_download(aff_filepath, cancelled))
    return ret;

  if (auto ret = do_download(aff_filepath.substr(0, aff_filepath.length() - aff_ext.length()) + dic_ext_w.data(), cancelled))
    return ret;

  return {};
}

namespace {
class DownloadResult {
public:
  std::optional<std::string> mb_error_description;
  bool was_cancelled;
};
}

void GitHubFileListProvider::download_dictionary(const std::wstring &aff_filepath, const std::wstring &target_path,
                                                 std::shared_ptr<ProgressData> progress_data) {
  m_download_file_task.do_deferred(
      [=, settings_data = m_settings.data](Concurrency::cancellation_token token) -> DownloadResult {
        std::vector<std::wstring> downloaded_filenames;
        bool cancelled = false;
        try {
          auto ret = download_dictionary_impl(token, aff_filepath, target_path, progress_data, downloaded_filenames, settings_data, cancelled);
          if (ret) {
            for (auto &file_name : downloaded_filenames)
              WinApi::delete_file(file_name.c_str());
          }
          return {ret, cancelled};
        } catch (const std::exception &err) {
          for (auto &file_name : downloaded_filenames)
            WinApi::delete_file(file_name.c_str());
          return {err.what(), false};
        }
      },
      [this](DownloadResult result) { file_downloaded(result.mb_error_description, result.was_cancelled); });
}

void GitHubFileListProvider::cancel_download() { m_download_file_task.cancel(); }
