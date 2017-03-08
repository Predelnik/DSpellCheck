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

#include "SelectProxy.h"

#include "DownloadDicsDlg.h"
#include "Controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "CommonFunctions.h"
#include "SpellChecker.h"
#include "Plugin.h"

#include "resource.h"

void SelectProxy::DoDialog() {
  if (!isCreated()) {
    create(IDD_DIALOG_SELECT_PROXY);
  }
  goToCenter();
}

void SelectProxy::init(HINSTANCE hInst, HWND Parent) {
  return Window::init(hInst, Parent);
}

void SelectProxy::ApplyChoice(SpellChecker *SpellCheckerInstance) {
  wchar_t *TBuf = 0;
  int BufSize = 0;
  BufSize = Edit_GetTextLength(HUserName) + 1;
  TBuf = new wchar_t[BufSize];
  Edit_GetText(HUserName, TBuf, BufSize);
  SpellCheckerInstance->SetProxyUserName(TBuf);
  CLEAN_AND_ZERO_ARR(TBuf);
  BufSize = Edit_GetTextLength(HHostName) + 1;
  TBuf = new wchar_t[BufSize];
  Edit_GetText(HHostName, TBuf, BufSize);
  SpellCheckerInstance->SetProxyHostName(TBuf);
  CLEAN_AND_ZERO_ARR(TBuf);
  BufSize = Edit_GetTextLength(HPassword) + 1;
  TBuf = new wchar_t[BufSize];
  Edit_GetText(HPassword, TBuf, BufSize);
  SpellCheckerInstance->SetProxyPassword(TBuf);
  CLEAN_AND_ZERO_ARR(TBuf);
  SpellCheckerInstance->SetUseProxy(Button_GetCheck(HUseProxy));
  SpellCheckerInstance->SetProxyAnonymous(Button_GetCheck(HProxyAnonymous));
  SpellCheckerInstance->SetProxyType(ComboBox_GetCurSel(HProxyType));
  TBuf = new wchar_t[DEFAULT_BUF_SIZE];
  Edit_GetText(HPort, TBuf, DEFAULT_BUF_SIZE);
  wchar_t *EndPtr;
  int x = wcstol(TBuf, &EndPtr, 10);
  SpellCheckerInstance->SetProxyPort(x);
  CLEAN_AND_ZERO_ARR(TBuf);
  GetDownloadDics()->Refresh();
}

void SelectProxy::DisableControls() {
  BOOL ProxyIsUsed = Button_GetCheck(HUseProxy);
  EnableWindow(HProxyAnonymous, ProxyIsUsed);
  EnableWindow(HUserName, ProxyIsUsed);
  EnableWindow(HHostName, ProxyIsUsed);
  EnableWindow(HPassword, ProxyIsUsed);
  EnableWindow(HPort, ProxyIsUsed);
  EnableWindow(HProxyType, ProxyIsUsed);

  if (ProxyIsUsed) {
    BOOL LoginUsed = !Button_GetCheck(HProxyAnonymous);
    BOOL AnonymousType = (ComboBox_GetCurSel(HProxyType) == 1);
    if (AnonymousType) {
      LoginUsed = FALSE;
      EnableWindow(HProxyAnonymous, FALSE);
    }
    EnableWindow(HUserName, LoginUsed);
    EnableWindow(HPassword, LoginUsed);
  }
}

void SelectProxy::SetOptions(BOOL UseProxy, wchar_t *HostName,
                             wchar_t *UserName, wchar_t *Password, int Port,
                             BOOL ProxyAnonymous, int ProxyType) {
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

INT_PTR SelectProxy::run_dlgProc(UINT message, WPARAM wParam,
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
    SendEvent(EID_UPDATE_SELECT_PROXY);
    return TRUE;
  } break;
  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case IDOK:
      if (HIWORD(wParam) == BN_CLICKED) {
        SendEvent(EID_APPLY_PROXY_SETTINGS);
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(wParam) == BN_CLICKED) {
        SendEvent(EID_UPDATE_SELECT_PROXY); // Reset all settings
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
        wchar_t *EndPtr = 0;
        int x;
        wchar_t Buf[DEFAULT_BUF_SIZE];
        Edit_GetText(HPort, Buf, DEFAULT_BUF_SIZE);
        if (!*Buf)
          return FALSE;

        x = wcstol(Buf, &EndPtr, 10);
        if (*EndPtr)
          Edit_SetText(HPort, L"0");
        else if (x > 65535)
          Edit_SetText(HPort, L"65535");
        else if (x < 0)
          Edit_SetText(HPort, L"0");

        return FALSE;
      }
    }
  } break;
  }
  return FALSE;
}