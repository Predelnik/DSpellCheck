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
#ifndef HUNSPELLINTERFACE_H
#define HUNSPELLINTERFACE_H

#include "AbstractSpellerInterface.h"

#ifndef USING_STATIC_LIBICONV
#define USING_STATIC_LIBICONV
#endif // !USING_STATIC_LIBICONV

#include "iconv.h"
#include "CommonFunctions.h"
#include "MainDef.h"

class Hunspell;

typedef std::unordered_set <char *, size_t (*)(char *), bool (*)(char *, char *)> WordSet;
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
  TCHAR *Name;
  int Type; // Type = 1 - System Dir Dictionary, 0 - Nomal Dictionary

  bool operator < (const AvailableLangInfo &rhs ) const
  {
    return (_tcscmp (Name, rhs.Name) < 0);
  }
};

class HunspellInterface : public AbstractSpellerInterface
{
public:
  HunspellInterface ();
  ~HunspellInterface ();
  __override virtual std::vector<TCHAR*> *GetLanguageList ();
  __override virtual void SetLanguage (TCHAR *Lang);
  __override virtual void SetMultipleLanguages (std::vector<TCHAR *> *List); // Languages are from LangList
  __override virtual BOOL CheckWord (char *Word); // Word in Utf-8 or ANSI
  __override virtual BOOL IsWorking ();
  __override virtual std::vector<char *> *GetSuggestions (char *Word);
  __override virtual void AddToDictionary (char *Word);
  __override virtual void IgnoreAll (char *Word);

  void SetDirectory (TCHAR *Dir);
  void SetAdditionalDirectory (TCHAR *Dir);
  void WriteUserDic (WordSet *Target, TCHAR *Path);
  void ReadUserDic (WordSet *Target, TCHAR *Path);
  void SetUseOneDic (BOOL Value);
  void UpdateOnDicRemoval (TCHAR *Path);
  BOOL GetLangOnlySystem (TCHAR *Lang);
private:
  DicInfo CreateHunspell (TCHAR *Name, int Type);
  BOOL SpellerCheckWord (DicInfo Dic, char *Word, EncodingType Encoding);
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
};
#endif // HUNSPELLINTERFACE_H