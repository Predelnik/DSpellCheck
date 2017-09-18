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
  wchar_t *OrigName;
  std::wstring AliasName;
  bool AliasApplied;
  LanguageName(const wchar_t *Name, bool UseAlias = true) {
    OrigName = nullptr;
    AliasApplied = false;
    SetString(OrigName, Name);
    if (UseAlias)
      std::tie (AliasName, AliasApplied) = applyAlias(Name);
    else
      AliasName = Name;
  }

  LanguageName(const LanguageName &rhs) {
    OrigName = nullptr;
    SetString(OrigName, rhs.OrigName);
    AliasName = rhs.AliasName;
    AliasApplied = rhs.AliasApplied;
  }

  LanguageName &operator=(const LanguageName &rhs) {
    if (&rhs == this)
      return *this;

    CLEAN_AND_ZERO_ARR(OrigName);
    AliasApplied = rhs.AliasApplied;
    SetString(OrigName, rhs.OrigName);
    AliasName = rhs.AliasName;
    return *this;
  }

  ~LanguageName() {
    CLEAN_AND_ZERO_ARR(OrigName);
    AliasApplied = false;
  }
};

inline bool lessAliases(const LanguageName &a, const LanguageName &b) {
  return wcscmp(a.AliasApplied ? a.AliasName.c_str () : a.OrigName,
                b.AliasApplied ? b.AliasName.c_str () : b.OrigName) < 0;
}

inline bool lessOriginal(const LanguageName &a, const LanguageName &b) {
  return wcscmp(a.OrigName, b.OrigName) < 0;
}
