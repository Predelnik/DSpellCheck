#/*
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

#include "SpellerController.h"

#ifndef USING_STATIC_LIBICONV
#define USING_STATIC_LIBICONV
#endif // !USING_STATIC_LIBICONV

#include "iconv.h"
#include "CommonFunctions.h"
#include "MainDef.h"

class Hunspell;

typedef stdext::hash_set <char *, hash_compare_strings> WordSet;
typedef std::set <char *, bool (*)(char *, char *)> SortedWordSet;

struct DicInfo
{
  Hunspell *Speller;
  iconv_t Converter;
  iconv_t BackConverter;
  iconv_t ConverterANSI;
  iconv_t BackConverterANSI;
  TCHAR *LocalDicPath;
  WordSet *LocalDic; // Stored in Dictionary encoding
};

struct AvailableLangInfo
{
  wchar_t *Name;
  int Type; // Type = 1 - System Dir Dictionary, 0 - Nomal Dictionary

  bool operator < (const AvailableLangInfo &rhs ) const
  {
    return (_tcscmp (Name, rhs.Name) < 0);
  }
};

class HunspellController : public SpellerController
{
public:
  HunspellController (HWND NppWindowArg);
  ~HunspellController ();
  std::vector<TCHAR*> *getLanguageList () override;
  void setLanguage (const wchar_t *Lang) override;
  void SetMultipleLanguages (std::vector<TCHAR *> *List) override; // Languages are from LangList
  BOOL CheckWord (char *Word) override; // Word in Utf-8 or ANSI
  BOOL IsWorking () override;
  std::vector<char *> *GetSuggestions (char *Word) override;
  void AddToDictionary (char *Word) override;
  void IgnoreAll (char *Word) override;
  void setPath (const wchar_t *dir) override;
  void setAdditionalPath (const wchar_t *dir) override;
  BOOL GetLangOnlySystem (const wchar_t *Lang) override;

  void WriteUserDic (WordSet *Target, TCHAR *Path);
  void ReadUserDic (WordSet *Target, TCHAR *Path);
  void SetUseOneDic (BOOL Value);
  void UpdateOnDicRemoval (TCHAR *Path, BOOL &NeedSingleLangReset, BOOL &NeedMultiLangReset);

private:
  DicInfo CreateHunspell (TCHAR *Name, int Type);
  BOOL SpellerCheckWord (DicInfo Dic, char *Word, EncodingType Encoding);
  void MessageBoxWordCannotBeAdded ();
public:
private:
  BOOL IsHunspellWorking;
  BOOL UseOneDic;
  TCHAR *DicDir;
  TCHAR *SysDicDir;
  std::set <AvailableLangInfo> *DicList;
  std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)> *AllHunspells;
  char *GetConvertedWord (const char *Source, iconv_t Converter);
  DicInfo SingularSpeller;
  DicInfo LastSelectedSpeller;
  DicInfo Empty;
  std::vector <DicInfo> *Spellers;
  WordSet *Memorized;
  WordSet *Ignored;
  BOOL InitialReadingBeenDone;
  char *TemporaryBuffer;
  TCHAR *UserDicPath; // For now only default one.
  TCHAR *SystemWrongDicPath; // Only for reading and then removing
  HWND NppWindow;
};
