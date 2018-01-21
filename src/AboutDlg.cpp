// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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

#include "AboutDlg.h"
#include "CommonFunctions.h"
#include "Plugin.h"
#include "resource.h"

void AboutDlg::do_dialog() {
  if (!isCreated()) {
    create(IDD_ABOUT);
    goToCenter();
  }
  display();
}

std::wstring get_product_and_version() {
  // get the filename of the executable containing the version resource
  std::vector<wchar_t> sz_filename (MAX_PATH + 1);
  if (GetModuleFileName(static_cast<HMODULE>(get_h_module()), sz_filename.data (), MAX_PATH) == 0) {
    return {};
  }

  // allocate a block of memory for the version info
  DWORD dummy;
  auto dw_size = GetFileVersionInfoSize(sz_filename.data (), &dummy);
  if (dw_size == 0) {
    return {};
  }
  std::vector<BYTE> data(dw_size);

  // load the version info
  if (!GetFileVersionInfo(sz_filename.data (), NULL, dw_size, &data[0])) {
    return {};
  }

  UINT ui_ver_len = 0;
  VS_FIXEDFILEINFO *p_fixed_info = nullptr; // pointer to fixed file info structure
  // get the fixed file info (language-independent)
  if (VerQueryValue(data.data (), TEXT("\\"), reinterpret_cast<void **>(&p_fixed_info),
                    static_cast<UINT *>(&ui_ver_len)) == 0) {
    return {};
  }
  return wstring_printf (rc_str (IDS_VERSION_PU_PU_PU_PU).c_str (),
           HIWORD(p_fixed_info->dwProductVersionMS),
           LOWORD(p_fixed_info->dwProductVersionMS),
           HIWORD(p_fixed_info->dwProductVersionLS),
           LOWORD(p_fixed_info->dwProductVersionLS));
}

void AboutDlg::init(HINSTANCE h_inst, HWND parent) {
  return Window::init(h_inst, parent);
}

INT_PTR AboutDlg::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) {
  switch (message) {
  case WM_INITDIALOG: {
    Static_SetText(::GetDlgItem(_hSelf, IDC_VERSION), get_product_and_version ().c_str ());
  }
    return TRUE;
  case WM_NOTIFY: {
    switch (reinterpret_cast<LPNMHDR>(l_param)->code) {
    case NM_CLICK: // Fall through to the next case.
    case NM_RETURN: {
      auto p_nm_link = reinterpret_cast<PNMLINK>(l_param);
      LITEM item = p_nm_link->item;

      ShellExecute(nullptr, L"open", item.szUrl, nullptr, nullptr, SW_SHOW);

      return TRUE;
    }
    }
    return FALSE;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDOK:
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        display(false);
        return TRUE;
      }
    }
  } break;
  }
  return FALSE;
}