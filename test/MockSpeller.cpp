#include "MockSpeller.h"
#include "LanguageInfo.h"

MockSpeller::MockSpeller() = default;
MockSpeller::~MockSpeller() = default;

void MockSpeller::set_language(const wchar_t *lang) { m_current_lang = lang; }

void MockSpeller::add_to_dictionary(const wchar_t *word) {
  switch (m_multi_mode) {
  case 0:
    m_user_dict[m_current_lang].insert(word);
    break;
  case 1:
    for (auto &lang : m_current_multi_lang)
      m_user_dict[lang].insert(word);
    break;
  default:
    break;
  }
}

void MockSpeller::ignore_all(const wchar_t *word) { m_ignored.insert(word); }

bool MockSpeller::is_working() const { return true; }

std::vector<LanguageInfo> MockSpeller::get_language_list() const {
  std::vector<LanguageInfo> ret;
  for (auto &p : m_inner_dict)
    ret.emplace_back(p.first);
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
  switch (m_multi_mode) {
  case 0:
    process_lang(m_current_lang);
    break;
  case 1:
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

bool MockSpeller::check_word(WordForSpeller word) const {
  switch (m_multi_mode) {
  case 0: {
    auto it = m_inner_dict.find(m_current_lang);
    if (it == m_inner_dict.end())
      return true;

    return it->second.find(word.str) != it->second.end();
  }
  case 1: {
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
