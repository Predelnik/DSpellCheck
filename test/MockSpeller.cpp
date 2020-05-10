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

#include "MockSpeller.h"
#include "LanguageInfo.h"
#include "Settings.h"

MockSpeller::MockSpeller(const Settings &settings) : m_settings (settings) {}
MockSpeller::~MockSpeller() = default;

void MockSpeller::set_language(const wchar_t *lang) { m_current_lang = lang; }

void MockSpeller::add_to_dictionary(const wchar_t *word) {
  switch (m_speller_mode) {
  case SpellerMode::SingleLanguage:
    m_user_dict[m_current_lang].insert(word);
    break;
  case SpellerMode::MultipleLanguages:
    for (auto &lang : m_current_multi_lang)
      m_user_dict[lang].insert(word);
    break;
  }
}

void MockSpeller::ignore_all(const wchar_t *word) { m_ignored.insert(word); }

bool MockSpeller::is_working() const { return m_working; }

std::vector<LanguageInfo> MockSpeller::get_language_list() const {
  std::vector<LanguageInfo> ret;
  for (auto &p : m_inner_dict)
    ret.emplace_back(p.first, m_settings.data.language_name_style);
  return ret;
}

void MockSpeller::set_multiple_languages(
    const std::vector<std::wstring> &list) {
  m_current_multi_lang = list;
}

std::vector<std::wstring>
MockSpeller::get_suggestions(const wchar_t *word) const {
  std::vector<std::vector<std::wstring>> v;
  auto process_lang = [&](const std::wstring &lang) {
    auto it = m_sugg_dict.find(lang);
    if (it == m_sugg_dict.end())
      return;
    auto jt = it->second.find(word);
    if (jt == it->second.end())
      return;
    v.push_back(jt->second);
  };
  switch (m_speller_mode) {
  case SpellerMode::SingleLanguage:
    process_lang(m_current_lang);
    break;
  case SpellerMode::MultipleLanguages:
    for (auto &lang : m_current_multi_lang)
      process_lang(lang);
    break;
  }
  std::vector<std::wstring> ans;
  for (auto &l : v)
    if (l.size() > ans.size()) {
      ans = std::move(l);
    }
  return ans;
}

void MockSpeller::set_inner_dict(const Dict &dict) { m_inner_dict = dict; }

void MockSpeller::set_suggestions_dict(const SuggestionsDict &dict) {
  m_sugg_dict = dict;
}

bool MockSpeller::check_word(const WordForSpeller& word) const {
  switch (m_speller_mode) {
  case SpellerMode::SingleLanguage: {
    auto it = m_inner_dict.find(m_current_lang);
    if (it == m_inner_dict.end())
      return true;

    return it->second.find(word.str) != it->second.end();
  }
  case SpellerMode::MultipleLanguages: {
    for (auto &lang : m_current_multi_lang) {
      auto it = m_inner_dict.find(lang);
      if (it == m_inner_dict.end())
        continue;

      return it->second.find(word.str) != it->second.end();
    }
    break;
  }
  }
  return false;
}

void MockSpeller::set_working(bool working) { m_working = working; }
