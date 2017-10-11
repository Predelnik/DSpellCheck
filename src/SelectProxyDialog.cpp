/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "SelectProxyDialog.h"

#include "DownloadDicsDlg.h"
#include "MainDef.h"
#include "SpellChecker.h"
#include "Plugin.h"

#include "utils/winapi.h"
#include "resource.h"

void SelectProxyDialog::do_dialog() {
  if (!isCreated()) {
    create(IDD_DIALOG_SELECT_PROXY);
  }
  goToCenter();
}

void SelectProxyDialog::apply_choice(SpellChecker *spell_checker_instance) {
  spell_checker_instance->set_proxy_user_name (get_edit_text(m_user_name).c_str ());
  spell_checker_instance->set_proxy_host_name (get_edit_text (m_host_name).c_str ());
  spell_checker_instance->set_proxy_password (get_edit_text (m_password).c_str ());
  spell_checker_instance->set_use_proxy(Button_GetCheck(m_use_proxy) == BST_CHECKED);
  spell_checker_instance->set_proxy_anonymous(Button_GetCheck(m_proxy_anonymous) == BST_CHECKED);
  spell_checker_instance->set_proxy_type(ComboBox_GetCurSel(m_proxy_type));
  auto text = get_edit_text (m_port);
  wchar_t *end_ptr;
  int x = wcstol(text.c_str(), &end_ptr, 10);
  spell_checker_instance->set_proxy_port(x);
  get_download_dics()->refresh();
}

void SelectProxyDialog::disable_controls() {
  bool proxy_is_used = Button_GetCheck(m_use_proxy);
  EnableWindow(m_proxy_anonymous, proxy_is_used);
  EnableWindow(m_user_name, proxy_is_used);
  EnableWindow(m_host_name, proxy_is_used);
  EnableWindow(m_password, proxy_is_used);
  EnableWindow(m_port, proxy_is_used);
  EnableWindow(m_proxy_type, proxy_is_used);

  if (proxy_is_used) {
    bool login_used = !Button_GetCheck(m_proxy_anonymous);
    bool anonymous_type = (ComboBox_GetCurSel(m_proxy_type) == 1);
    if (anonymous_type) {
      login_used = false;
      EnableWindow(m_proxy_anonymous, false);
    }
    EnableWindow(m_user_name, login_used);
    EnableWindow(m_password, login_used);
  }
}

void SelectProxyDialog::set_options(bool use_proxy, const wchar_t* host_name,
                                   const wchar_t* user_name, const wchar_t* password, int port,
                                   bool proxy_anonymous, int proxy_type) {
  Button_SetCheck(m_use_proxy, use_proxy ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_proxy_anonymous,
                  proxy_anonymous ? BST_CHECKED : BST_UNCHECKED);
  Edit_SetText(m_user_name, user_name);
  Edit_SetText(m_host_name, host_name);
  Edit_SetText(m_password, password);
  if (port < 0)
    port = 0;
  if (port > 65535)
    port = 65535;
  wchar_t buf[DEFAULT_BUF_SIZE];
  _itow(port, buf, 10);
  Edit_SetText(m_port, buf);
  disable_controls();
  ComboBox_SetCurSel(m_proxy_type, proxy_type);
}

INT_PTR SelectProxyDialog::run_dlg_proc(UINT message, WPARAM w_param,
                                 LPARAM /*lParam*/) {
  switch (message) {
  case WM_INITDIALOG: {
    m_port = ::GetDlgItem(_hSelf, IDC_PORT);
    m_user_name = ::GetDlgItem(_hSelf, IDC_USERNAME);
    m_host_name = ::GetDlgItem(_hSelf, IDC_HOSTNAME);
    m_password = ::GetDlgItem(_hSelf, IDC_PASSWORD);
    m_use_proxy = ::GetDlgItem(_hSelf, IDC_USEPROXY);
    m_proxy_anonymous = ::GetDlgItem(_hSelf, IDC_ANONYMOUS_LOGIN);
    m_proxy_type = ::GetDlgItem(_hSelf, IDC_PROXY_TYPE);
    ComboBox_AddString(m_proxy_type, L"FTP Web Proxy");
    ComboBox_AddString(m_proxy_type, L"FTP Gateway");
    get_spell_checker ()->update_select_proxy();
    return true;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        get_spell_checker()->apply_proxy_settings ();
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        get_spell_checker ()->update_select_proxy(); // Reset all settings
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
          wchar_t buf[DEFAULT_BUF_SIZE];
        Edit_GetText(m_port, buf, DEFAULT_BUF_SIZE);
        if (!*buf)
          return false;

        int x = wcstol(buf, &end_ptr, 10);
        if (*end_ptr)
          Edit_SetText(m_port, L"0");
        else if (x > 65535)
          Edit_SetText(m_port, L"65535");
        else if (x < 0)
          Edit_SetText(m_port, L"0");

        return false;
      }
    }
  } break;
  }
  return false;
}