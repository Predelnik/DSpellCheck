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

#include <CommCtrl.h>

#include "SelectProxyDialog.h"

#include "DownloadDicsDlg.h"
#include "CheckedList/CheckedList.h"
#include "MainDef.h"
#include "CommonFunctions.h"
#include "SpellChecker.h"
#include "Plugin.h"

#include "utils/winapi.h"
#include "resource.h"

void SelectProxyDialog::DoDialog() {
  if (!isCreated()) {
    create(IDD_DIALOG_SELECT_PROXY);
  }
  goToCenter();
}

void SelectProxyDialog::ApplyChoice(SpellChecker *SpellCheckerInstance) {
  SpellCheckerInstance->SetProxyUserName (getEditText(HUserName).c_str ());
  SpellCheckerInstance->SetProxyHostName (getEditText (HHostName).c_str ());
  SpellCheckerInstance->SetProxyPassword (getEditText (HPassword).c_str ());
  SpellCheckerInstance->SetUseProxy(Button_GetCheck(HUseProxy) == BST_CHECKED);
  SpellCheckerInstance->SetProxyAnonymous(Button_GetCheck(HProxyAnonymous) == BST_CHECKED);
  SpellCheckerInstance->SetProxyType(ComboBox_GetCurSel(HProxyType));
  auto text = getEditText (HPort);
  wchar_t *EndPtr;
  int x = wcstol(text.c_str(), &EndPtr, 10);
  SpellCheckerInstance->SetProxyPort(x);
  get_download_dics()->refresh();
}

void SelectProxyDialog::DisableControls() {
  bool ProxyIsUsed = Button_GetCheck(HUseProxy);
  EnableWindow(HProxyAnonymous, ProxyIsUsed);
  EnableWindow(HUserName, ProxyIsUsed);
  EnableWindow(HHostName, ProxyIsUsed);
  EnableWindow(HPassword, ProxyIsUsed);
  EnableWindow(HPort, ProxyIsUsed);
  EnableWindow(HProxyType, ProxyIsUsed);

  if (ProxyIsUsed) {
    bool LoginUsed = !Button_GetCheck(HProxyAnonymous);
    bool AnonymousType = (ComboBox_GetCurSel(HProxyType) == 1);
    if (AnonymousType) {
      LoginUsed = false;
      EnableWindow(HProxyAnonymous, false);
    }
    EnableWindow(HUserName, LoginUsed);
    EnableWindow(HPassword, LoginUsed);
  }
}

void SelectProxyDialog::SetOptions(bool UseProxy, const wchar_t* HostName,
                                   const wchar_t* UserName, const wchar_t* Password, int Port,
                                   bool ProxyAnonymous, int ProxyType) {
  Button_SetCheck(HUseProxy, UseProxy ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(HProxyAnonymous,
                  ProxyAnonymous ? BST_CHECKED : BST_UNCHECKED);
  Edit_SetText(HUserName, UserName);
  Edit_SetText(HHostName, HostName);
  Edit_SetText(HPassword, Password);
  if (Port < 0)
    Port = 0;
  if (Port > 65535)
    Port = 65535;
  wchar_t Buf[DEFAULT_BUF_SIZE];
  _itow(Port, Buf, 10);
  Edit_SetText(HPort, Buf);
  DisableControls();
  ComboBox_SetCurSel(HProxyType, ProxyType);
}

INT_PTR SelectProxyDialog::run_dlg_proc(UINT message, WPARAM wParam,
                                 LPARAM /*lParam*/) {
  switch (message) {
  case WM_INITDIALOG: {
    HPort = ::GetDlgItem(_hSelf, IDC_PORT);
    HUserName = ::GetDlgItem(_hSelf, IDC_USERNAME);
    HHostName = ::GetDlgItem(_hSelf, IDC_HOSTNAME);
    HPassword = ::GetDlgItem(_hSelf, IDC_PASSWORD);
    HUseProxy = ::GetDlgItem(_hSelf, IDC_USEPROXY);
    HProxyAnonymous = ::GetDlgItem(_hSelf, IDC_ANONYMOUS_LOGIN);
    HProxyType = ::GetDlgItem(_hSelf, IDC_PROXY_TYPE);
    ComboBox_AddString(HProxyType, L"FTP Web Proxy");
    ComboBox_AddString(HProxyType, L"FTP Gateway");
    get_spell_checker ()->updateSelectProxy();
    return true;
  }
  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case IDOK:
      if (HIWORD(wParam) == BN_CLICKED) {
        get_spell_checker()->applyProxySettings ();
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(wParam) == BN_CLICKED) {
        get_spell_checker ()->updateSelectProxy(); // Reset all settings
        display(false);
      }
      break;
    case IDC_USEPROXY:
    case IDC_ANONYMOUS_LOGIN: {
      if (HIWORD(wParam) == BN_CLICKED)
        DisableControls();
    } break;
    case IDC_PROXY_TYPE:
      if (HIWORD(wParam) == CBN_SELCHANGE)
        DisableControls();
      break;
    case IDC_PORT:
      if (HIWORD(wParam) == EN_CHANGE) {
        wchar_t *EndPtr = nullptr;
        int x;
        wchar_t Buf[DEFAULT_BUF_SIZE];
        Edit_GetText(HPort, Buf, DEFAULT_BUF_SIZE);
        if (!*Buf)
          return false;

        x = wcstol(Buf, &EndPtr, 10);
        if (*EndPtr)
          Edit_SetText(HPort, L"0");
        else if (x > 65535)
          Edit_SetText(HPort, L"65535");
        else if (x < 0)
          Edit_SetText(HPort, L"0");

        return false;
      }
    }
  } break;
  }
  return false;
}