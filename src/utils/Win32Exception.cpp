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
    throw Win32Exception(get_win32_error_msg(err));
}
