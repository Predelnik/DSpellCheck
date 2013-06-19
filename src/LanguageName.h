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

struct LanguageName
{
  TCHAR *OrigName;
  TCHAR *AliasName;
  BOOL AliasApplied;
  LanguageName (TCHAR *Name, BOOL UseAlias = TRUE)
  {
    OrigName = 0;
    AliasName = 0;
    AliasApplied = FALSE;
    SetString (OrigName, Name);
    if (UseAlias)
      AliasApplied = SetStringWithAliasApplied (AliasName, Name);
    else
      SetString (AliasName, Name);
  }

  LanguageName (const LanguageName &rhs)
  {
    OrigName = 0;
    AliasName = 0;
    SetString (OrigName, rhs.OrigName);
    SetString (AliasName, rhs.AliasName);
    AliasApplied = rhs.AliasApplied;
  }

  LanguageName &operator = (const LanguageName &rhs)
  {
    if (&rhs == this)
      return *this;

    CLEAN_AND_ZERO_ARR (OrigName);
    CLEAN_AND_ZERO_ARR (AliasName);
    AliasApplied = rhs.AliasApplied;
    SetString (OrigName, rhs.OrigName);
    SetString (AliasName, rhs.AliasName);
    return *this;
  }

  ~LanguageName ()
  {
    CLEAN_AND_ZERO_ARR (OrigName);
    CLEAN_AND_ZERO_ARR (AliasName);
    AliasApplied = FALSE;
  }
};

inline bool CompareAliases (LanguageName &a, LanguageName &b)
{
  return _tcsicmp (a.AliasApplied ? a.AliasName : a.OrigName, b.AliasApplied ? b.AliasName : b.OrigName) < 0;
}

inline bool CompareOriginal (LanguageName &a, LanguageName &b)
{
  return _tcsicmp (a.OrigName, b.OrigName) < 0;
}
#endif // LANGUAGE_NAME_H