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

using WordSet = std::unordered_set<std::string>;
using SortedWordSet = std::set<std::string>;
class iconv_wrapper_t {
    static void close_iconv (iconv_t conv)
    {
      if (conv != reinterpret_cast<iconv_t>(-1))
        iconv_close (conv);
    }

public:
    template <typename... ArgTypes>
    iconv_wrapper_t (ArgTypes &&... args) :
      conv (iconv_open (std::forward<ArgTypes> (args)...), &close_iconv)
    {
    }
    iconv_t get () const { return conv.get (); }
    iconv_wrapper_t () : conv (nullptr, &close_iconv) {}
    iconv_wrapper_t (iconv_wrapper_t &&) = default;
    iconv_wrapper_t &operator= (iconv_wrapper_t &&) = default;

    ~iconv_wrapper_t () {
    }

private:
    std::unique_ptr<void, void(*)(iconv_t)> conv;
};

struct DicInfo {
  std::unique_ptr<Hunspell> hunspell;
  iconv_wrapper_t Converter;
  iconv_wrapper_t BackConverter;
  iconv_wrapper_t ConverterANSI;
  iconv_wrapper_t BackConverterANSI;
  std::wstring LocalDicPath;
  WordSet LocalDic; // Stored in Dictionary encoding
};

struct AvailableLangInfo {
  std::wstring name;
  int type; // Type = 1 - System Dir Dictionary, 0 - Nomal Dictionary

  bool operator<(const AvailableLangInfo &rhs) const {
    return name < rhs.name;
  }
};

class HunspellInterface : public AbstractSpellerInterface {
public:
  HunspellInterface(HWND NppWindowArg);
  ~HunspellInterface();
  std::vector<std::wstring> GetLanguageList() override;
  void SetLanguage(const wchar_t* Lang) override;
  void SetMultipleLanguages(
      const std::vector<std::wstring>& List) override;             // Languages are from LangList
  bool CheckWord(const char* Word) override; // Word in Utf-8 or ANSI
  bool IsWorking() override;
  std::vector<std::string> GetSuggestions(const char* Word) override;
  void AddToDictionary(const char* Word) override;
  void IgnoreAll(const char* Word) override;

  void SetDirectory(const wchar_t* Dir);
  void SetAdditionalDirectory(const wchar_t* Dir);
  void ReadUserDic(WordSet *Target, const wchar_t* Path);
  void SetUseOneDic(bool Value);
  void UpdateOnDicRemoval(wchar_t *Path, bool &NeedSingleLangReset,
                          bool &NeedMultiLangReset);
  bool GetLangOnlySystem(const wchar_t* Lang);

private:
    DicInfo* CreateHunspell(const wchar_t* Name, int Type);
  bool SpellerCheckWord(const DicInfo& Dic, const char* Word, EncodingType Encoding);
  void MessageBoxWordCannotBeAdded();

private:
  bool IsHunspellWorking;
  bool UseOneDic;
  std::wstring DicDir;
  std::wstring SysDicDir;
  std::set<AvailableLangInfo> dicList;
  std::map<std::wstring, DicInfo> AllHunspells;
  char *GetConvertedWord(const char *Source, iconv_t Converter);
  DicInfo *SingularSpeller;
  DicInfo *LastSelectedSpeller;
  std::vector<DicInfo *> m_spellers;
  WordSet Memorized;
  WordSet Ignored;
  bool InitialReadingBeenDone;
  std::wstring UserDicPath;        // For now only default one.
  std::wstring SystemWrongDicPath; // Only for reading and then removing
  HWND NppWindow;
};
