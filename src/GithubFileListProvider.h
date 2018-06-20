#pragma once

#include "FileListProvider.h"
#include "utils/WinInet.h"

class GitHubFileListProvider : public FileListProvider
{
public:
  GitHubFileListProvider ();
  void set_root_path (const std::wstring &root_path);

private:
  void update_file_list() override;
  void download_file(const std::string& filename) override;

private:
  std::wstring m_root_path;
  WinInet::WinInetHandle m_handle;
};
