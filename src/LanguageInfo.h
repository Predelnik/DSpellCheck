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

class LanguageInfo {
public:
  std::wstring orig_name;
  std::wstring alias_name;
  bool alias_applied;
  bool for_all_users = false;

  LanguageInfo(std::wstring_view name, bool use_alias = true,
               bool for_all_users_arg = false) {
    alias_applied = false;
    orig_name = name;
    for_all_users = for_all_users_arg;
    if (use_alias) {
      std::tie(alias_name, alias_applied) = apply_alias(name);
    } else {
      alias_name = name;
    }
  }

  const wchar_t *get_aliased_name(bool use_alias) {
    return (use_alias ? alias_name : orig_name).c_str();
  }
};

inline bool less_aliases(const LanguageInfo &a, const LanguageInfo &b) {
  return (a.alias_applied ? a.alias_name : a.orig_name) <
         (b.alias_applied ? b.alias_name : b.orig_name);
}

inline bool less_original(const LanguageInfo &a, const LanguageInfo &b) {
  return a.orig_name < b.orig_name;
}
