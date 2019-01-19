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

#pragma once
#include <vector>
#include <string>

class LanguageInfo;

class AdditionalWordData {
public:
  bool ends_with_dot = false; // could be useful for words like etc.
};

class WordForSpeller {
public:
  std::wstring str;
  AdditionalWordData data;
};

class SpellerInterface {

public:
  enum class SpellerMode {
    SingleLanguage,
    MultipleLanguages,
  };

public:
  virtual ~SpellerInterface() = default;
  virtual std::vector<LanguageInfo> get_language_list() const = 0;
  virtual void set_language(const wchar_t *lang) = 0;
  virtual void set_multiple_languages(
      const std::vector<std::wstring> &list) = 0; // Languages are from LangList
  void set_mode(SpellerMode multi) { m_speller_mode = multi; }
  // Implement either check_word or check_words or get the endless recursion
  virtual bool check_word(WordForSpeller word) const;
  // Functions which should be implemented in case if words for some awkward
  // reason could be faster checked in bulk
  virtual std::vector<bool>
  check_words(const std::vector<WordForSpeller> &words) const;
  virtual std::vector<std::wstring>
  get_suggestions(const wchar_t *word) const = 0;
  virtual void add_to_dictionary(const wchar_t *word) = 0;
  virtual void ignore_all(const wchar_t *word) = 0;
  virtual bool is_working() const = 0;

protected:
  SpellerMode m_speller_mode = SpellerMode::SingleLanguage;
};

// speller that does not do anything and never works
class DummySpeller : public SpellerInterface {
public:
  std::vector<LanguageInfo> get_language_list() const override;
  void set_language(const wchar_t *lang) override;

  void set_multiple_languages(const std::vector<std::wstring> &langs) override;

  bool check_word(WordForSpeller word) const override;
  std::vector<std::wstring> get_suggestions(const wchar_t *word) const override;
  void add_to_dictionary(const wchar_t *word) override;
  void ignore_all(const wchar_t *word) override;
  bool is_working() const override;
};
