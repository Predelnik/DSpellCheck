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

#include "WinApiControls.h"
#include <cassert>
#include "winapi.h"

namespace WinApi
{
WinBase::~WinBase() = default;

void WinBase::set_enabled(bool enabled) { EnableWindow(m_hwnd, enabled ? TRUE : FALSE); }

bool WinBase::is_enabled() const {
  return IsWindowEnabled (m_hwnd) != FALSE;
}

void WinBase::init(HWND hwnd) {
  m_hwnd = hwnd;
  check_hwnd();
  init_impl();
}

WinBase::operator bool() const {
  return m_hwnd;
}

void ComboBox::check_hwnd() { assert(get_class_name(m_hwnd) == L"ComboBox"); }

int ComboBox::current_index() const { return static_cast<int>(ComboBox_GetCurSel(m_hwnd)); }

std::wstring ComboBox::current_text() const {
  auto length = ComboBox_GetTextLength(m_hwnd);
  std::vector<wchar_t> buf(length + 1);
  ComboBox_GetText(m_hwnd, buf.data(), static_cast<int>(buf.size()));
  return buf.data();
}

int ComboBox::current_data() const { return static_cast<int>(ComboBox_GetItemData(m_hwnd, current_index())); }

void ComboBox::set_current_index(int index) {
  ComboBox_SetCurSel(m_hwnd, index);
}

std::optional<int> ComboBox::find_by_data(int data) {
  for (int i = 0; i < count (); ++i)
    if (ComboBox_GetItemData(m_hwnd, i) == data)
      return i;
  return std::nullopt;
}

void ComboBox::clear() { ComboBox_ResetContent(m_hwnd); }

int ComboBox::count() const {
  return ComboBox_GetCount(m_hwnd);
}
}
