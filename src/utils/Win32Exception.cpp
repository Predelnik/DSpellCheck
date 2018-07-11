#include "Win32Exception.h"
#include "ConnectionSettingsDialog.h"

namespace {

struct LocalFreeHelper {
  void operator()(void *toFree) const {
    ::LocalFree(reinterpret_cast<HLOCAL>(toFree));
  };
};

std::string get_win32_error_msg(DWORD errorCode) {
  std::unique_ptr<wchar_t[], LocalFreeHelper> buff;
  LPWSTR buffPtr;
  DWORD bufferLength = ::FormatMessageW(
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr,
      errorCode, 0, reinterpret_cast<LPWSTR>(&buffPtr), 0, nullptr);
  buff.reset(buffPtr);
  return to_utf8_string({buff.get(), bufferLength});
}

} // namespace

void throw_if_error() {
  auto err = GetLastError();
  if (err != ERROR_SUCCESS)
    throw Win32Exception(get_win32_error_msg(err));
}
