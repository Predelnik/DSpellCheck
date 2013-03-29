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
#include "MainDef.h"

class Hunspell;

struct DicInfo
{
  Hunspell *Speller;
  iconv_t Converter;
  iconv_t BackConverter;
  iconv_t ConverterANSI;
  iconv_t BackConverterANSI;
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
private:
  DicInfo CreateHunspell (TCHAR *Name);
public:
private:
  BOOL IsHunspellWorking;
  BOOL SpellerCheckWord (DicInfo Dic, char *Word, EncodingType Encoding);
  TCHAR *DicDir;
  std::vector <TCHAR *> *DicList;
  std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)> *AllHunspells;
  char *GetConvertedWord (const char *Source, iconv_t Converter);
  DicInfo SingularSpeller;
  DicInfo LastSelectedSpeller;
  DicInfo Empty;
  std::vector <DicInfo> *Spellers;
  std::set <char *, bool (*)(char *, char *)> *Ignored;
  char *TemporaryBuffer;
};
#endif // HUNSPELLINTERFACE_H