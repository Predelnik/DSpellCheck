// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "SpellerInterface.h"
#include "LanguageInfo.h"

bool SpellerInterface::check_word(const WordForSpeller& word) const {
  return check_words({std::move (word)}).front();
}

std::vector<bool>
SpellerInterface::check_words(const std::vector<WordForSpeller> &words) const {
  std::vector<bool> ret;
  ret.resize(words.size());
  for (int i = 0; i < static_cast<int>(words.size()); ++i)
    ret[i] = check_word(words[i]);
  return ret;
}

std::vector<LanguageInfo> DummySpeller::get_language_list() const { return {}; }

void DummySpeller::set_language(const wchar_t * /*lang*/) {}

void DummySpeller::set_multiple_languages(
    const std::vector<std::wstring> & /*langs*/) {}

bool DummySpeller::check_word(const WordForSpeller
    /*word*/&) const { return true; }

std::vector<std::wstring>
DummySpeller::get_suggestions(const wchar_t * /*word*/) const {
  return {};
}

void DummySpeller::add_to_dictionary(const wchar_t * /*word*/) {}

void DummySpeller::ignore_all(const wchar_t * /*word*/) {}

bool DummySpeller::is_working() const { return false; }
