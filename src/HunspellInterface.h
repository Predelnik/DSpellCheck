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

#include "AbstractSpellerInterface.h"

#include "iconv.h"
#include "CommonFunctions.h"
#include "MainDef.h"

class Hunspell;

typedef std::unordered_set<char *, hash_compare_strings> WordSet;
typedef std::set<char *, bool (*)(char *, char *)> SortedWordSet;

struct DicInfo {
  Hunspell *Speller;
  iconv_t Converter;
  iconv_t BackConverter;
  iconv_t ConverterANSI;
  iconv_t BackConverterANSI;
  wchar_t *LocalDicPath;
  WordSet *LocalDic; // Stored in Dictionary encoding
};

struct AvailableLangInfo {
  wchar_t *Name;
  int Type; // Type = 1 - System Dir Dictionary, 0 - Nomal Dictionary

  bool operator<(const AvailableLangInfo &rhs) const {
    return (wcscmp(Name, rhs.Name) < 0);
  }
};

class HunspellInterface : public AbstractSpellerInterface {
public:
  HunspellInterface(HWND NppWindowArg);
  ~HunspellInterface();
  __override virtual std::vector<wchar_t *> *GetLanguageList();
  __override virtual void SetLanguage(wchar_t *Lang);
  __override virtual void SetMultipleLanguages(
      std::vector<wchar_t *> *List);             // Languages are from LangList
  __override virtual bool CheckWord(char *Word); // Word in Utf-8 or ANSI
  __override virtual bool IsWorking();
  __override virtual std::vector<char *> *GetSuggestions(char *Word);
  __override virtual void AddToDictionary(char *Word);
  __override virtual void IgnoreAll(char *Word);

  void SetDirectory(wchar_t *Dir);
  void SetAdditionalDirectory(wchar_t *Dir);
  void WriteUserDic(WordSet *Target, wchar_t *Path);
  void ReadUserDic(WordSet *Target, wchar_t *Path);
  void SetUseOneDic(bool Value);
  void UpdateOnDicRemoval(wchar_t *Path, bool &NeedSingleLangReset,
                          bool &NeedMultiLangReset);
  bool GetLangOnlySystem(wchar_t *Lang);

private:
  DicInfo CreateHunspell(wchar_t *Name, int Type);
  bool SpellerCheckWord(DicInfo Dic, char *Word, EncodingType Encoding);
  void MessageBoxWordCannotBeAdded();

public:
private:
  bool IsHunspellWorking;
  bool UseOneDic;
  wchar_t *DicDir;
  wchar_t *SysDicDir;
  std::set<AvailableLangInfo> *DicList;
  std::map<wchar_t *, DicInfo, bool (*)(wchar_t *, wchar_t *)> *AllHunspells;
  char *GetConvertedWord(const char *Source, iconv_t Converter);
  DicInfo SingularSpeller;
  DicInfo LastSelectedSpeller;
  DicInfo Empty;
  std::vector<DicInfo> *Spellers;
  WordSet *Memorized;
  WordSet *Ignored;
  bool InitialReadingBeenDone;
  char *TemporaryBuffer;
  wchar_t *UserDicPath;        // For now only default one.
  wchar_t *SystemWrongDicPath; // Only for reading and then removing
  HWND NppWindow;
};
