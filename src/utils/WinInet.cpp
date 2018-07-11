#include "WinInet.h"
#include "Win32Exception.h"
#include <sstream>

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
  if (!m_handle.get ())
    throw_if_error();
}

void WinInetUrlHandle::set_proxy_username(std::wstring_view username) {
  InternetSetOption(m_handle.get (), INTERNET_OPTION_PROXY_USERNAME,
                    const_cast<wchar_t *>(username.data()),
                    static_cast<DWORD>(username.length()) + 1);
  proxy_settings_changed();
}

void WinInetUrlHandle::set_proxy_password(std::wstring_view password) {
  InternetSetOption(m_handle.get (), INTERNET_OPTION_PROXY_PASSWORD,
                    const_cast<wchar_t *>(password.data()),
                    static_cast<DWORD>(password.length()) + 1);
  proxy_settings_changed();
}

void WinInetHandle::set_connect_timeout(DWORD ms) {
  InternetSetOption(m_handle.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &ms,
                    sizeof(DWORD));
}

void WinInetHandle::set_send_timeout(DWORD ms) {
  InternetSetOption(m_handle.get(), INTERNET_OPTION_SEND_TIMEOUT, &ms,
                    sizeof(DWORD));
}

void WinInetHandle::set_receive_timeout(DWORD ms) {
  InternetSetOption(m_handle.get(), INTERNET_OPTION_RECEIVE_TIMEOUT, &ms,
                    sizeof(DWORD));
}

WinInetUrlHandle::~WinInetUrlHandle()
{
  if (m_handle.get ())
    InternetCloseHandle(m_handle.get ());
}

void WinInetUrlHandle::proxy_settings_changed() {
  InternetSetOption(m_handle.get (), INTERNET_OPTION_PROXY_SETTINGS_CHANGED, nullptr,
                    0);
}

void download_file(const WinInetUrlHandle& handle, std::ostream& stream)
{
	constexpr DWORD buf_size = 1024 * 1024;
	DWORD bytes_to_read = 0, bytes_read_total = 0, cur_index = 0, bytes_read = 0;
	static std::vector<char> file_buffer(buf_size);
	file_buffer.clear();
	while (true) {
		InternetQueryDataAvailable(handle.get(), &bytes_to_read, 0, 0);
		if (bytes_to_read == 0)
			break;

		InternetReadFile(handle.get(), file_buffer.data(),
			std::min (bytes_to_read, buf_size), &bytes_read);
		if (bytes_read == 0)
			break;
		bytes_read_total += bytes_read;
		cur_index += bytes_read;
		stream.write(file_buffer.data (), bytes_read);
	}
}

std::string download_text_file(const WinInetUrlHandle &handle) {
	std::stringstream ss;
	download_file(handle, ss);
	return ss.str();
}

WinInetHandle::WinInetHandle() = default;

WinInetHandle::WinInetHandle(const wchar_t *agent, DWORD access_type,
                             const wchar_t *proxy_string,
                             const wchar_t *proxy_bypass, DWORD flags) {
  m_handle =
      InternetOpen(agent, access_type, proxy_string, proxy_bypass, flags);
  if (!m_handle.get())
    throw_if_error();
}

WinInetHandle::~WinInetHandle()
{
  if (m_handle.get ())
    InternetCloseHandle(m_handle.get());
}
} // namespace WinInet
