#pragma once
#include <Windows.h>
#include <WinInet.h>
#include "move_only.h"
#include "remap_rvalues_for.h"

namespace WinInet {
class GlobalHandle {
public:
  GlobalHandle ();
  GlobalHandle(const wchar_t *agent, DWORD access_type,
                const wchar_t *proxy_string,
                const wchar_t *proxy_bypass = nullptr, DWORD flags = 0);
  void set_connect_timeout(DWORD ms);
  void set_send_timeout(DWORD ms);
  void set_receive_timeout(DWORD ms);
  GlobalHandle (GlobalHandle&&) = default;
  GlobalHandle &operator=(GlobalHandle&&) = default;
  ~GlobalHandle();
  HINTERNET get() const { return m_handle.get (); }

private:
  move_only<HINTERNET> m_handle;
};

class CreateGlobalHandle {
  using Self = CreateGlobalHandle;

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
  operator GlobalHandle() const;

private:
  const wchar_t *m_agent = nullptr;
  DWORD m_access_type = INTERNET_OPEN_TYPE_DIRECT;
  const wchar_t *m_proxy_string = nullptr;
};

class UrlHandle {
public:
  UrlHandle(HINTERNET h_internet, const wchar_t *url,
                   std::wstring_view headers, DWORD flags, void *context);
  UrlHandle (UrlHandle &&) = default;
  UrlHandle &operator= (UrlHandle &&) = default;
  HINTERNET get() const { return m_handle.get (); };
  void set_proxy_username(std::wstring_view username);
  void set_proxy_password(std::wstring_view password);
  ~UrlHandle();

private:
  void proxy_settings_changed();

private:
  move_only<HINTERNET> m_handle;
};

class WinInetOpenUrl {
  using Self = WinInetOpenUrl;

public:
  WinInetOpenUrl(const GlobalHandle &inet_handle, const wchar_t *url)
      : m_inet_handle(inet_handle), m_url(url) {}

  Self &set_headers(std::wstring_view headers) & { m_headers = headers; return *this; }
  REMAP_RVALUES_FOR(set_headers);
  Self &set_context(void *context) & { m_context = context; return *this; }
  REMAP_RVALUES_FOR(set_context);
  Self &set_flags(DWORD flags) & { m_flags = flags; return *this; }
  REMAP_RVALUES_FOR(set_flags);

  operator UrlHandle() {
    UrlHandle handle(m_inet_handle.get(), m_url, m_headers, m_flags,
                            m_context);
    return handle;
  }

private:
  std::wstring_view m_headers;
  const GlobalHandle &m_inet_handle;
  const wchar_t *m_url;
  DWORD m_flags = 0;
  void *m_context = nullptr;
};

// returns false if downloading was cancelled
bool download_file(const UrlHandle &handle, std::ostream &stream, std::function<bool(int bytes_read, int total_bytes)> callback = {});

// returns content of target text file or throws an exception
std::string download_text_file(const UrlHandle &handle);
} // namespace WinInet
