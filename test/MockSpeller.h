#pragma once
#include "SpellerInterface.h"

#include <unordered_map>
#include <unordered_set>

class MockSpeller : public SpellerInterface {
public:
  using Dict =
      std::unordered_map<std::wstring, std::unordered_set<std::wstring>>;
  using SuggestionsDict = std::unordered_map<
      std::wstring,
      std::unordered_map<std::wstring, std::vector<std::wstring>>>;
  ;
  MockSpeller();
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

  bool check_word(WordForSpeller word) const override;
  void set_working(bool working);

private:
  std::wstring m_current_lang;
  std::unordered_set<std::wstring> m_ignored;
  std::vector<std::wstring> m_current_multi_lang;
  Dict m_user_dict;
  Dict m_inner_dict;
  SuggestionsDict m_sugg_dict;
  bool m_working = true;
};
