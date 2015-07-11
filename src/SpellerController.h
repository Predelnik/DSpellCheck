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

#pragma  once

#include <MainDef.h>

class SpellerController
{
private:
protected:
public:
  virtual std::vector<TCHAR*> *getLanguageList () = 0;
  virtual void setLanguage (const wchar_t *Lang) = 0;
  virtual void SetMultipleLanguages (std::vector<TCHAR *> *List) = 0; // Languages are from LangList
  void setMultiMode (int Multi) { MultiMode = Multi; }                         // Multi - 1, Single - 0
  void setEncoding (EncodingType Encoding) {CurrentEncoding = Encoding; }
  virtual BOOL CheckWord (char *Word) = 0;                         // Word in Utf-8 or ANSI
  virtual std::vector<char *> *GetSuggestions (char *Word) = 0;
  virtual void AddToDictionary (char *Word) = 0;
  virtual void IgnoreAll (char *Word) = 0;
  virtual BOOL IsWorking () = 0;
  virtual void setPath (const wchar_t *dir) = 0;
  virtual void setAdditionalPath (const wchar_t *dir) = 0;
  virtual BOOL GetLangOnlySystem (const wchar_t *) { return false; };

private:
protected:
  int MultiMode;
  EncodingType CurrentEncoding;
public:
};
