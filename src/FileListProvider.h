#pragma once

#include "lsignal.h"

class ProgressData;

class FileDescription
{
public:
  std::wstring title;
  std::wstring path;
};

class FileListProvider
{
  virtual void update_file_list () = 0; // asynchronous call, caller should wait for file_list_received
  virtual void download_dictionary (const std::wstring& aff_filepath, const std::wstring& target_path, std::shared_ptr<ProgressData> progress_data) = 0;
public:
  virtual ~FileListProvider() = default;
  lsignal::signal<void (std::vector<FileDescription>)> file_list_received;
  lsignal::signal<void()> file_downloaded;
};

