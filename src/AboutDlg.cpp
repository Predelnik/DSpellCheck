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


#include "AboutDlg.h"
#include "CommonFunctions.h"
#include "MainDef.h"
#include "Plugin.h"
#include "resource.h"

void AboutDlg::DoDialog() {
  if (!isCreated()) {
    create(IDD_ABOUT);
    goToCenter();
  }
  display();
}

std::wstring GetProductAndVersion() {
  // get the filename of the executable containing the version resource
  std::vector<wchar_t> szFilename (MAX_PATH + 1);
  if (GetModuleFileName((HMODULE)getHModule(), szFilename.data (), MAX_PATH) == 0) {
    return {};
  }

  // allocate a block of memory for the version info
  DWORD dummy;
  auto dwSize = GetFileVersionInfoSize(szFilename.data (), &dummy);
  if (dwSize == 0) {
    return {};
  }
  std::vector<BYTE> data(dwSize);

  // load the version info
  if (!GetFileVersionInfo(szFilename.data (), NULL, dwSize, &data[0])) {
    return {};
  }

  UINT uiVerLen = 0;
  VS_FIXEDFILEINFO *pFixedInfo = nullptr; // pointer to fixed file info structure
  // get the fixed file info (language-independent)
  if (VerQueryValue(data.data (), TEXT("\\"), reinterpret_cast<void **>(&pFixedInfo),
                    static_cast<UINT *>(&uiVerLen)) == 0) {
    return {};
  }
  return wstring_printf (L"Version: %u.%u.%u.%u",
           HIWORD(pFixedInfo->dwProductVersionMS),
           LOWORD(pFixedInfo->dwProductVersionMS),
           HIWORD(pFixedInfo->dwProductVersionLS),
           LOWORD(pFixedInfo->dwProductVersionLS));
}

void AboutDlg::init(HINSTANCE hInst, HWND Parent) {
  return Window::init(hInst, Parent);
}

INT_PTR AboutDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_INITDIALOG: {
    Static_SetText(::GetDlgItem(_hSelf, IDC_VERSION), GetProductAndVersion ().c_str ());
  }
    return true;
  case WM_NOTIFY: {
    switch (((LPNMHDR)lParam)->code) {
    case NM_CLICK: // Fall through to the next case.
    case NM_RETURN: {
      PNMLINK pNMLink = (PNMLINK)lParam;
      LITEM item = pNMLink->item;

      ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);

      return true;
    }
    }
    return false;
  } break;
  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case IDOK:
    case IDCANCEL:
      if (HIWORD(wParam) == BN_CLICKED) {
        display(false);
        return true;
      }
    }
  } break;
  }
  return false;
}