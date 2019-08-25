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
#include "SpellerInterface.h"

#include <unordered_map>
#include <unordered_set>

class Settings;

class MockSpeller : public SpellerInterface {
public:
  using Dict =
      std::unordered_map<std::wstring, std::unordered_set<std::wstring>>;
  using SuggestionsDict = std::unordered_map<
      std::wstring,
      std::unordered_map<std::wstring, std::vector<std::wstring>>>;
  ;
  MockSpeller(const Settings &settings);
  virtual ~MockSpeller();
  void set_language(const wchar_t *lang) override;
  void add_to_dictionary(const wchar_t *word) override;
  void ignore_all(const wchar_t *word) override;
  bool is_working() const override;
  std::vector<LanguageInfo> get_language_list() const override;
  void set_multiple_languages(const std::vector<std::wstring> &list) override;
  std::vector<std::wstring> get_suggestions(const wchar_t *word) const override;

  void set_inner_dict(const Dict &dict);
  void set_suggestions_dict(const SuggestionsDict &dict);

  bool check_word(const WordForSpeller& word) const override;
  void set_working(bool working);

private:
  std::wstring m_current_lang;
  std::unordered_set<std::wstring> m_ignored;
  std::vector<std::wstring> m_current_multi_lang;
  Dict m_user_dict;
  Dict m_inner_dict;
  SuggestionsDict m_sugg_dict;
  bool m_working = true;
  const Settings &m_settings;
};
