// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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

#include "NativeSpellerInterface.h"
#include "SpellerInterface.h"

#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "LanguageInfo.h"
#include "MainDef.h"
#include "MappedWString.h"
#include "Plugin.h"
#include "PluginInterface.h"
#include "SciUtils.h"
#include "Scintilla.h"
#include "Settings.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "SpellCheckerHelpers.h"
#include "SpellerContainer.h"
#include "SpellerId.h"
#include "SuggestionsButton.h"
#include "npp/EditorInterface.h"
#include "npp/NppInterface.h"
#include "utils/string_utils.h"

SpellChecker::SpellChecker(const Settings *settings, EditorInterface &editor,
                           const SpellerContainer &speller_container)
    : m_settings(*settings), m_editor(editor),
      m_speller_container(speller_container) {
  m_current_position = 0;
  reset_hot_spot_cache();
  m_settings.settings_changed.connect([this] { on_settings_changed(); });
  m_speller_container.speller_status_changed.connect(
      [this] { recheck_visible_both_views(); });
  on_settings_changed();
}

SpellChecker::~SpellChecker() = default;

void SpellChecker::recheck_visible_both_views() {
  recheck_visible(EditorViewType::primary);
  recheck_visible(EditorViewType::secondary);
}

void SpellChecker::lang_change() { recheck_visible(m_editor.active_view()); }

void SpellChecker::find_next_mistake() {
  auto view = m_editor.active_view();
  m_current_position = m_editor.get_current_pos(view);
  auto cur_line = m_editor.line_from_position(view, m_current_position);
  auto line_start_pos = m_editor.get_line_start_position(view, cur_line);
  auto doc_length = m_editor.get_active_document_length(view);
  auto iterator_pos = line_start_pos;
  bool full_check = false;

  while (true) {
    auto from = static_cast<long>(iterator_pos);
    auto to = static_cast<long>(iterator_pos +
                                m_settings.find_next_buffer_size * 1024);
    int ignore_offsetting = 0;
    if (to > doc_length) {
      ignore_offsetting = 1;
      to = static_cast<long>(doc_length);
    }
    if (from < to) {
      auto text = SpellCheckerHelpers::to_mapped_wstring(
          m_editor, view, m_editor.get_text_range(view, from, to));
      auto index = static_cast<long> (text.str.size());
      if (to != doc_length && next_token_end(text.str, to) == index)
          index = prev_token_begin(text.str, index - 1);
      text.str.erase(index, text.str.size() - index);
      m_editor.force_style_update(view, from, to);
      bool result = check_text(view, text, static_cast<long>(iterator_pos),
                               CheckTextMode::find_first) != 0;
      if (result)
        break;

      iterator_pos += text.to_original_index(index);
    }

    if (to == doc_length) {
      if (!full_check) {
        m_current_position = 0;
        iterator_pos = 0;
        full_check = true;
      } else
        break;

      if (full_check && iterator_pos > m_current_position)
        break; // So nothing was found TODO: Message probably
    }
  }
}

void SpellChecker::find_prev_mistake() {
  auto view = m_editor.active_view();
  m_current_position = m_editor.get_current_pos(view);
  auto cur_line = m_editor.line_from_position(view, m_current_position);
  auto doc_length = m_editor.get_active_document_length(view);
  auto line_end_pos = m_editor.get_line_end_position(view, cur_line);

  auto iterator_pos = line_end_pos;
  bool full_check = false;

  while (true) {
    auto from = static_cast<long>(iterator_pos -
                                  m_settings.find_next_buffer_size * 1024);
    auto to = static_cast<long>(iterator_pos);
    int ignore_offsetting = 0;
    if (from < 0) {
      from = 0;
      ignore_offsetting = 1;
    }

    if (from < to) {
      auto text = SpellCheckerHelpers::to_mapped_wstring(
          m_editor, view, m_editor.get_text_range(view, from, to));
      auto offset = next_token_end(text.str, 0);
      m_editor.force_style_update(view, from + offset, to);
      bool result = check_text(view, text, from, CheckTextMode::find_last) != 0;
      if (result)
        break;

      iterator_pos -= (m_settings.find_next_buffer_size * 1024 -
                       text.to_original_index(offset));
    } else
      --iterator_pos;

    if (iterator_pos < 0) {
      if (!full_check) {
        m_current_position = doc_length + 1;
        iterator_pos = doc_length;
        full_check = true;
      } else
        break;

      if (full_check && iterator_pos < m_current_position - 1)
        break; // So nothing was found TODO: Message probably
    }
  }
}

