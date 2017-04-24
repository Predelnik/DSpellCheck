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

#ifndef LANGUAGE_NAME_H
#define LANGUAGE_NAME_H
#include "CommonFunctions.h"

struct SettingsData;

struct LanguageName
{
  std::wstring originalName;
  std::wstring aliasName;
  bool aliasApplied = false;
  bool systemOnly = false;
  LanguageName (const std::wstring &name, bool useAlias = true, bool systemOnlyArg = false)
  {
    systemOnly = systemOnlyArg;
    aliasApplied = FALSE;
    originalName = name;
    if (useAlias)
      {
        wchar_t *aliasNameTemp = nullptr;
        aliasApplied = setStringWithAliasApplied (aliasNameTemp, name.c_str ());
        aliasName = aliasNameTemp;
      }
    else
      aliasName = name;
  }

  std::wstring getLanguageName (const SettingsData &settings, bool forRemoveDialog = false) const;
};

inline bool CompareAliases (LanguageName &a, LanguageName &b)
{
  return (a.aliasApplied ? a.aliasName : a.originalName) < (b.aliasApplied ? b.aliasName : b.originalName);
}

inline bool CompareOriginal (LanguageName &a, LanguageName &b)
{
  return a.originalName < b.originalName;
}
#endif // LANGUAGE_NAME_H