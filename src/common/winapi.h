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

#pragma once
#include "move_only.h"

// various winapi helpers
namespace WinApi {
std::wstring get_edit_text(HWND edit);
bool move_file_and_reset_security_descriptor(const wchar_t *from, const wchar_t *to);

std::wstring get_class_name(HWND hwnd);
std::optional<std::wstring> browse_for_directory(HWND parent_wnd, const wchar_t *initial_path);

HWND create_tooltip(int tool_id, HWND h_dlg, const wchar_t *psz_text);
bool delete_file(const wchar_t *path);
std::optional<int> library_bitness(const wchar_t *path);

bool is_locale_info_available();
std::wstring get_locale_info(const wchar_t *locale_name, LCTYPE type);

class Brush {
  using Self = Brush;
public:
  Brush () = default;
  Brush (HBRUSH brush) : m_brush (brush) {}
  HBRUSH get () const { return m_brush.get(); }
  Brush (const Self&) = delete;
  Brush &operator= (const Self&) = delete;
  Brush (Self&&) = default;
  Brush &operator= (Self&&) = default;
  ~Brush () { DeleteObject (m_brush.get()); }

private:
  move_only<HBRUSH> m_brush;
};
} // namespace WinApi
