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

#include "MenuItem.h"
#include "plugin/MainDefs.h"

MenuItem::MenuItem(const wchar_t *text_arg, int id_arg,
                                         bool separator_arg /*= false*/) {
  text = text_arg;
  id = static_cast<BYTE>(id_arg);
  separator = separator_arg;
}

void MenuItem::append_to_menu(HMENU menu, int insert_pos) const {
MENUITEMINFO mi;
  memset(&mi, 0, sizeof(mi));
  mi.cbSize = sizeof(MENUITEMINFO);
  if (separator) {
    mi.fType = MFT_SEPARATOR;
  } else {
    mi.fType = MFT_STRING;
    mi.fMask = MIIM_ID | MIIM_TYPE;
    if (!children.empty ()) {
      mi.fMask |= MIIM_SUBMENU;
      mi.hSubMenu = CreatePopupMenu();
      for (auto &child : children) {
        child.append_to_menu (mi.hSubMenu);
      }
    }
    if (!get_use_allocated_ids())
      mi.wID = MAKEWORD(id, menu_id::plguin_menu);
    else
      mi.wID = get_context_menu_id_start() + id;

    mi.dwTypeData = const_cast<wchar_t *>(text.data ());
    mi.cch = static_cast<int>(text.length ()) + 1;
  }
  if (insert_pos == -1)
    InsertMenuItem(menu, GetMenuItemCount(menu), 1, &mi);
  else
    InsertMenuItem(menu, insert_pos, 1, &mi);
}
