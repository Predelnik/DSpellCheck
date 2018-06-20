#include "WinInet.h"
#include "Win32Exception.hpp"

namespace WinInet {
CreateWinInetHandle::operator WinInetHandle() const {
  return {m_agent, m_access_type, m_proxy_string};
}

WinInetUrlHandle::WinInetUrlHandle(HINTERNET h_internet, const wchar_t *url,
                                   std::wstring_view headers, DWORD flags,
                                   void *context) {
  m_handle = InternetOpenUrl(h_internet, url, headers.data(),
                             static_cast<DWORD>(headers.length()), flags,
                             reinterpret_cast<uint64_t>(context));
}

void WinInetUrlHandle::set_proxy_username(std::wstring_view username) {
  InternetSetOption(m_handle, INTERNET_OPTION_PROXY_USERNAME,
                    const_cast<wchar_t *>(username.data()),
                    static_cast<DWORD>(username.length()) + 1);
  proxy_settings_changed();
}

void WinInetUrlHandle::set_proxy_password(std::wstring_view password) {
  InternetSetOption(m_handle, INTERNET_OPTION_PROXY_PASSWORD,
                    const_cast<wchar_t *>(password.data()),
                    static_cast<DWORD>(password.length()) + 1);
  proxy_settings_changed();
}

void WinInetHandle::set_connect_timeout(DWORD ms) {
  InternetSetOption(m_handle, INTERNET_OPTION_CONNECT_TIMEOUT, &ms,
                    sizeof(DWORD));
}

void WinInetHandle::set_send_timeout(DWORD ms) {
  InternetSetOption(m_handle, INTERNET_OPTION_SEND_TIMEOUT, &ms, sizeof(DWORD));
}

void WinInetHandle::set_receive_timeout(DWORD ms) {
  InternetSetOption(m_handle, INTERNET_OPTION_RECEIVE_TIMEOUT, &ms,
                    sizeof(DWORD));
}

WinInetUrlHandle::~WinInetUrlHandle() { InternetCloseHandle(m_handle); }

void WinInetUrlHandle::proxy_settings_changed() {
  InternetSetOption(m_handle, INTERNET_OPTION_PROXY_SETTINGS_CHANGED, nullptr,
                    0);
}

std::string download_text_file(const WinInetUrlHandle &handle)
{

}

WinInetHandle::WinInetHandle() {}

WinInetHandle::WinInetHandle(const wchar_t *agent, DWORD access_type,
                             const wchar_t *proxy_string,
                             const wchar_t *proxy_bypass, DWORD flags) {
  m_handle =
      InternetOpen(agent, access_type, proxy_string, proxy_bypass, flags);
  if (!m_handle)
    throw_last_error ();
}

WinInetHandle::~WinInetHandle() { InternetCloseHandle(m_handle); }
} // namespace WinInet
