#pragma once
#include <Windows.h>
#include <WinInet.h>
#include "move_only.h"
#include "remap_rvalues_for.h"

namespace WinInet {
class WinInetHandle {
public:
  WinInetHandle ();
  WinInetHandle(const wchar_t *agent, DWORD access_type,
                const wchar_t *proxy_string,
                const wchar_t *proxy_bypass = nullptr, DWORD flags = 0);
  void set_connect_timeout(DWORD ms);
  void set_send_timeout(DWORD ms);
  void set_receive_timeout(DWORD ms);
  WinInetHandle (WinInetHandle&&) = default;
  WinInetHandle &operator=(WinInetHandle&&) = default;
  ~WinInetHandle();
  HINTERNET get() const { return m_handle.get (); }

private:
  move_only<HINTERNET> m_handle;
};

class CreateWinInetHandle {
  using Self = CreateWinInetHandle;

public:
  Self &agent(const wchar_t *agent) & {
    m_agent = agent;
    return *this;
  }
  REMAP_RVALUES_FOR (agent)
  Self &access_type(DWORD access_type) & {
    m_access_type = access_type;
    return *this;
  }
  REMAP_RVALUES_FOR (access_type)
  Self &proxy_string(const wchar_t *proxy_string) & {
    m_proxy_string = proxy_string;
    return *this;
  }
  REMAP_RVALUES_FOR (proxy_string)
  operator WinInetHandle() const;

private:
  const wchar_t *m_agent = nullptr;
  DWORD m_access_type = INTERNET_OPEN_TYPE_DIRECT;
  const wchar_t *m_proxy_string = nullptr;
};

class WinInetUrlHandle {
public:
  WinInetUrlHandle(HINTERNET h_internet, const wchar_t *url,
                   std::wstring_view headers, DWORD flags, void *context);
  WinInetUrlHandle (WinInetUrlHandle &&) = default;
  WinInetUrlHandle &operator= (WinInetUrlHandle &&) = default;
  HINTERNET get() const { return m_handle.get (); };
  void set_proxy_username(std::wstring_view username);
  void set_proxy_password(std::wstring_view password);
  ~WinInetUrlHandle();

private:
  void proxy_settings_changed();

private:
  move_only<HINTERNET> m_handle;
};

class WinInetOpenUrl {
  using Self = WinInetOpenUrl;

public:
  WinInetOpenUrl(const WinInetHandle &inet_handle, const wchar_t *url)
      : m_inet_handle(inet_handle), m_url(url) {}

  void set_headers(std::wstring_view headers) { m_headers = headers; }
  void set_context(void *context) { m_context = context; }
  void set_flags(DWORD flags) { m_flags = flags; }

  operator WinInetUrlHandle() {
    WinInetUrlHandle handle(m_inet_handle.get(), m_url, m_headers, m_flags,
                            m_context);
    return handle;
  }

private:
  std::wstring_view m_headers;
  const WinInetHandle &m_inet_handle;
  const wchar_t *m_url;
  DWORD m_flags = 0;
  void *m_context = nullptr;
};

// returns content of target text file or throws an exception
void download_file(const WinInetUrlHandle &handle, std::ostream &stream);
std::string download_text_file(const WinInetUrlHandle &handle);
} // namespace WinInet
