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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA.

#include "winapi.h"
#pragma warning(push)
#pragma warning(disable : 4091)
#include <DbgHelp.h>
#pragma warning(pop)
#include "Aclapi.h"
#include <cassert>

std::wstring get_edit_text(HWND edit) {
  auto length = Edit_GetTextLength(edit);
  std::vector<wchar_t> buf(length + 1);
  Edit_GetText(edit, buf.data(), static_cast<int>(buf.size()));
  return buf.data();
}

bool move_file_and_reset_security_descriptor(const wchar_t *from, const wchar_t *to) {
  // on why it is needed
  // https://blogs.msdn.microsoft.com/oldnewthing/20060824-16/?p=29973/
  auto ret = MoveFile(from, to);
  // here goes exact code from https://stackoverflow.com/a/20009217/1269661
  ACL g_null_acl;
  memset(&g_null_acl, 0, sizeof(g_null_acl));
  InitializeAcl(&g_null_acl, sizeof(g_null_acl), ACL_REVISION);
  SetNamedSecurityInfo(const_cast<wchar_t *>(to), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr,
                       static_cast<PACL>(&g_null_acl), nullptr);
  return ret != 0;
}

std::wstring get_class_name(HWND hwnd) {
  static const int max_class_name = 256;
  std::vector<wchar_t> buf(max_class_name);
  GetClassName(hwnd, buf.data(), max_class_name);
  return {buf.data()};
}

namespace WinApi {
WinBase::~WinBase() = default;

void WinBase::set_enabled(bool enabled) { EnableWindow(m_hwnd, enabled ? TRUE : FALSE); }

void WinBase::init(HWND hwnd) {
  m_hwnd = hwnd;
  check_hwnd();
  init_impl();
}

void ComboBox::check_hwnd() { assert(get_class_name(m_hwnd) == L"ComboBox"); }

std::optional<int> ComboBox::find_by_data(int data) {
  for (int i = 0; i < ComboBox_GetCount(m_hwnd); ++i)
    if (ComboBox_GetItemData(m_hwnd, i) == data)
      return i;
  return std::nullopt;
}

void ComboBox::clear() { ComboBox_ResetContent(m_hwnd); }

HWND create_tooltip(int tool_id, HWND h_dlg, const wchar_t *psz_text) {
  if (tool_id == 0 || h_dlg == nullptr || psz_text == nullptr) {
    return nullptr;
  }
  // Get the window of the tool.
  HWND hwnd_tool = GetDlgItem(h_dlg, tool_id);

  // Create the tooltip. g_hInst is the global instance handle.
  HWND hwnd_tip = CreateWindowEx(NULL, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, h_dlg,
                                 nullptr, static_cast<HINSTANCE>(get_h_module()), nullptr);

  if (hwnd_tool == nullptr || hwnd_tip == nullptr) {
    return nullptr;
  }

  // Associate the tooltip with the tool.
  TOOLINFO tool_info;
  memset(&tool_info, 0, sizeof(tool_info));
  tool_info.cbSize = sizeof(tool_info);
  tool_info.hwnd = h_dlg;
  tool_info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  tool_info.uId = reinterpret_cast<UINT_PTR>(hwnd_tool);
  tool_info.lpszText = const_cast<wchar_t *>(psz_text);
  SendMessage(hwnd_tip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&tool_info));

  return hwnd_tip;
}

bool delete_file(const wchar_t *path) {
  SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);
  return DeleteFile(path);
}

std::optional<int> library_bitness(const wchar_t *path) {
  auto h_file = CreateFile(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
  if (h_file == INVALID_HANDLE_VALUE)
    return {};
  auto h_map = CreateFileMapping(h_file, nullptr, PAGE_READONLY, 0, 0, nullptr);
  if (h_map == INVALID_HANDLE_VALUE) {
    CloseHandle(h_file);
    return {};
  }
  void *map_addr = MapViewOfFileEx(h_map, FILE_MAP_READ, 0, 0, 0, nullptr);
  auto pe_hdr = ImageNtHeader(map_addr);
  auto machine = pe_hdr->FileHeader.Machine;
  CloseHandle(h_map);
  CloseHandle(h_file);
  switch (machine) {
  case IMAGE_FILE_MACHINE_I386:
  default:
    return 32;
  case IMAGE_FILE_MACHINE_IA64:
  case IMAGE_FILE_MACHINE_AMD64:
    return 64;
  }
}
} // namespace WinApi
