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
  std::wstring OrigName;
  std::wstring AliasName;
  bool AliasApplied;
  LanguageName(const wchar_t *Name, bool UseAlias = true) {
    AliasApplied = false;
    OrigName = Name;
    if (UseAlias)
      std::tie (AliasName, AliasApplied) = applyAlias(Name);
    else
      AliasName = Name;
  }
};

inline bool lessAliases(const LanguageName &a, const LanguageName &b) {
  return (a.AliasApplied ? a.AliasName : a.OrigName) <
         (b.AliasApplied ? b.AliasName : b.OrigName);
}

inline bool lessOriginal(const LanguageName &a, const LanguageName &b) {
  return a.OrigName < b.OrigName;
}
