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
#include "WinInet.h"
#include <sstream>

namespace WinInet {
CreateGlobalHandle::operator GlobalHandle() const { return {m_agent, m_access_type, m_proxy_string}; }

UrlHandle::UrlHandle(HINTERNET h_internet, const wchar_t *url, std::wstring_view headers, DWORD flags, void *context) {
  m_handle = InternetOpenUrl(h_internet, url, headers.data(), static_cast<DWORD>(headers.length()), flags, reinterpret_cast<uint64_t>(context));
  if (!m_handle.get())
    throw_if_error();
}

void UrlHandle::set_proxy_username(std::wstring_view username) {
  InternetSetOption(m_handle.get(), INTERNET_OPTION_PROXY_USERNAME, const_cast<wchar_t *>(username.data()), static_cast<DWORD>(username.length()) + 1);
  proxy_settings_changed();
}

void UrlHandle::set_proxy_password(std::wstring_view password) {
  InternetSetOption(m_handle.get(), INTERNET_OPTION_PROXY_PASSWORD, const_cast<wchar_t *>(password.data()), static_cast<DWORD>(password.length()) + 1);
  proxy_settings_changed();
}

void GlobalHandle::set_connect_timeout(DWORD ms) { InternetSetOption(m_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &ms, sizeof(DWORD)); }

void GlobalHandle::set_send_timeout(DWORD ms) { InternetSetOption(m_handle.get(), INTERNET_OPTION_SEND_TIMEOUT, &ms, sizeof(DWORD)); }

void GlobalHandle::set_receive_timeout(DWORD ms) { InternetSetOption(m_handle.get(), INTERNET_OPTION_RECEIVE_TIMEOUT, &ms, sizeof(DWORD)); }

UrlHandle::~UrlHandle() {
  if (m_handle.get())
    InternetCloseHandle(m_handle.get());
}

void UrlHandle::proxy_settings_changed() { InternetSetOption(m_handle.get(), INTERNET_OPTION_PROXY_SETTINGS_CHANGED, nullptr, 0); }

bool download_file(const UrlHandle &handle, std::ostream &stream, std::function<bool(int bytes_read, int total_bytes)> callback) {
  constexpr DWORD buf_size = 1024 * 1024;
  DWORD bytes_to_read = 0, bytes_read_total = 0, cur_index = 0, bytes_read = 0;
  static std::vector<char> file_buffer(buf_size);
  file_buffer.clear();
  std::vector<wchar_t> buf (32);
  DWORD size = static_cast<DWORD> (buf.size ());
  HttpQueryInfo(handle.get(), HTTP_QUERY_CONTENT_LENGTH, buf.data (), &size, nullptr);
  size_t idx;
  int total_bytes_to_read = 0;
  try {
    total_bytes_to_read = std::stoi (buf.data (), &idx);
  }
  catch (...) {
  }
  while (true) {
    if (callback && !callback(bytes_read_total, total_bytes_to_read))
      return false;
    InternetQueryDataAvailable(handle.get(), &bytes_to_read, 0, 0);
    if (bytes_to_read == 0)
      break;

    InternetReadFile(handle.get(), file_buffer.data(), std::min(bytes_to_read, buf_size), &bytes_read);
    if (bytes_read == 0)
      break;
    bytes_read_total += bytes_read;
    cur_index += bytes_read;
    stream.write(file_buffer.data(), bytes_read);
  }
  return true;
}

std::string download_text_file(const UrlHandle &handle) {
  std::stringstream ss;
  download_file(handle, ss);
  return ss.str();
}

GlobalHandle::GlobalHandle() = default;

GlobalHandle::GlobalHandle(const wchar_t *agent, DWORD access_type, const wchar_t *proxy_string, const wchar_t *proxy_bypass, DWORD flags) {
  m_handle = InternetOpen(agent, access_type, proxy_string, proxy_bypass, flags);
  if (!m_handle.get())
    throw_if_error();
}

GlobalHandle::~GlobalHandle() {
  if (m_handle.get())
    InternetCloseHandle(m_handle.get());
}
} // namespace WinInet
