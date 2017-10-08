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
class AbstractSpellerInterface {
private:
protected:
public:
  virtual ~AbstractSpellerInterface() = default;
  virtual std::vector<std::wstring> get_language_list() = 0;
  virtual void set_language(const wchar_t* Lang) = 0;
  virtual void set_multiple_languages(
  const std::vector<std::wstring>& List) = 0;         // Languages are from LangList
  void set_mode(int Multi) { m_multi_mode = Multi; } // Multi - 1, Single - 0
  void set_encoding(EncodingType Encoding) { m_current_encoding = Encoding; }
  virtual bool check_word(const char* Word) = 0; // Word in Utf-8 or ANSI
  virtual std::vector<std::string> get_suggestions(const char* Word) = 0;
  virtual void add_to_dictionary(const char* Word) = 0;
  virtual void ignore_all(const char* Word) = 0;
  virtual bool is_working() = 0;

private:
protected:
  int m_multi_mode = 0;
  EncodingType m_current_encoding = ENCODING_UTF8;

public:
};