WordForSpeller SpellChecker::to_word_for_speller(std::wstring_view word) const {
  WordForSpeller res;
  res.data.ends_with_dot = *(word.data() + word.length()) == '.';
  res.str = {word};
  SpellCheckerHelpers::apply_word_conversions(m_settings, res.str);
  return res;
}

std::wstring_view SpellChecker::get_word_at(long char_pos,
                                            const MappedWstring &text,
                                            long offset) const {
  auto index = text.from_original_index(char_pos - offset);
  if (index >= static_cast<long>(text.str.length()))
    index = static_cast<long>(text.str.length()) - 1;
  auto begin = prev_token_begin(text.str, index);
  auto end = next_token_end(text.str, begin);
  return std::wstring_view(text.str).substr(begin, end - begin);
}

void SpellChecker::refresh_underline_style() {
  for (auto view : enum_range<EditorViewType>()) {
    m_editor.set_indicator_style(view, dspellchecker_indicator_id,
                                 m_settings.underline_style);
    m_editor.set_indicator_foreground(view, dspellchecker_indicator_id,
                                      m_settings.underline_color);
  }
}

void SpellChecker::on_settings_changed() {
  refresh_underline_style();
  recheck_visible_both_views();
  update_delimiters();
}

void SpellChecker::create_word_underline(EditorViewType view, long start,
                                         long end) const {
  m_editor.set_current_indicator(view, dspellchecker_indicator_id);
  m_editor.indicator_fill_range(view, start, end);
}

void SpellChecker::remove_underline(EditorViewType view, long start, long end) const {
  if (end < start)
    return;
  m_editor.set_current_indicator(view, dspellchecker_indicator_id);
  m_editor.indicator_clear_range(view, start, end);
}

void SpellChecker::get_visible_limits(EditorViewType view, long &start,
                                      long &finish) {
  auto top = m_editor.get_first_visible_line(view);
  auto bottom = top + m_editor.get_lines_on_screen(view);
  top = m_editor.get_document_line_from_visible(view, top);
  bottom = m_editor.get_document_line_from_visible(view, bottom);
  auto line_count = m_editor.get_document_line_count(view);
  start = m_editor.get_line_start_position(view, top);
  // Not using end of line position cause utf-8 symbols could be more than one
  // char
  // So we use next line start as the end of our visible text
  if (bottom + 1 < line_count) {
    finish = m_editor.get_line_start_position(view, bottom + 1);
  } else {
    finish = m_editor.get_active_document_length(view);
  }
}

MappedWstring SpellChecker::get_visible_text(EditorViewType view, long *offset,
                                             bool /*not_intersection_only*/) {
  long from, to;
  get_visible_limits(view, from, to);

  if (to < 0 || from > to)
    return {};

  *offset = from;

  return SpellCheckerHelpers::to_mapped_wstring(
      m_editor, view, m_editor.get_text_range(view, from, to));
}

void SpellChecker::clear_all_underlines(EditorViewType view) const
{
  auto length = m_editor.get_active_document_length(view);
  if (length > 0) {
    m_editor.set_current_indicator(view, dspellchecker_indicator_id);
    m_editor.indicator_clear_range(view, 0, length);
  }
}

void SpellChecker::reset_hot_spot_cache() {
  memset(m_hot_spot_cache, -1, sizeof(m_hot_spot_cache));
}

