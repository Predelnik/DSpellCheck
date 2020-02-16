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

#include "utils/enum_range.h"

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
  recheck_visible(NppViewType::primary);
  recheck_visible(NppViewType::secondary);
}

void SpellChecker::lang_change() { recheck_visible(m_editor.active_view()); }

void SpellChecker::find_next_mistake() {
  auto view = m_editor.active_view();
  m_current_position = m_editor.get_current_pos(view);
  auto doc_length = m_editor.get_active_document_length(view);
  auto iterator_pos = prev_token_begin_in_document(view, m_current_position);
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
      auto text = m_editor.get_mapped_wstring_range(view, from, to);
      auto index = static_cast<TextPosition>(text.str.size());
      if (to != doc_length && next_token_end(text.str, to) == index)
        index = prev_token_begin(text.str, index - 1);
      text.str.erase(index, text.str.size() - index);
      if (check_text(view, text, CheckTextMode::find_first))
        break;

      iterator_pos += (text.to_original_index(index) - from);
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
    auto from = static_cast<TextPosition>(iterator_pos - 4096);
    auto to = static_cast<TextPosition>(iterator_pos);
    int ignore_offsetting = 0;
    if (from < 0) {
      from = 0;
      ignore_offsetting = 1;
    }

    if (from < to) {
      auto text = m_editor.get_mapped_wstring_range(view, from, to);
      auto offset = next_token_end(text.str, 0);
      if (check_text(view, text, CheckTextMode::find_last))
        break;

      iterator_pos -= (4096 - (text.to_original_index(offset) - from));
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
    auto view = static_cast<NppViewType> (view_index);
    m_editor.set_indicator_style(spell_check_indicator_id, m_settings.underline_style);
    m_editor.set_indicator_foreground(view, spell_check_indicator_id, m_settings.underline_color);
  }
}

void SpellChecker::on_settings_changed() {
  refresh_underline_style();
  recheck_visible_both_views();
}

void SpellChecker::create_word_underline(NppViewType view, TextPosition start, TextPosition end) const {
  m_editor.set_current_indicator(view, spell_check_indicator_id);
  m_editor.indicator_fill_range(view, start, end);
}

void SpellChecker::remove_underline(NppViewType view, TextPosition start, TextPosition end) const {
  if (end < start)
    return;
  m_editor.set_current_indicator(view, spell_check_indicator_id);
  m_editor.indicator_clear_range(view, start, end);
}

TextPosition SpellChecker::prev_token_begin_in_document(NppViewType view, TextPosition start) const {
  TextPosition shift = 15;
  auto prev_start = start + 1;
  while (start > 0) {
    start = std::max(start - shift, 0_sz);
    auto mapped_str = m_editor.get_mapped_wstring_range(view, start, prev_start);
    // finding any start before start which starts a token
    auto index = prev_token_begin(mapped_str.str, static_cast<TextPosition>(mapped_str.str.length()) - 1);
    if (index > 0)
      return mapped_str.to_original_index(index);
    prev_start = start;
    shift *= 2;
  }
  return start;
}

