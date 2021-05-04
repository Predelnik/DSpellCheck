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
#include "common/CommonFunctions.h"

class LanguageInfo {
public:
  std::wstring orig_name;
  std::wstring alias_name;
  bool for_all_users = false;

  LanguageInfo(std::wstring_view name, LanguageNameStyle language_name_style,
               bool for_all_users_arg = false) : orig_name (name) {
    for_all_users = for_all_users_arg;
    std::tie(alias_name, std::ignore) = apply_alias(name, language_name_style);
  }
};

inline bool less_aliases(const LanguageInfo &a, const LanguageInfo &b) {
  return a.alias_name < b.alias_name;
}

inline bool less_original(const LanguageInfo &a, const LanguageInfo &b) {
  return a.orig_name < b.orig_name;
}