bool SpellChecker::is_word_under_cursor_correct(long &pos, long &length,
                                                bool use_text_cursor) const {
  POINT p;
  long init_char_pos;
  LRESULT selection_start = 0;
  LRESULT selection_end = 0;
  auto view = m_editor.active_view();

  if (!use_text_cursor) {
    if (GetCursorPos(&p) == 0)
      return true;

    auto mb_pos = m_editor.char_position_from_point(view, p.x, p.y);
    if (!mb_pos)
      return true;
    init_char_pos = *mb_pos;
  } else {
    selection_start = m_editor.get_selection_start(view);
    selection_end = m_editor.get_selection_end(view);
    init_char_pos = m_editor.get_current_pos(view);
  }

  if (init_char_pos == -1)
    return true;
  auto line = m_editor.line_from_position(view, init_char_pos);
  auto offset = m_editor.get_line_start_position(view, line);
  auto mapped_str = SpellCheckerHelpers::to_mapped_wstring(
      m_editor, view, m_editor.get_line(view, line).data());
  if (mapped_str.str.empty())
    return true;
  auto word = get_word_at(static_cast<long>(init_char_pos), mapped_str,
                          static_cast<long>(offset));
  if (word.empty())
    return true;
  SpellCheckerHelpers::cut_apostrophes(m_settings, word);
  pos = static_cast<long>(mapped_str.to_original_index(static_cast<long>(
                              word.data() - mapped_str.str.data())) +
                          offset);
  long pos_end = mapped_str.to_original_index(static_cast<long>(
                     word.data() + word.length() - mapped_str.str.data())) +
                 offset;
  long word_len = pos_end - pos;
  if (selection_start != selection_end &&
      (selection_start != pos || selection_end != pos + word_len))
    return true;
  if (check_word(view, word, pos)) {
    return true;
  }
  length = word_len;
  return false;
}

bool SpellChecker::is_spellchecking_needed(EditorViewType view,
                                           std::wstring_view word,
                                           long word_start) const {
  if (!m_speller_container.active_speller().is_working() || word.empty()) {
    return false;
  }
  // Well Numbers have same codes for ANSI and Unicode I guess, so
  // If word contains number then it's probably just a number or some crazy name
  auto style = m_editor.get_style_at(view, word_start);
  auto lexer = m_editor.get_lexer(view);
  auto category = SciUtils::get_style_category(lexer, style);
  if (category == SciUtils::StyleCategory::unknown) {
    return false;
  }

  if (category != SciUtils::StyleCategory::text &&
      !((category == SciUtils::StyleCategory::comment &&
         m_settings.check_comments) ||
        (category == SciUtils::StyleCategory::string &&
         m_settings.check_strings) ||
        (category == SciUtils::StyleCategory::identifier &&
         m_settings.check_variable_functions))) {
    return false;
  }

  if (m_editor.is_style_hotspot(view, style)) {
    return false;
  }

  if (static_cast<int>(word.length()) < m_settings.word_minimum_length)
    return false;

  if (m_settings.ignore_one_letter && word.length() == 1)
    return false;

  if (m_settings.ignore_containing_digit &&
      word.find_first_of(L"0123456789") != std::wstring_view::npos) {
    return false;
  }

  if (m_settings.ignore_starting_with_capital && IsCharUpper(word.front())) {
    return false;
  }

  if (m_settings.ignore_having_a_capital || m_settings.ignore_all_capital) {
    if (m_settings.ignore_having_a_capital || m_settings.ignore_all_capital) {
      bool all_upper = IsCharUpper(word.front()), any_upper = false;
      for (auto c : std::wstring_view(word).substr(1)) {
        if (IsCharUpper(c)) {
          any_upper = true;
        } else
          all_upper = false;
      }

      if (!all_upper && any_upper && m_settings.ignore_having_a_capital)
        return false;

      if (all_upper && m_settings.ignore_all_capital)
        return false;
    }
  }

  if (m_settings.ignore_having_underscore &&
      word.find(L'_') != std::wstring_view::npos)
    return false;

  if (m_settings.ignore_starting_or_ending_with_apostrophe) {
    if (word.front() == '\'' || word.back() == '\'')
      return false;
  }

  return true;
}

