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

#pragma once

#include "FileListProvider.h"
#include "Settings.h"
#include "TaskWrapper.h"
#include "utils/WinInet.h"

class Settings;

class GitHubFileListProvider : public FileListProvider {
public:
  GitHubFileListProvider(HWND parent, const Settings &settings);
  void set_root_path(const std::wstring &root_path);
  void update_file_list() override;
  void download_dictionary(const std::wstring &aff_filepath, const std::wstring &target_path, std::shared_ptr<ProgressData> progress_data) override;
  void cancel_download();

private:
  static WinInet::GlobalHandle create_global_handle(const Settings::Data& settings_data);
  static WinInet::UrlHandle create_url_handle(const Settings::Data& settings_data, const WinInet::GlobalHandle& global_handle, const wchar_t* url);
  static std::optional<std::string> download_dictionary_impl(const Concurrency::cancellation_token &token, const std::wstring &aff_filepath,
                                                             const std::wstring &target_path, std::shared_ptr<ProgressData> progress_data,
                                                             std::vector<std::wstring> &downloaded_file_names, const
                                                             Settings::Data &settings_data, bool& cancelled);

private:
  std::wstring m_root_path;
  TaskWrapper m_get_file_list_task;
  TaskWrapper m_download_file_task;
  const Settings &m_settings;
};
