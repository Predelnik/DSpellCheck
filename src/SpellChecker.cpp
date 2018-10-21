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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA.

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

#include <cctype>

#include <experimental/string>

SpellChecker::SpellChecker(const Settings *settings, EditorInterface &editor, const SpellerContainer &speller_container)
    : m_settings(*settings), m_editor(editor), m_speller_container(speller_container) {
  m_current_position = 0;
  reset_hot_spot_cache();
  m_settings.settings_changed.connect([this] { on_settings_changed(); });
  m_speller_container.speller_status_changed.connect([this] { recheck_visible_both_views(); });
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
  auto doc_length = m_editor.get_active_document_length(view);
  auto iterator_pos = prev_token_begin_in_document(view, m_current_position);
  bool full_check = false;

  while (true) {
    auto from = static_cast<long>(iterator_pos);
    auto to = static_cast<long>(iterator_pos + 4096);
    int ignore_offsetting = 0;
    if (to > doc_length) {
      ignore_offsetting = 1;
      to = static_cast<long>(doc_length);
    }
    if (from < to) {
      auto text = SpellCheckerHelpers::to_mapped_wstring(m_editor, view, m_editor.get_text_range(view, from, to));
      auto index = static_cast<long>(text.str.size());
      if (to != doc_length && next_token_end(text.str, to) == index)
        index = prev_token_begin(text.str, index - 1);
      text.str.erase(index, text.str.size() - index);
      if (check_text(view, text, static_cast<long>(iterator_pos), CheckTextMode::find_first))
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
  auto doc_length = m_editor.get_active_document_length(view);

  auto iterator_pos = next_token_end_in_document(view, m_current_position);
  bool full_check = false;

  while (true) {
    auto from = static_cast<long>(iterator_pos - 4096);
    auto to = static_cast<long>(iterator_pos);
    int ignore_offsetting = 0;
    if (from < 0) {
      from = 0;
      ignore_offsetting = 1;
    }

    if (from < to) {
      auto text = SpellCheckerHelpers::to_mapped_wstring(m_editor, view, m_editor.get_text_range(view, from, to));
      auto offset = next_token_end(text.str, 0);
      if (check_text(view, text, from, CheckTextMode::find_last))
        break;

      iterator_pos -= (4096 - text.to_original_index(offset));
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
  res.str = word;
  SpellCheckerHelpers::apply_word_conversions(m_settings, res.str);
  return res;
}

std::wstring_view SpellChecker::get_word_at(long char_pos, const MappedWstring &text, long offset) const {
  auto index = text.from_original_index(char_pos - offset);
  if (index >= static_cast<long>(text.str.length()))
    index = static_cast<long>(text.str.length()) - 1;
  auto begin = prev_token_begin(text.str, index);
  auto end = next_token_end(text.str, begin);
  return std::wstring_view(text.str).substr(begin, end - begin);
}

void SpellChecker::refresh_underline_style() {
  for (auto view : enum_range<EditorViewType>()) {
    m_editor.set_indicator_style(view, dspellchecker_indicator_id, m_settings.underline_style);
    m_editor.set_indicator_foreground(view, dspellchecker_indicator_id, m_settings.underline_color);
  }
}

void SpellChecker::on_settings_changed() {
  refresh_underline_style();
  update_delimiters();
  recheck_visible_both_views();
}

void SpellChecker::create_word_underline(EditorViewType view, long start, long end) const {
  m_editor.set_current_indicator(view, dspellchecker_indicator_id);
  m_editor.indicator_fill_range(view, start, end);
}

void SpellChecker::remove_underline(EditorViewType view, long start, long end) const {
  if (end < start)
    return;
  m_editor.set_current_indicator(view, dspellchecker_indicator_id);
  m_editor.indicator_clear_range(view, start, end);
}

long SpellChecker::prev_token_begin_in_document(EditorViewType view, long start) const {
  long shift = 15;
  auto prev_start = start + 1;
  while (start > 0) {
    start = std::max(start - shift, 0l);
    auto mapped_str = SpellCheckerHelpers::to_mapped_wstring(m_editor, view, m_editor.get_text_range(view, start, prev_start));
    // finding any start before start which starts a token
    auto index = prev_token_begin(mapped_str.str, static_cast<long>(mapped_str.str.length()) - 1);
    if (index > 0)
      return start + mapped_str.to_original_index(index);
    prev_start = start;
    shift *= 2;
  }
  return start;
}

long SpellChecker::next_token_end_in_document(EditorViewType view, long end) const {
  long shift = 15;
  auto prev_end = end;
  auto length = m_editor.get_active_document_length(view);
  if (end == length)
    return end;
  while (end > 0) {
    end = std::min(end + shift, length);
    auto mapped_str = SpellCheckerHelpers::to_mapped_wstring(m_editor, view, m_editor.get_text_range(view, prev_end, end));
    // finding any start before start which starts a token
    auto index = next_token_end(mapped_str.str, 0);
    if (index < static_cast<long>(mapped_str.str.length()))
      return prev_end + mapped_str.to_original_index(index);
    if (end == length)
      return end;
    prev_end = end;
    shift *= 2;
  }
  return end;
}

MappedWstring SpellChecker::get_visible_text(EditorViewType view) {

  auto top_visible_line = m_editor.get_first_visible_line(view);
  auto top_visible_line_index = m_editor.get_document_line_from_visible(view, top_visible_line);
  auto bottom_visible_line_index = m_editor.get_document_line_from_visible(view, top_visible_line + m_editor.get_lines_on_screen(view) - 1);
  auto rect = m_editor.editor_rect(view);
  auto len = m_editor.get_active_document_length(view);
  MappedWstring result;
  for (auto line = top_visible_line_index; line <= bottom_visible_line_index; ++line) {
    auto start = m_editor.get_line_start_position(view, line);
    if (start >= len) // skipping possible empty lines when document is too short
      continue;
    auto start_point = m_editor.get_point_from_position(view, start);
    if (start_point.y < rect.top) {
      start = m_editor.char_position_from_point(view, {0, 0});
      start = prev_token_begin_in_document(view, start);
    } else if (start_point.x < rect.left) {
      start = m_editor.char_position_from_point(view, {0, start_point.y});
      start = prev_token_begin_in_document(view, start);
    }
    auto end = m_editor.get_line_end_position(view, line);
    auto end_point = m_editor.get_point_from_position(view, end);
    if (end_point.y > rect.bottom) {
      end = m_editor.char_position_from_point(view, {rect.right, rect.bottom});
      end = next_token_end_in_document(view, end);
    } else if (end_point.x > rect.right) {
      end = m_editor.char_position_from_point(view, {rect.right, end_point.y});
      end = next_token_end_in_document(view, end);
    }
    auto new_str = get_document_mapped_wstring(view, start, end);
    result.append(new_str);
  }
  return result;
}

void SpellChecker::clear_all_underlines(EditorViewType view) const {
  auto length = m_editor.get_active_document_length(view);
  if (length > 0) {
    m_editor.set_current_indicator(view, dspellchecker_indicator_id);
    m_editor.indicator_clear_range(view, 0, length);
  }
}

void SpellChecker::reset_hot_spot_cache() { memset(m_hot_spot_cache, -1, sizeof(m_hot_spot_cache)); }

bool SpellChecker::is_word_under_cursor_correct(long &pos, long &length, bool use_text_cursor) const {
  POINT p;
  long init_char_pos;
  LRESULT selection_start = 0;
  LRESULT selection_end = 0;
  auto view = m_editor.active_view();
  length = 0;
  pos = -1;

  if (!use_text_cursor) {
    if (GetCursorPos(&p) == 0)
      return true;

    auto mb_pos = m_editor.char_position_from_global_point(view, p.x, p.y);
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
  auto mapped_str = SpellCheckerHelpers::to_mapped_wstring(m_editor, view, m_editor.get_line(view, line).data());
  if (mapped_str.str.empty())
    return true;
  auto word = get_word_at(static_cast<long>(init_char_pos), mapped_str, static_cast<long>(offset));
  if (word.empty())
    return true;
  SpellCheckerHelpers::cut_apostrophes(m_settings, word);
  pos = static_cast<long>(mapped_str.to_original_index(static_cast<long>(word.data() - mapped_str.str.data())) + offset);
  long pos_end = mapped_str.to_original_index(static_cast<long>(word.data() + word.length() - mapped_str.str.data())) + offset;
  long word_len = pos_end - pos;
  if (selection_start != selection_end && (selection_start != pos || selection_end != pos + word_len))
    return true;
  if (check_word(view, word, pos)) {
    return true;
  }
  length = word_len;
  return false;
}

void SpellChecker::erase_all_misspellings() {
  auto view = m_editor.active_view();
  auto buf = m_editor.get_active_document_text(view);
  auto mapped_str = SpellCheckerHelpers::to_mapped_wstring(m_editor, view, buf.data());
  m_misspellings.clear();
  if (!check_text(view, mapped_str, 0, CheckTextMode::find_all))
    return;

  m_editor.begin_undo_action(view);
  auto chars_removed = 0l;
  for (auto &misspelling : m_misspellings) {
    auto start = mapped_str.to_original_index (static_cast<long> (misspelling.data() - mapped_str.str.data()));
    auto original_len = mapped_str.to_original_index (static_cast<long> (misspelling.data() - mapped_str.str.data() + misspelling.length ())) - start;
    m_editor.delete_range(view, start - chars_removed, original_len);
    chars_removed += original_len;
  }
  m_editor.end_undo_action(view);
}

bool SpellChecker::is_spellchecking_needed(EditorViewType view, std::wstring_view word, long word_start) const {
  if (!m_speller_container.active_speller().is_working() || word.empty()) {
    return false;
  }
  // Well Numbers have same codes for ANSI and Unicode I guess, so
  // If word contains number then it's probably just a number or some crazy name
  auto style = m_editor.get_style_at(view, word_start);
  auto lexer = m_editor.get_lexer(view);
  auto category = SciUtils::get_style_category(lexer, style, m_settings);
  if (category == SciUtils::StyleCategory::unknown) {
    return false;
  }

  if (category != SciUtils::StyleCategory::text && !((category == SciUtils::StyleCategory::comment && m_settings.check_comments) ||
                                                     (category == SciUtils::StyleCategory::string && m_settings.check_strings) ||
                                                     (category == SciUtils::StyleCategory::identifier && m_settings.check_variable_functions))) {
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
      std::find_if(word.begin(), word.end(), [](wchar_t wc) { return IsCharAlphaNumeric(wc) && !IsCharAlpha(wc); }) != word.end())
    return false;

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

  if (m_settings.ignore_having_underscore && word.find(L'_') != std::wstring_view::npos)
    return false;

  if (m_settings.ignore_starting_or_ending_with_apostrophe) {
    if (word.front() == '\'' || word.back() == '\'')
      return false;
  }

  return true;
}

bool SpellChecker::check_word(EditorViewType view, std::wstring_view word, long word_start) const {
  if (!is_spellchecking_needed(view, word, word_start))
    return true;

  return m_speller_container.active_speller().check_word(to_word_for_speller(word));
}

auto SpellChecker::non_alphabetic_tokenizer(std::wstring_view target) const {
  return make_condition_tokenizer(target,
                                  [this](wchar_t c) { return !IsCharAlphaNumeric(c) && m_settings.delimiter_exclusions.find(c) == std ::wstring_view::npos; },
                                  m_settings.split_camel_case);
}

auto SpellChecker::non_ansi_tokenizer(std::wstring_view target) const {

  return make_condition_tokenizer(target,
    [this](wchar_t c)
  {
    static const auto ansi_str = []()
    {
      constexpr auto char_cnt = 256;
      std::string s;
      for (int i = 1; i < char_cnt; ++i)
        s.push_back (static_cast<char> (i));
      auto ws = to_wstring (s);
      std::experimental::erase_if (ws, [](wchar_t c){ return !IsCharAlphaNumeric (c); });
      std::sort (ws.begin (), ws.end ());
      return ws;
    }();
    return !std::binary_search (ansi_str.begin (), ansi_str.end (), c) && m_settings.delimiter_exclusions.find(c) == std ::wstring_view::npos;
  },
    m_settings.split_camel_case);
}

auto SpellChecker::delimiter_tokenizer(std::wstring_view target) const { return make_delimiter_tokenizer(target, m_delimiters, m_settings.split_camel_case); }

long SpellChecker::next_token_end(std::wstring_view target, long index) const {
  switch (m_settings.tokenization_style) {
  case TokenizationStyle::by_non_alphabetic:
    return non_alphabetic_tokenizer(target).next_token_end(index);
  case TokenizationStyle::by_non_ansi:
    return non_ansi_tokenizer(target).next_token_end(index);
  case TokenizationStyle::by_delimiters:
    return delimiter_tokenizer(target).next_token_end(index);
  case TokenizationStyle::COUNT:
    break;
  }
  assert(false);
  return static_cast<long>(target.size());
}

long SpellChecker::prev_token_begin(std::wstring_view target, long index) const {
  switch (m_settings.tokenization_style) {
  case TokenizationStyle::by_non_alphabetic:
    return non_alphabetic_tokenizer(target).prev_token_begin(index);
  case TokenizationStyle::by_non_ansi:
    return non_ansi_tokenizer(target).prev_token_begin(index);
  case TokenizationStyle::by_delimiters:
    return delimiter_tokenizer(target).prev_token_begin(index);
  case TokenizationStyle::COUNT:
    break;
  }
  assert(false);
  return 0;
}

MappedWstring SpellChecker::get_document_mapped_wstring(EditorViewType view, long start, long end) const {
  auto result = SpellCheckerHelpers::to_mapped_wstring(m_editor, view, m_editor.get_text_range(view, start, end));
  for (auto &val : result.mapping)
    val += start;
  return result;
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

bool SpellChecker::check_text(EditorViewType view, const MappedWstring &text_to_check, long offset, CheckTextMode mode) const {
  if (text_to_check.str.empty())
    return false;

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
  case TokenizationStyle::by_non_ansi:
    tokens = non_ansi_tokenizer(sv).get_all_tokens();
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
    auto word_start = static_cast<long>(offset + text_to_check.to_original_index(static_cast<long>(token.data() - text_to_check.str.data())));
    auto word_end = static_cast<long>(offset + text_to_check.to_original_index(static_cast<long>(token.data() - text_to_check.str.data() + token.length())));
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
                 words_for_speller.begin(), [](auto &word) -> auto && { return std::move(word.word_for_speller); });
  auto spellcheck_result = m_speller_container.active_speller().check_words(words_for_speller);
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
    for (long i = 0; i < static_cast<long>(underline_buffer.size()) - 1; i += 2) {
      remove_underline(view, prev_pos, underline_buffer[i]);
      create_word_underline(view, underline_buffer[i], underline_buffer[i + 1]);
      prev_pos = underline_buffer[i + 1];
    }
    remove_underline(view, prev_pos, offset + static_cast<long>(text_len));
  }
  switch (mode) {
  case CheckTextMode::underline_errors:
    return true;
  case CheckTextMode::find_first:
    return stop;
  case CheckTextMode::find_all:
    return true;
  case CheckTextMode::find_last:
    if (resulting_word_start == -1)
      return false;
    else {
      m_editor.set_selection(view, resulting_word_start, resulting_word_end);
      return true;
    }
  };
  return false;
}

void SpellChecker::check_visible(EditorViewType view) {
  auto text = get_visible_text(view);
  check_text(view, text, 0, CheckTextMode::underline_errors);
}

void SpellChecker::recheck_visible(EditorViewType view) {
  if (!m_speller_container.active_speller().is_working()) {
    clear_all_underlines(view);
    return;
  }

  // to utf-8 or no
  if (SpellCheckerHelpers::is_spell_checking_needed(m_editor, m_settings))
    check_visible(view);
  else
    clear_all_underlines(view);
}

std::wstring SpellChecker::get_all_misspellings_as_string() const {
  auto view = m_editor.active_view();
  auto buf = m_editor.get_active_document_text(view);
  auto mapped_str = SpellCheckerHelpers::to_mapped_wstring(m_editor, view, buf.data());
  m_misspellings.clear();
  if (!check_text(view, mapped_str, 0, CheckTextMode::find_all))
    return {};
  std::sort(m_misspellings.begin(), m_misspellings.end(), [](const auto &lhs, const auto &rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), [](wchar_t lhs, wchar_t rhs) {
      return CharUpper(reinterpret_cast<LPWSTR>(lhs)) < CharUpper(reinterpret_cast<LPWSTR>(rhs));
    });
  });
  auto it = std::unique(m_misspellings.begin(), m_misspellings.end(), [](const auto &lhs, const auto &rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                      [](wchar_t lhs, wchar_t rhs) { return CharUpper(reinterpret_cast<LPWSTR>(lhs)) == CharUpper(reinterpret_cast<LPWSTR>(rhs)); });
  });
  m_misspellings.erase(it, m_misspellings.end());
  std::wstring str;
  for (auto &s : m_misspellings)
    str += std::wstring{s} + L'\n';
  m_misspellings.clear();
  return str;
}

void SpellChecker::update_delimiters() { m_delimiters = L" \n\r\t\v" + parse_string(m_settings.delimiters.c_str()); }