bool SpellChecker::check_word(EditorViewType view, std::wstring_view word,
                              long word_start) const {
  if (!is_spellchecking_needed(view, word, word_start))
    return true;

  return m_speller_container.active_speller().check_word(
      to_word_for_speller(word));
}

auto SpellChecker::non_alphabetic_tokenizer(std::wstring_view target) const {
  return make_condition_tokenizer(target,
                                  [this](wchar_t c) {
                                    return !IsCharAlphaNumeric(c) &&
                                           m_settings.delimiter_exclusions.find(
                                               c) == std ::wstring_view::npos;
                                  },
                                  m_settings.split_camel_case);
}

auto SpellChecker::delimiter_tokenizer(std::wstring_view target) const {
  return make_delimiter_tokenizer(target, m_delimiters,
                                  m_settings.split_camel_case);
}

long SpellChecker::next_token_end(std::wstring_view target,
                                  long index) const {
  switch (m_settings.tokenization_style) {
  case TokenizationStyle::by_non_alphabetic:
    return non_alphabetic_tokenizer(target).next_token_end(index);
  case TokenizationStyle::by_delimiters:
    return delimiter_tokenizer(target).next_token_end(index);
  case TokenizationStyle::COUNT:
    break;
  }
  assert(false);
  return static_cast<long> (target.size ());
}

long SpellChecker::prev_token_begin(std::wstring_view target,
                                    long index) const {
  switch (m_settings.tokenization_style) {
  case TokenizationStyle::by_non_alphabetic:
    return non_alphabetic_tokenizer(target).prev_token_begin(index);
  case TokenizationStyle::by_delimiters:
    return delimiter_tokenizer(target).prev_token_begin(index);
  case TokenizationStyle::COUNT:
    break;
  }
  assert(false);
  return 0;
}

namespace {
struct WordData {
  std::wstring_view token;
  WordForSpeller word_for_speller;
  long word_start;
  long word_end;
  bool is_correct;
};
} // namespace

int SpellChecker::check_text(EditorViewType view,
                             const MappedWstring &text_to_check, long offset,
                             CheckTextMode mode) const {
  if (text_to_check.str.empty())
    return 0;

  bool stop = false;
  long resulting_word_end = -1, resulting_word_start = -1;
  auto text_len = text_to_check.original_length();
  std::vector<long> underline_buffer;

  auto sv = std::wstring_view(text_to_check.str);
  std::vector<std::wstring_view> tokens;
  switch (m_settings.tokenization_style) {
  case TokenizationStyle::by_non_alphabetic:
    tokens = non_alphabetic_tokenizer(sv).get_all_tokens();
    break;
  case TokenizationStyle::by_delimiters:
    tokens = delimiter_tokenizer(sv).get_all_tokens();
    break;
  case TokenizationStyle::COUNT:
    break;
  }

  std::vector<bool> results(tokens.size());
  std::vector<WordData> words_to_check;
  words_to_check.clear();
  std::vector<WordForSpeller> words_for_speller;
  for (auto token : tokens) {
    SpellCheckerHelpers::cut_apostrophes(m_settings, token);
    auto word_start = static_cast<long>(
        offset + text_to_check.to_original_index(static_cast<long>(
                     token.data() - text_to_check.str.data())));
    auto word_end = static_cast<long>(
        offset +
        text_to_check.to_original_index(static_cast<long>(
            token.data() - text_to_check.str.data() + token.length())));
    if (is_spellchecking_needed(view, token, word_start)) {
      words_to_check.emplace_back();
      auto &w = words_to_check.back();
      w.word_for_speller = to_word_for_speller(token);
      w.word_start = word_start;
      w.word_end = word_end;
      w.token = token;
    }
  }
  words_for_speller.resize(words_to_check.size());
  std::transform(words_to_check.begin(), words_to_check.end(),
                 words_for_speller.begin(), [](auto &word) -> auto && {
                   return std::move(word.word_for_speller);
                 });
  auto spellcheck_result =
      m_speller_container.active_speller().check_words(words_for_speller);
  if (!spellcheck_result.empty()) {
    for (int i = 0; i < static_cast<int>(words_for_speller.size()); ++i)
      words_to_check[i].is_correct = spellcheck_result[i];
  } else
    for (auto &w : words_to_check)
      w.is_correct = true;

  for (auto &result : words_to_check) {
    if (!result.is_correct) {
      auto word_start = result.word_start;
      auto word_end = result.word_end;
      switch (mode) {
      case CheckTextMode::underline_errors:
        underline_buffer.push_back(word_start);
        underline_buffer.push_back(word_end);
        break;
      case CheckTextMode::find_first:
        if (word_end > m_current_position) {
          m_editor.set_selection(view, word_start, word_end);
          stop = true;
        }
        break;
      case CheckTextMode::find_last: {
        if (word_end >= m_current_position) {
          stop = true;
          break;
        }
        resulting_word_start = word_start;
        resulting_word_end = word_end;
      } break;
      case CheckTextMode::find_all:
        m_misspellings.push_back(result.token);
        break;
      }
      if (stop)
        break;
    }
  }

  if (mode == CheckTextMode::underline_errors) {
    long prev_pos = offset;
    for (long i = 0; i < static_cast<long>(underline_buffer.size()) - 1;
         i += 2) {
      remove_underline(view, prev_pos, underline_buffer[i]);
      create_word_underline(view, underline_buffer[i],
                            underline_buffer[i + 1]);
      prev_pos = underline_buffer[i + 1];
    }
    remove_underline(view, prev_pos, offset + static_cast<long>(text_len));
  }
  switch (mode) {
  case CheckTextMode::underline_errors:
    return 1;
  case CheckTextMode::find_first:
    return static_cast<int>(stop);
  case CheckTextMode::find_all:
    return 1;
  case CheckTextMode::find_last:
    if (resulting_word_start == -1)
      return 0;
    else {
      m_editor.set_selection(view, resulting_word_start, resulting_word_end);
      return 1;
    }
  };
  return 0;
}

