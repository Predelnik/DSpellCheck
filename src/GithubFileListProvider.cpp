#include "GithubFileListProvider.h"

#include "MainDef.h"

#include "json.hpp"
#include "utils/string_utils.h"
#include <iostream>
#include <string>
#include "utils/WinInet.h"
#include <fstream>
#include "UrlHelpers.h"

GitHubFileListProvider::GitHubFileListProvider(HWND parent)
  : m_get_file_list_task(parent),
    m_download_file_task(parent) {
}

void GitHubFileListProvider::set_root_path(const std::wstring &root_path) {
  m_root_path = root_path;
}

void GitHubFileListProvider::update_file_list() {
  m_get_file_list_task.do_deferred(
                                   [root_path = m_root_path](Concurrency::cancellation_token token) -> std::vector<FileDescription>
                                   {
                                     WinInet::WinInetHandle inet =
                                       WinInet::CreateWinInetHandle().agent(static_plugin_name);
                                     const auto url_to_json = [&](const std::wstring &path)
                                     {
                                       auto url_handle = WinInet::WinInetOpenUrl(inet, path.c_str());
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
                                       if (!ends_with(path, ".aff"))
                                         continue;

                                         auto dic_name = path.substr(0, path.length() - 4) + ".dic";
                                         if (std::find_if(nodes.begin(), nodes.end(), [&](const auto &f) { return f["path"] == dic_name; }) == nodes.end())
                                           continue;
                                         auto start_pos = path.rfind('/') + 1;
                                         result.push_back({utf8_to_wstring(path.substr(start_pos, path.length() - start_pos - 4).c_str()),
                                                            download_url + utf8_to_wstring(node["path"].get<std::string>().c_str())
                                                          });
                                     }
                                     return result;
                                   },
                                   [this](std::vector<FileDescription> files)
                                   {
                                     file_list_received(std::move(files));
                                   });
}

void GitHubFileListProvider::download_dictionary(const std::wstring &aff_filepath, const std::wstring &target_path) {
  m_download_file_task.do_deferred(
                                   [=](Concurrency::cancellation_token token)
                                   {
                                     auto do_download = [&](const std::wstring &path)
                                     {
                                       WinInet::WinInetHandle inet =
                                         WinInet::CreateWinInetHandle().agent(static_plugin_name);
                                       auto url_handle = WinInet::WinInetOpenUrl(inet, path.c_str());
                                       auto filename = path.substr(path.rfind(L'/'), std::wstring::npos);
                                       std::ofstream fs(target_path + filename, std::ios_base::out | std::ios_base::binary);
                                       WinInet::download_file(url_handle, fs);
                                     };
                                     do_download(aff_filepath);
                                     do_download(aff_filepath.substr(0, aff_filepath.length() - 4) + L".dic");
                                     return true;
                                   }, [this](bool)
                                   {
                                     file_downloaded();
                                   }
                                  );
}
