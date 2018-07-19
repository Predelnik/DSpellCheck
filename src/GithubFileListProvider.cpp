#include "GithubFileListProvider.h"

#include "MainDef.h"

#include "ProgressData.h"
#include "Settings.h"
#include "UrlHelpers.h"
#include "json.hpp"
#include "utils/Win32Exception.h"
#include "utils/WinInet.h"
#include "utils/string_utils.h"
#include <fstream>
#include <iostream>
#include <string>

constexpr auto dic_ext = ".dic"sv, aff_ext = ".aff"sv;
constexpr auto dic_ext_w = L".dic"sv;

GitHubFileListProvider::GitHubFileListProvider(HWND parent, const Settings &settings)
    : m_get_file_list_task(parent), m_download_file_task(parent), m_settings(settings) {}

void GitHubFileListProvider::set_root_path(const std::wstring &root_path) { m_root_path = root_path; }

void GitHubFileListProvider::update_file_list() {
  constexpr auto default_timeout = 3000;
  std::wstring proxy_string;
  if (m_settings.use_proxy && m_settings.proxy_type == ProxyType::web_proxy)
    proxy_string = m_settings.proxy_host_name + L":" + std::to_wstring(m_settings.proxy_port);
  auto task = [root_path = m_root_path, proxy_string, default_timeout, proxy_name = m_settings.proxy_user_name, proxy_pwd = m_settings.proxy_password,
               is_anon = m_settings.proxy_is_anonymous](Concurrency::cancellation_token token) -> std::vector<FileDescription> {
    try {
      WinInet::GlobalHandle inet = WinInet::CreateGlobalHandle()
                                       .agent(static_plugin_name)
                                       .proxy_string(proxy_string.empty() ? L"" : proxy_string.c_str())
                                       .access_type(proxy_string.empty() ? INTERNET_OPEN_TYPE_PRECONFIG : INTERNET_OPEN_TYPE_PROXY);
      inet.set_connect_timeout(default_timeout);
      inet.set_receive_timeout(default_timeout);
      inet.set_send_timeout(default_timeout);
      const auto url_to_json = [&](const std::wstring &path) {
        WinInet::UrlHandle url_handle = WinInet::WinInetOpenUrl(inet, path.c_str());
        if (!is_anon) {
          url_handle.set_proxy_username(proxy_name);
          url_handle.set_proxy_password(proxy_pwd);
        }
        const auto text = WinInet::download_text_file(url_handle);
        return nlohmann::json::parse(text);
      };

      auto download_url = UrlHelpers::github_file_url_to_download_url(root_path);
      auto nodes = url_to_json(UrlHelpers::github_url_to_api_recursive_tree_url(root_path))["tree"];
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
    } catch (const Win32Exception &) {
      // TODO: return std::variant
      return {};
    }
  };
  m_get_file_list_task.do_deferred(task, [this](std::vector<FileDescription> files) { file_list_received(std::move(files)); });
}

void GitHubFileListProvider::download_dictionary(const std::wstring &aff_filepath, const std::wstring &target_path,
                                                 std::shared_ptr<ProgressData> progress_data) {
  m_download_file_task.do_deferred(
      [=](Concurrency::cancellation_token token) {
        auto do_download = [&](const std::wstring &path) {
          WinInet::GlobalHandle inet = WinInet::CreateGlobalHandle().agent(static_plugin_name);
          auto url_handle = WinInet::WinInetOpenUrl(inet, path.c_str());
          auto filename = path.substr(path.rfind(L'/'), std::wstring::npos);
          std::ofstream fs(target_path + filename, std::ios_base::out | std::ios_base::binary);
          WinInet::download_file(url_handle, fs, [&](int bytes_read, int total_bytes) {
            if (token.is_canceled())
              return false;

            auto percent = bytes_read * 100 / total_bytes;
            progress_data->set(percent, wstring_printf (rc_str(IDS_PD_OF_PD_BYTES_DOWNLOADED_PD).c_str(), bytes_read,
				total_bytes, percent));
            return true;
          });
        };
        do_download(aff_filepath);
        do_download(aff_filepath.substr(0, aff_filepath.length() - aff_ext.length()) + dic_ext_w.data());
        return true;
      },
      [this](bool) { file_downloaded(); });
}

void GitHubFileListProvider::cancel_download() { m_download_file_task.cancel(); }