TextPosition SpellChecker::next_token_end_in_document(NppViewType view, TextPosition end) const {
  TextPosition shift = 15;
  auto prev_end = end;
  auto length = m_editor.get_active_document_length(view);
  if (end == length)
    return end;
  while (end > 0) {
    end = std::min(end + shift, length);
    auto mapped_str = m_editor.get_mapped_wstring_range(view, prev_end, end);
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

MappedWstring SpellChecker::get_visible_text(NppViewType view) {

  auto top_visible_line = m_editor.get_first_visible_line(view);
  auto top_visible_line_index = m_editor.get_document_line_from_visible(view, top_visible_line);
  auto bottom_visible_line_index = m_editor.get_document_line_from_visible(view, top_visible_line + m_editor.get_lines_on_screen(view) - 1);
  auto rect = m_editor.editor_rect(view);
  auto len = m_editor.get_active_document_length(view);
  MappedWstring result;
  for (auto line = top_visible_line_index; line <= bottom_visible_line_index; ++line) {
    if (!m_editor.is_line_visible(view, line))
      continue;
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
    auto new_str = m_editor.get_mapped_wstring_range(view, start, end);
    result.append(new_str);
  }
  return result;
}

void SpellChecker::clear_all_underlines(NppViewType view) const {
  auto length = m_editor.get_active_document_length(view);
  if (length > 0) {
    m_editor.set_current_indicator(view, spell_check_indicator_id);
    m_editor.indicator_clear_range(view, 0, length);
  }
}

bool SpellChecker::is_spellchecking_needed(NppViewType view, std::wstring_view word, TextPosition word_start) const {
  if (!m_speller_container.active_speller().is_working())
    return false;

  return SpellCheckerHelpers::is_word_spell_checking_needed (m_settings, m_editor, view, word, word_start);
}

bool SpellChecker::is_word_under_cursor_correct(TextPosition &pos, TextPosition &length, bool use_text_cursor) const {
  POINT p;
  TextPosition init_char_pos, selection_start = 0, selection_end = 0;
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
    init_char_pos = std::min (selection_start, selection_end);
  }

  if (init_char_pos == -1)
    return true;
  auto line = m_editor.line_from_position(view, init_char_pos);
  auto mapped_str = m_editor.get_mapped_wstring_line(view, line);
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
  if (check_word(view, word, pos)) {
    return true;
  }
  length = word_len;
  return false;
}

void SpellChecker::erase_all_misspellings() {
  auto view = m_editor.active_view();
  m_editor.target_active_view();
  auto mapped_str = m_editor.to_mapped_wstring(view, m_editor.get_active_document_text(view));
  m_misspellings.clear();
  if (!check_text(view, mapped_str, CheckTextMode::find_all))
    return;

  UNDO_BLOCK(m_editor, view);
  TextPosition chars_removed = 0;
  for (auto &misspelling : m_misspellings) {
    auto start = mapped_str.to_original_index (static_cast<TextPosition> (misspelling.data() - mapped_str.str.data()));
    auto original_len = mapped_str.to_original_index (static_cast<TextPosition> (misspelling.data() - mapped_str.str.data() + misspelling.length ())) - start;
    m_editor.delete_range(start - chars_removed, original_len);
    chars_removed += original_len;
  }
}

bool SpellChecker::check_word(NppViewType view, std::wstring_view word, TextPosition word_start) const {
  SpellCheckerHelpers::print_to_log(&m_settings, L"bool SpellChecker::check_word(NppViewType view, std::wstring_view word, TextPosition word_start)", m_editor.get_editor_hwnd());
  if (!is_spellchecking_needed(view, word, word_start))
    return true;

  return m_speller_container.active_speller().check_word(to_word_for_speller(word));
}

TextPosition SpellChecker::next_token_end(std::wstring_view target, TextPosition index) const {
  return m_settings.do_with_tokenizer (target, [index](const auto &tokenizer){ return tokenizer.next_token_end (index); });
}

TextPosition SpellChecker::prev_token_begin(std::wstring_view target, TextPosition index) const {
  return m_settings.do_with_tokenizer (target, [index](const auto &tokenizer){ return tokenizer.prev_token_begin (index); });
}

namespace {
class WordData {
public:
  std::wstring_view token;
  WordForSpeller word_for_speller;
  TextPosition word_start;
  TextPosition word_end;
  bool is_correct;
};
} // namespace

bool SpellChecker::check_text(NppViewType view, const MappedWstring &text_to_check, CheckTextMode mode) const {
  TARGET_VIEW_BLOCK(m_editor, static_cast<int> (view));
  SpellCheckerHelpers::print_to_log(&m_settings, L"bool SpellChecker::check_text(NppViewType view, const MappedWstring &text_to_check, CheckTextMode mode)", m_editor.get_editor_hwnd());
  if (text_to_check.str.empty())
    return false;

  bool stop = false;
  TextPosition resulting_word_end = -1, resulting_word_start = -1;
  auto text_len = text_to_check.original_length();
  std::vector<TextPosition> underline_buffer;

  auto sv = std::wstring_view(text_to_check.str);
  std::vector<std::wstring_view> tokens;
  m_settings.do_with_tokenizer(sv, [&](const auto &tokenizer){ tokens = tokenizer.get_all_tokens (); });

  std::vector<bool> results(tokens.size());
  std::vector<WordData> words_to_check;
  words_to_check.clear();
  std::vector<WordForSpeller> words_for_speller;
  for (auto token : tokens) {
    SpellCheckerHelpers::cut_apostrophes(m_settings, token);
    auto word_start = static_cast<TextPosition>(text_to_check.to_original_index(static_cast<TextPosition>(token.data() - text_to_check.str.data())));
    auto word_end = static_cast<TextPosition>(text_to_check.to_original_index(static_cast<TextPosition>(token.data() - text_to_check.str.data() + token.length())));
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

  if (mode == CheckTextMode::underline_errors) {
    TextPosition prev_pos = 0;
    for (TextPosition i = 0; i < static_cast<TextPosition>(underline_buffer.size()) - 1; i += 2) {
      remove_underline(view, prev_pos, underline_buffer[i]);
      create_word_underline(view, underline_buffer[i], underline_buffer[i + 1]);
      prev_pos = underline_buffer[i + 1];
    }
    remove_underline(view, prev_pos, static_cast<TextPosition>(text_len));
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
      m_editor.set_selection(resulting_word_start, resulting_word_end);
      return true;
    }
  };
  return false;
}

void SpellChecker::check_visible(NppViewType view) {
  SpellCheckerHelpers::print_to_log(&m_settings, L"void SpellChecker::check_visible(NppViewType view)", m_editor.get_editor_hwnd());
  auto text = get_visible_text(view);
  check_text(view, text, CheckTextMode::underline_errors);
}

void SpellChecker::recheck_visible(NppViewType view) {
  if (!m_speller_container.active_speller().is_working()) {
    clear_all_underlines(view);
    return;
  }

  if (!SpellCheckerHelpers::is_spell_checking_needed_for_file(m_editor, m_settings))
    return clear_all_underlines(view);

  check_visible(view);
}

std::wstring SpellChecker::get_all_misspellings_as_string() const {
  auto view = m_editor.active_view();
  auto buf = m_editor.get_active_document_text(view);
  auto mapped_str = m_editor.to_mapped_wstring(view, buf);
  m_editor.target_active_view();
  m_editor.force_style_update (mapped_str.mapping.front (), mapped_str.mapping.back ());
  m_misspellings.clear();
  if (!check_text(view, mapped_str, CheckTextMode::find_all))
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

void SpellChecker::mark_lines_with_misspelling() const {
  auto view = m_editor.active_view();
  auto buf = m_editor.get_active_document_text(view);
  auto mapped_str = m_editor.to_mapped_wstring(view, buf);
  m_editor.target_active_view();
  m_editor.force_style_update (mapped_str.mapping.front (), mapped_str.mapping.back ());
  if (!check_text(view, mapped_str, CheckTextMode::find_all))
    return;
  for (auto &misspelling : m_misspellings) {
    auto start_index = misspelling.data () - mapped_str.str.data ();
    auto position = mapped_str.to_original_index (start_index);
    auto line = m_editor.line_from_position(view, position);
    m_editor.add_bookmark(view, line);
  }
  m_misspellings.clear();
}