void SpellChecker::check_visible(EditorViewType view,
                                 bool not_intersection_only) {
  long offset;
  auto text = get_visible_text(view, &offset, not_intersection_only);
  check_text(view, text, offset, CheckTextMode::underline_errors);
}

void SpellChecker::recheck_visible(EditorViewType view,
                                   bool not_intersection_only) {
  if (!m_speller_container.active_speller().is_working()) {
    clear_all_underlines(view);
    return;
  }

  // to utf-8 or no
  if (SpellCheckerHelpers::is_spell_checking_needed(m_editor, m_settings))
    check_visible(view, not_intersection_only);
  else
    clear_all_underlines(view);
}

std::wstring SpellChecker::get_all_misspellings_as_string() const {
  auto view = m_editor.active_view();
  auto buf = m_editor.get_active_document_text(view);
  auto mapped_str =
      SpellCheckerHelpers::to_mapped_wstring(m_editor, view, buf.data());
  m_misspellings.clear();
  check_text(view, mapped_str, 0, CheckTextMode::find_all);
  std::sort(m_misspellings.begin(), m_misspellings.end(),
            [](const auto &lhs, const auto &rhs) {
              return std::lexicographical_compare(
                  lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                  [](wchar_t lhs, wchar_t rhs) {
                    return CharUpper(reinterpret_cast<LPWSTR>(lhs)) <
                           CharUpper(reinterpret_cast<LPWSTR>(rhs));
                  });
            });
  auto it = std::unique(
      m_misspellings.begin(), m_misspellings.end(),
      [](const auto &lhs, const auto &rhs) {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                          [](wchar_t lhs, wchar_t rhs) {
                            return CharUpper(reinterpret_cast<LPWSTR>(lhs)) ==
                                   CharUpper(reinterpret_cast<LPWSTR>(rhs));
                          });
      });
  m_misspellings.erase(it, m_misspellings.end());
  std::wstring str;
  for (auto &s : m_misspellings)
    str += std::wstring{s} + L'\n';
  m_misspellings.clear();
  return str;
}

void SpellChecker::update_delimiters() {
  m_delimiters = L" \n\r\t\v" + parse_string(m_settings.delimiters.c_str());
}
