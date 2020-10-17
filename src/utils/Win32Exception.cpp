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

#include "Win32Exception.h"
#include "ConnectionSettingsDialog.h"
#include <Wininet.h>
#include <cassert>
#include "string_utils.h"

namespace {

struct LocalFreeHelper {
  void operator()(void *toFree) const {
    ::LocalFree(reinterpret_cast<HLOCAL>(toFree));
  };
};

std::string get_win32_error_msg(DWORD error_code) {
  std::unique_ptr<wchar_t[], LocalFreeHelper> buff;
  LPWSTR buf_ptr = nullptr;
  DWORD buf_len = 0;
  if (error_code >= INTERNET_ERROR_BASE && error_code <= INTERNET_ERROR_LAST)
  {
    buf_len = ::FormatMessageW(
	FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, GetModuleHandle(L"wininet.dll"),
        error_code, 0, reinterpret_cast<LPWSTR>(&buf_ptr), 0, nullptr);
  }
  else {
      assert (false); // unsupported
  }
  buff.reset(buf_ptr);
  auto str = to_utf8_string({buff.get(), buf_len});
  return str;
}

} // namespace

void throw_if_error() {
  auto err = GetLastError();
  if (err != ERROR_SUCCESS)
    throw Win32Exception(std::to_string (err) + " - " + get_win32_error_msg(err));
}
