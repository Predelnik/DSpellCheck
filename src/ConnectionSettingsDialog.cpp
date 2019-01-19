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

#include "ConnectionSettingsDialog.h"

#include "DownloadDicsDlg.h"
#include "MainDefs.h"
#include "Plugin.h"
#include "SpellChecker.h"

#include "Settings.h"
#include "resource.h"
#include "utils/winapi.h"

ConnectionSettingsDialog::ConnectionSettingsDialog(Settings &settings,
                                     DownloadDicsDlg &download_dics_dlg)
    : m_download_dics_dlg(download_dics_dlg), m_settings(settings) {}

void ConnectionSettingsDialog::do_dialog() {
  if (!isCreated()) {
    create(IDD_CONNECTION_SETTINGS_DIALOG);
  }
  goToCenter();
}

void ConnectionSettingsDialog::apply_choice() {
  auto mut_settings = m_settings.modify();
  mut_settings->proxy_user_name = get_edit_text(m_user_name);
  mut_settings->proxy_host_name = get_edit_text(m_host_name);
  mut_settings->proxy_password = get_edit_text(m_password);
  mut_settings->ftp_use_passive_mode = Button_GetCheck(m_passive_mode_cb) == BST_CHECKED;
  mut_settings->use_proxy = Button_GetCheck(m_use_proxy) == BST_CHECKED;
  mut_settings->proxy_is_anonymous =
      Button_GetCheck(m_proxy_anonymous) == BST_CHECKED;
  mut_settings->proxy_type = m_proxy_type_cmb.current_data();
  auto text = get_edit_text(m_port);
  wchar_t *end_ptr;
  int x = wcstol(text.c_str(), &end_ptr, 10);
  mut_settings->proxy_port = (x);
  m_download_dics_dlg.refresh();
}

void ConnectionSettingsDialog::disable_controls() {
  bool proxy_is_used = Button_GetCheck(m_use_proxy);
  auto show = proxy_is_used ? TRUE : FALSE;
  EnableWindow(m_proxy_anonymous, show);
  EnableWindow(m_user_name, show);
  EnableWindow(m_host_name, show);
  EnableWindow(m_password, show);
  EnableWindow(m_port, show);
  m_proxy_type_cmb.set_enabled(proxy_is_used);

  if (proxy_is_used) {
    auto login_used = Button_GetCheck(m_proxy_anonymous) ? FALSE : TRUE;
    bool anonymous_type =
        (m_proxy_type_cmb.current_data() == ProxyType::ftp_gateway);
    if (anonymous_type) {
      login_used = FALSE;
      EnableWindow(m_proxy_anonymous, FALSE);
    }
    EnableWindow(m_user_name, login_used);
    EnableWindow(m_password, login_used);
  }
}

void ConnectionSettingsDialog::update_controls() {
  Button_SetCheck(m_use_proxy,
                  m_settings.use_proxy ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_proxy_anonymous,
                  m_settings.proxy_is_anonymous ? BST_CHECKED : BST_UNCHECKED);
  Edit_SetText(m_user_name, m_settings.proxy_user_name.c_str());
  Edit_SetText(m_host_name, m_settings.proxy_host_name.c_str());
  Edit_SetText(m_password, m_settings.proxy_password.c_str());
  Edit_SetText(m_port, std::to_wstring(m_settings.proxy_port).c_str());
  Button_SetCheck (m_passive_mode_cb, m_settings.ftp_use_passive_mode);
  disable_controls();
  m_proxy_type_cmb.set_current(m_settings.proxy_type);
  m_proxy_type_cmb.set_current(m_settings.proxy_type);
}

INT_PTR ConnectionSettingsDialog::run_dlg_proc(UINT message, WPARAM w_param,
                                        LPARAM /*lParam*/) {
  switch (message) {
  case WM_INITDIALOG: {
    m_port = ::GetDlgItem(_hSelf, IDC_PORT);
    m_user_name = ::GetDlgItem(_hSelf, IDC_USERNAME);
    m_host_name = ::GetDlgItem(_hSelf, IDC_HOSTNAME);
    m_password = ::GetDlgItem(_hSelf, IDC_PASSWORD);
    m_use_proxy = ::GetDlgItem(_hSelf, IDC_USEPROXY);
    m_proxy_anonymous = ::GetDlgItem(_hSelf, IDC_ANONYMOUS_LOGIN);
    m_proxy_type_cmb.init(::GetDlgItem(_hSelf, IDC_PROXY_TYPE));
    m_passive_mode_cb = ::GetDlgItem(_hSelf, IDC_FTP_PASSIVE_MODE_CB);
    update_controls();
    return TRUE;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        apply_choice();
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        update_controls(); // Reset all settings
        display(false);
      }
      break;
    case IDC_USEPROXY:
    case IDC_ANONYMOUS_LOGIN: {
      if (HIWORD(w_param) == BN_CLICKED)
        disable_controls();
    } break;
    case IDC_PROXY_TYPE:
      if (HIWORD(w_param) == CBN_SELCHANGE)
        disable_controls();
      break;
    case IDC_PORT:
      if (HIWORD(w_param) == EN_CHANGE) {
        wchar_t *end_ptr = nullptr;
        wchar_t buf[default_buf_size];
        Edit_GetText(m_port, buf, default_buf_size);
        if (*buf == L'\0')
          return FALSE;

        int x = wcstol(buf, &end_ptr, 10);
        if (*end_ptr != L'\0')
          Edit_SetText(m_port, L"0");
        else if (x > 65535)
          Edit_SetText(m_port, L"65535");
        else if (x < 0)
          Edit_SetText(m_port, L"0");

        return FALSE;
      }
    }
  } break;
  }
  return FALSE;
}
