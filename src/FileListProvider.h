#pragma once

#include "lsignal.h"

class FileListProvider
{
  virtual void update_file_list () = 0; // asynchronous call, caller should wait for on_file_list_received
  virtual void download_file (const std::string &filename) = 0;
public:
  lsignal::signal<std::vector<std::string> ()> on_file_list_received;
};

