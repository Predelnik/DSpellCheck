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

#include "lsignal.h"

class ProgressData;

class FileDescription
{
public:
  std::wstring title;
  std::wstring path;
  bool was_alias_applied = false;
};

enum class FileListProviderDownloadErrorType
{
  none,
  canceled,
  file_is_not_writeable,
  was_not_able_to_download,
};

class FileListProvider
{
  virtual void update_file_list () = 0; // asynchronous call, caller should wait for file_list_received
  virtual void download_dictionary (const std::wstring& aff_filepath, const std::wstring& target_path, std::shared_ptr<ProgressData> progress_data) = 0;
public:
  virtual ~FileListProvider() = default;
  lsignal::signal<void (std::vector<FileDescription>)> file_list_received;
  lsignal::signal<void(const std::optional<std::string> &error_description, bool was_cancelled)> file_downloaded;
  lsignal::signal<void (const std::string &description)> error_happened;
};

