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

#pragma once

struct MenuItem {
  std::wstring text;
  BYTE id;
  bool separator;
  MenuItem(const wchar_t *text_arg, int id_arg,
                      bool separator_arg = false);
  void append_to_menu (HMENU menu, int insert_pos = -1) const;

  template <typename Range>
  static void append_to_menu (HMENU menu, const Range &range)
  {
    for (auto &item : range)
      item.append_to_menu (menu);
  }

  template <typename Range>
  static void prepend_to_menu (HMENU menu, const Range &range)
  {
    for (auto it = range.rbegin (); it != range.rend (); ++it)
      it->append_to_menu (menu, 0);
  }
};
