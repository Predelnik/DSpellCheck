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

struct AspellCanHaveError;
struct AspellSpeller;

#include "AbstractSpellerInterface.h"

class AspellInterface : public AbstractSpellerInterface {
public:
  AspellInterface(HWND NppWindowArg); // Window for error reporting
  ~AspellInterface();
  std::vector<wchar_t *> *GetLanguageList() override;
  void SetLanguage(wchar_t *Lang) override;
  void SetMultipleLanguages(
      std::vector<wchar_t *> *List) override; // Languages are from LangList
  bool
  CheckWord(char *Word) override; // Word in Utf-8 or ANSI (For now only Utf-8)
  bool IsWorking() override;
  std::vector<char *> *GetSuggestions(char *Word) override;
  void AddToDictionary(char *Word) override;
  void IgnoreAll(char *Word) override;

  bool Init(wchar_t *PathArg);

private:
  void SendAspellErorr(AspellCanHaveError *Error);

public:
private:
  AspellSpeller *LastSelectedSpeller;
  AspellSpeller *SingularSpeller;
  std::vector<AspellSpeller *> *Spellers;
  bool AspellLoaded;
  HWND NppWindow; // For message boxes
};
