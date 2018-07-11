#include "GithubFileListProvider.h"

#include "MainDef.h"

#include <iostream>

GitHubFileListProvider::GitHubFileListProvider() {
  m_handle = WinInet::CreateWinInetHandle().agent(static_plugin_name);
}

void GitHubFileListProvider::set_root_path(const std::wstring &root_path) {
  m_root_path = root_path;
}

void GitHubFileListProvider::update_file_list() {
  auto url_handle = WinInet::WinInetOpenUrl(m_handle, m_root_path.c_str());
  auto text = WinInet::download_text_file (url_handle);
  std::cout << text << '\n';
}

void GitHubFileListProvider::download_file(const std::string &/*filename*/) {}
