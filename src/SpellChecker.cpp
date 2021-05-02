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

#include "NativeSpellerInterface.h"
#include "SpellerInterface.h"

#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "MainDefs.h"
#include "MappedWstring.h"
#include "Settings.h"
#include "SpellChecker.h"
#include "SpellCheckerHelpers.h"
#include "SpellerContainer.h"
#include "npp/EditorInterface.h"
#include "npp/NppInterface.h"

#include <ranges>

SpellChecker::SpellChecker(const Settings *settings, EditorInterface &editor, const SpellerContainer &speller_container)
    : m_settings(*settings), m_editor(editor), m_speller_container(speller_container) {
  m_current_position = 0;
  m_settings.settings_changed.connect([this] { on_settings_changed(); });
  m_speller_container.speller_status_changed.connect([this] { recheck_visible_both_views(); });
  on_settings_changed();
}

SpellChecker::~SpellChecker() = default;

void SpellChecker::recheck_visible_both_views() {
  SpellCheckerHelpers::print_to_log(&m_settings, L"void SpellChecker::recheck_visible_both_views()", m_editor.get_editor_hwnd());
  auto view_count = m_editor.get_view_count();
  for (int view_index = 0; view_index < view_count; ++view_index)
  {
    TARGET_VIEW_BLOCK(m_editor, view_index);
    recheck_visible();
  }
}

void SpellChecker::recheck_visible_on_active_view() {
  ACTIVE_VIEW_BLOCK(m_editor);
  recheck_visible();
}

void SpellChecker::find_next_mistake() {
  ACTIVE_VIEW_BLOCK(m_editor);
  m_current_position = m_editor.get_current_pos();
  auto doc_length = m_editor.get_active_document_length();
  auto iterator_pos = prev_token_begin_in_document(m_current_position);
  bool full_check = false;

  while (true) {
    auto from = static_cast<TextPosition>(iterator_pos);
    auto to = static_cast<TextPosition>(iterator_pos + 4096);
    int ignore_offsetting = 0;
    if (to > doc_length) {
      ignore_offsetting = 1;
      to = static_cast<TextPosition>(doc_length);
    }
    if (from < to) {
      auto text = m_editor.get_mapped_wstring_range(from, to);
      auto index = static_cast<TextPosition>(text.str.size());
      if (to != doc_length && next_token_end(text.str, to) == index)
        index = prev_token_begin(text.str, index - 1);
      text.str.erase(index, text.str.size() - index);
      if (check_text<CheckTextMode::find_first>(text))
        break;

      iterator_pos += (text.to_original_index(index) - from);
    }

    if (to == doc_length) {
      if (full_check)
        break;

      m_current_position = 0;
      iterator_pos = 0;
      full_check = true;
    }
  }
}

