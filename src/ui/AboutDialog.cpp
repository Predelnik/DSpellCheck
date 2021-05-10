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

#include "AboutDialog.h"

#include "common/Utility.h"
#include "plugin/Plugin.h"
#include "plugin/resource.h"

void AboutDialog::do_dialog() {
  if (!isCreated()) {
    create(IDD_ABOUT);
    goToCenter();
  }
  display();
}

#define STR_HELPER(x) L#x
#define STR(x) STR_HELPER(x)

void AboutDialog::update_compiler_version() {
  std::wstring compiler_name = L"Unknown Compiler";
#ifdef _MSC_FULL_VER
  std::wstring ver_str = STR(_MSC_FULL_VER);
  compiler_name = wstring_printf(L"MSVC cl %s.%s.%s.%02d", ver_str.substr(0, 2).c_str(), ver_str.substr(2, 2).c_str(), ver_str.substr(4, 5).c_str(),
                                 _MSC_BUILD);
#endif
  auto wnd = ::GetDlgItem(_hSelf, IDC_COMPILER_TEXT);
  auto str = wstring_printf(rc_str(IDS_BUILT_WITH_PS).c_str(), compiler_name.c_str());
  auto ret = Static_SetText(wnd, str.c_str ());
  (void)ret;
}

std::wstring get_product_and_version() {
  // get the filename of the executable containing the version resource
  std::vector<wchar_t> sz_filename(MAX_PATH + 1);
  if (GetModuleFileName(static_cast<HMODULE>(get_h_module()), sz_filename.data(), MAX_PATH) == 0) {
    return {};
  }

  // allocate a block of memory for the version info
  DWORD dummy;
  auto dw_size = GetFileVersionInfoSize(sz_filename.data(), &dummy);
  if (dw_size == 0) {
    return {};
  }
  std::vector<BYTE> data(dw_size);

  // load the version info
  if (!GetFileVersionInfo(sz_filename.data(), NULL, dw_size, &data[0])) {
    return {};
  }

  UINT ui_ver_len = 0;
  VS_FIXEDFILEINFO *p_fixed_info = nullptr; // pointer to fixed file info structure
  // get the fixed file info (language-independent)
  if (VerQueryValue(data.data(), TEXT("\\"), reinterpret_cast<void **>(&p_fixed_info),
                    static_cast<UINT *>(&ui_ver_len)) == 0) {
    return {};
  }
  return wstring_printf(rc_str(IDS_VERSION_PU_PU_PU_PU).c_str(),
                        HIWORD(p_fixed_info->dwProductVersionMS),
                        LOWORD(p_fixed_info->dwProductVersionMS),
                        HIWORD(p_fixed_info->dwProductVersionLS),
                        LOWORD(p_fixed_info->dwProductVersionLS));
}

void AboutDialog::init(HINSTANCE h_inst, HWND parent) {
  return Window::init(h_inst, parent);
}

INT_PTR AboutDialog::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) {
  switch (message) {
  case WM_INITDIALOG: {
    Static_SetText(::GetDlgItem(_hSelf, IDC_VERSION), get_product_and_version ().c_str ());
    update_compiler_version();
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
  }
  break;
  }
  return FALSE;
}
