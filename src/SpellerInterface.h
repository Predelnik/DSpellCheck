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
#include <MainDef.h>

class LanguageInfo;

class SpellerInterface {
private:
protected:
public:
  virtual ~SpellerInterface() = default;
  virtual std::vector<LanguageInfo> get_language_list() const = 0;
  virtual void set_language(const wchar_t* lang) = 0;
  virtual void set_multiple_languages(
  const std::vector<std::wstring>& list) = 0;         // Languages are from LangList
  void set_mode(int multi) { m_multi_mode = multi; } // Multi - 1, Single - 0
  void set_encoding(EncodingType encoding) { m_current_encoding = encoding; }
  virtual bool check_word(const wchar_t* word) = 0; // Word in Utf-8 or ANSI
  // returned suggestions should be in utf-8
  virtual std::vector<std::wstring> get_suggestions(const wchar_t* word) = 0;
  virtual void add_to_dictionary(const wchar_t* word) = 0;
  virtual void ignore_all(const wchar_t* word) = 0;
  virtual bool is_working() const = 0;

protected:
  int m_multi_mode = 0;
  EncodingType m_current_encoding = EncodingType::utf8;
};