void SpellChecker::find_prev_mistake() {
  ACTIVE_VIEW_BLOCK(m_editor);
  m_current_position = m_editor.get_current_pos();
  auto doc_length = m_editor.get_active_document_length();

  auto iterator_pos = next_token_end_in_document(m_current_position);
  bool full_check = false;

  while (true) {
    auto from = static_cast<TextPosition>(iterator_pos - 4096);
    auto to = static_cast<TextPosition>(iterator_pos);
    int ignore_offsetting = 0;
    if (from < 0) {
      from = 0;
      ignore_offsetting = 1;
    }

    if (from < to) {
      auto text = m_editor.get_mapped_wstring_range(from, to);
      auto offset = next_token_end(text.str, 0);
      if (check_text<CheckTextMode::find_last>(text))
        break;

      iterator_pos -= (4096 - (text.to_original_index(offset) - from));
    } else
      --iterator_pos;

    if (iterator_pos < 0) {
      if (full_check)
        break;

      m_current_position = doc_length + 1;
      iterator_pos = doc_length;
      full_check = true;
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

std::wstring_view SpellChecker::get_word_at(TextPosition char_pos, const MappedWstring &text) const {
  auto index = text.from_original_index(char_pos);
  if (index >= static_cast<TextPosition>(text.str.length()))
    index = static_cast<TextPosition>(text.str.length()) - 1;
  auto begin = prev_token_begin(text.str, index);
  auto end = next_token_end(text.str, begin);
  return std::wstring_view(text.str).substr(begin, end - begin);
}

void SpellChecker::refresh_underline_style() {
  auto view_count = m_editor.get_view_count();
  for (int view_index = 0; view_index < view_count; ++view_index) {
    TARGET_VIEW_BLOCK (m_editor, view_index);
    m_editor.set_indicator_style(spell_check_indicator_id, m_settings.data.underline_style);
    m_editor.set_indicator_foreground(spell_check_indicator_id, m_settings.data.underline_color);
  }
}

void SpellChecker::on_settings_changed() {
  refresh_underline_style();
  recheck_visible_both_views();
}

void SpellChecker::create_word_underline(TextPosition start, TextPosition end) const {
  m_editor.set_current_indicator(spell_check_indicator_id);
  m_editor.indicator_fill_range(start, end);
}

void SpellChecker::remove_underline(TextPosition start, TextPosition end) const {
  m_editor.set_current_indicator(spell_check_indicator_id);
  m_editor.indicator_clear_range(start, end);
}

TextPosition SpellChecker::prev_token_begin_in_document(TextPosition start) const {
  TextPosition shift = 15;
  auto prev_start = start + 1;
  while (start > 0) {
    start = std::max(start - shift, 0_sz);
    auto mapped_str = m_editor.get_mapped_wstring_range(start, prev_start);
    // finding any start before start which starts a token
    auto index = prev_token_begin(mapped_str.str, static_cast<TextPosition>(mapped_str.str.length()) - 1);
    if (index > 0)
      return mapped_str.to_original_index(index);
    prev_start = start;
    shift *= 2;
  }
  return start;
}

TextPosition SpellChecker::next_token_end_in_document(TextPosition end) const {
  TextPosition shift = 15;
  auto prev_end = end;
  auto length = m_editor.get_active_document_length();
  if (end == length)
    return end;
  while (end > 0) {
    end = std::min(end + shift, length);
    auto mapped_str = m_editor.get_mapped_wstring_range(prev_end, end);
    // finding any start before start which starts a token
    auto index = next_token_end(mapped_str.str, 0);
    if (index < static_cast<TextPosition>(mapped_str.str.length()))
      return mapped_str.to_original_index(index);
    if (end == length)
      return end;
    prev_end = end;
    shift *= 2;
  }
  return end;
}

MappedWstring SpellChecker::get_visible_text() {
  auto top_visible_line = m_editor.get_first_visible_line();
  auto top_visible_line_index = m_editor.get_document_line_from_visible(top_visible_line);
  auto bottom_visible_line_index = m_editor.get_document_line_from_visible(top_visible_line + m_editor.get_lines_on_screen() - 1);
  auto rect = m_editor.editor_rect();
  auto len = m_editor.get_active_document_length();
  MappedWstring result;
  for (auto line = top_visible_line_index; line <= bottom_visible_line_index; ++line) {
    auto start = m_editor.get_line_start_position(line);
    if (start >= len) // skipping possible empty lines when document is too short
      continue;
    auto start_point = m_editor.get_point_from_position(start);
    if (start_point.y < rect.top) {
      start = m_editor.char_position_from_point({0, 0});
      start = prev_token_begin_in_document(start);
    } else if (start_point.x < rect.left) {
      start = m_editor.char_position_from_point({0, start_point.y});
      start = prev_token_begin_in_document(start);
    }
    auto end = m_editor.get_line_end_position(line);
    auto end_point = m_editor.get_point_from_position(end);
    if (end_point.y > rect.bottom - rect.top) {
      end = m_editor.char_position_from_point({rect.right - rect.left, rect.bottom - rect.top});
      end = next_token_end_in_document(end);
    } else if (end_point.x > rect.right) {
      end = m_editor.char_position_from_point({rect.right - rect.left, end_point.y});
      end = next_token_end_in_document(end);
    }
    auto new_str = m_editor.get_mapped_wstring_range(start, end);
    result.append(new_str);
  }
  return result;
}

void SpellChecker::clear_all_underlines() const {
  auto length = m_editor.get_active_document_length();
  if (length > 0) {
    m_editor.set_current_indicator(spell_check_indicator_id);
    m_editor.indicator_clear_range(0, length);
  }
}

bool SpellChecker::is_spellchecking_needed(std::wstring_view word, TextPosition word_start) const {
  if (!m_speller_container.active_speller().is_working())
    return false;

  return SpellCheckerHelpers::is_word_spell_checking_needed (m_settings, m_editor, word, word_start);
}

bool SpellChecker::is_word_under_cursor_correct(TextPosition &pos, TextPosition &length, bool use_text_cursor) const {
  TextPosition init_char_pos, selection_start = 0, selection_end = 0;
  ACTIVE_VIEW_BLOCK(m_editor);
  length = 0;
  pos = -1;

  if (!use_text_cursor) {
    auto p = m_editor.get_mouse_cursor_pos();
    if (!p)
      return true;

    auto mb_pos = m_editor.char_position_from_global_point(p->x, p->y);
    if (!mb_pos)
      return true;
    init_char_pos = *mb_pos;
  } else {
    selection_start = m_editor.get_selection_start();
    selection_end = m_editor.get_selection_end();
    init_char_pos = std::min (selection_start, selection_end);
  }

  auto line = m_editor.line_from_position(init_char_pos);
  auto mapped_str = m_editor.get_mapped_wstring_line(line);
  if (mapped_str.str.empty())
    return true;
  auto word = get_word_at(static_cast<TextPosition>(init_char_pos), mapped_str);
  if (word.empty())
    return true;
  SpellCheckerHelpers::cut_apostrophes(m_settings, word);
  pos = static_cast<TextPosition>(mapped_str.to_original_index(static_cast<TextPosition>(word.data() - mapped_str.str.data())));
  TextPosition pos_end = mapped_str.to_original_index(static_cast<TextPosition>(word.data() + word.length() - mapped_str.str.data()));
  TextPosition word_len = pos_end - pos;
  if (selection_start != selection_end && (selection_start != pos || selection_end != pos + word_len))
    return true;
  if (check_word(word, pos)) {
    return true;
  }
  length = word_len;
  return false;
}

void SpellChecker::erase_all_misspellings() {
  ACTIVE_VIEW_BLOCK (m_editor);
  auto mapped_str = m_editor.to_mapped_wstring(m_editor.get_active_document_text());
  m_misspellings.clear();
  check_text<CheckTextMode::find_all>(mapped_str);

  UNDO_BLOCK(m_editor);
  TextPosition chars_removed = 0;
  for (auto &misspelling : m_misspellings) {
    auto start = mapped_str.to_original_index (static_cast<TextPosition> (misspelling.data() - mapped_str.str.data()));
    auto original_len = mapped_str.to_original_index (static_cast<TextPosition> (misspelling.data() - mapped_str.str.data() + misspelling.length ())) - start;
    m_editor.delete_range(start - chars_removed, original_len);
    chars_removed += original_len;
  }
}

bool SpellChecker::check_word(std::wstring_view word, TextPosition word_start) const {
  SpellCheckerHelpers::print_to_log(&m_settings, L"bool SpellChecker::check_word(NppViewType view, std::wstring_view word, TextPosition word_start)", m_editor.get_editor_hwnd());
  if (!is_spellchecking_needed(word, word_start))
    return true;

  return m_speller_container.active_speller().check_word(to_word_for_speller(word));
}

TextPosition SpellChecker::next_token_end(std::wstring_view target, TextPosition index) const {
  return m_settings.do_with_tokenizer (target, [index](const auto &tokenizer){ return tokenizer.next_token_end (index); });
}

TextPosition SpellChecker::prev_token_begin(std::wstring_view target, TextPosition index) const {
  return m_settings.do_with_tokenizer (target, [index](const auto &tokenizer){ return tokenizer.prev_token_begin (index); });
}

class SpellerWordData {
public:
  std::wstring_view token;
  WordForSpeller word_for_speller;
  TextPosition word_start;
  TextPosition word_end;
  bool is_correct;
};

std::vector<SpellerWordData> SpellChecker::generate_words_to_check (const MappedWstring &text_to_check) const {
  if (text_to_check.str.empty())
    return {};
  auto sv = std::wstring_view(text_to_check.str);
  std::vector<std::wstring_view> tokens;
  m_settings.do_with_tokenizer(sv, [&](const auto &tokenizer){ tokens = tokenizer.get_all_tokens (); });

  std::vector<bool> results(tokens.size());
  std::vector<SpellerWordData> words_to_check;
  words_to_check.clear();
  std::vector<WordForSpeller> words_for_speller;
  for (auto token : tokens) {
    SpellCheckerHelpers::cut_apostrophes(m_settings, token);
    auto word_start = static_cast<TextPosition>(text_to_check.to_original_index(static_cast<TextPosition>(token.data() - text_to_check.str.data())));
    auto word_end = static_cast<TextPosition>(text_to_check.to_original_index(static_cast<TextPosition>(token.data() - text_to_check.str.data() + token.length())));
    if (is_spellchecking_needed(token, word_start)) {
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
  return words_to_check;
}

void SpellChecker::underline_errors(const MappedWstring &text_to_check) const {
  std::vector<TextPosition> underline_buffer;
  auto words_to_check = generate_words_to_check (text_to_check);
  for (auto &result : words_to_check) {
    if (result.is_correct)
      continue;
    std::array list {result.word_start, result.word_end};
    underline_buffer.insert(underline_buffer.end (), list.begin (), list.end ());
  }
  TextPosition prev_pos = 0;
  for (TextPosition i = 0; i < static_cast<TextPosition>(underline_buffer.size()) - 1; i += 2) {
    remove_underline(prev_pos, underline_buffer[i]);
    create_word_underline(underline_buffer[i], underline_buffer[i + 1]);
    prev_pos = underline_buffer[i + 1];
  }
  auto text_len = text_to_check.original_length();
  remove_underline(prev_pos, text_len);
}

template <SpellChecker::CheckTextMode mode>
bool SpellChecker::check_text(const MappedWstring &text_to_check) const {
  bool stop = false;
  TextPosition resulting_word_start = -1, resulting_word_end = -1;
  auto words_to_check = generate_words_to_check (text_to_check);
  std::vector<TextPosition> underline_buffer;
  for (auto &result : words_to_check) {
    if (!result.is_correct) {
      auto word_start = result.word_start;
      auto word_end = result.word_end;
      switch (mode) {
      case CheckTextMode::find_first:
        if (word_end > m_current_position) {
          m_editor.set_selection(word_start, word_end);
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

  if constexpr (mode == CheckTextMode::find_all) {
    return true;
  } else if constexpr (mode == CheckTextMode::find_first) {
    return stop;
  }
  else if constexpr (mode == CheckTextMode::find_last) {
    if (resulting_word_start == -1)
      return false;

    m_editor.set_selection(resulting_word_start, resulting_word_end);
    return true;
  } else {
    static_assert(mode == CheckTextMode::find_last, "Incorrect mode was specified");
  }
}

void SpellChecker::check_visible() {
  SpellCheckerHelpers::print_to_log(&m_settings, L"void SpellChecker::check_visible(NppViewType view)", m_editor.get_editor_hwnd());
  auto text = get_visible_text();
  underline_errors(text);
}

void SpellChecker::recheck_visible() {
  if (!m_speller_container.active_speller().is_working()) {
    clear_all_underlines();
    return;
  }

  if (!SpellCheckerHelpers::is_spell_checking_needed_for_file(m_editor, m_settings))
    return clear_all_underlines();

  check_visible();
}

std::wstring SpellChecker::get_all_misspellings_as_string() const {
  ACTIVE_VIEW_BLOCK (m_editor);
  auto buf = m_editor.get_active_document_text();
  auto mapped_str = m_editor.to_mapped_wstring(buf);
  m_editor.force_style_update (mapped_str.mapping.front (), mapped_str.mapping.back ());
  m_misspellings.clear();
  check_text<CheckTextMode::find_all>(mapped_str);
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

void SpellChecker::mark_lines_with_misspelling() const {
  ACTIVE_VIEW_BLOCK (m_editor);
  auto buf = m_editor.get_active_document_text();
  auto mapped_str = m_editor.to_mapped_wstring(buf);
  m_editor.force_style_update (mapped_str.mapping.front (), mapped_str.mapping.back ());
  check_text<CheckTextMode::find_all>(mapped_str);
  for (auto &misspelling : m_misspellings) {
    auto start_index = misspelling.data () - mapped_str.str.data ();
    auto position = mapped_str.to_original_index (start_index);
    auto line = m_editor.line_from_position(position);
    m_editor.add_bookmark(line);
  }
  m_misspellings.clear();
}
