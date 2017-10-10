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

#pragma once
#include "CommonFunctions.h"

struct LanguageName {
  std::wstring orig_name;
  std::wstring alias_name;
  bool alias_applied;
  LanguageName(const wchar_t *name, bool use_alias = true) {
    alias_applied = false;
    orig_name = name;
    if (use_alias)
      std::tie (alias_name, alias_applied) = apply_alias(name);
    else
      alias_name = name;
  }
};

inline bool less_aliases(const LanguageName &a, const LanguageName &b) {
  return (a.alias_applied ? a.alias_name : a.orig_name) <
         (b.alias_applied ? b.alias_name : b.orig_name);
}

inline bool less_original(const LanguageName &a, const LanguageName &b) {
  return a.orig_name < b.orig_name;
}
