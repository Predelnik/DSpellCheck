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
// Class that will do most of the job with spellchecker

#include "npp/EditorInterface.h"


class EditorInterface;
class Settings;
class LanguageInfo;
class SettingsDlg;
class LangList;
class SuggestionsButton;
class SpellerInterface;
class AspellInterface;
class HunspellInterface;
class SelectProxy;
class NativeSpellerInterface;
class WordForSpeller;
class SpellerContainer;
class SpellerWordData;

class SpellChecker {
  enum class CheckTextMode {
    find_first,
    find_last,
  };

public:
  SpellChecker(const Settings *settings, EditorInterface &editor,
               const SpellerContainer &speller_container);
  ~SpellChecker();
  void recheck_visible_both_views();
  void recheck_visible();

  std::wstring get_all_misspellings_as_string() const;
  void on_settings_changed();
  void find_next_mistake();
  void find_prev_mistake();
  WordForSpeller to_word_for_speller(std::wstring_view word) const;
  void recheck_visible_on_active_view();
  bool is_word_under_cursor_correct(TextPosition &pos, TextPosition &length,
                                    bool use_text_cursor = false) const;
  void erase_all_misspellings();
  void mark_lines_with_misspelling() const;

private:
  void create_word_underline(TextPosition start, TextPosition end) const;
  void remove_underline(TextPosition start, TextPosition end) const;
  void clear_all_underlines() const;
  bool check_word(std::wstring_view word,
                  TextPosition word_start) const;
  TextPosition prev_token_begin_in_document(TextPosition start) const;
  TextPosition next_token_end_in_document(TextPosition end) const;
  MappedWstring get_visible_text();
  std::vector<SpellerWordData> check_text (const MappedWstring &text_to_check) const;
  void underline_misspelled_words(const MappedWstring &text_to_check) const;
  std::vector<std::wstring_view> get_misspelled_words(const MappedWstring &text_to_check) const;
  std::optional<std::array<TextPosition, 2>> find_first_misspelling(const MappedWstring &text_to_check, TextPosition last_valid_position) const;
  std::optional<std::array<TextPosition, 2>> find_last_misspelling(const MappedWstring &text_to_check, TextPosition last_valid_position) const;
  void check_visible();

  std::wstring_view get_word_at(TextPosition char_pos, const MappedWstring &text) const;
  void refresh_underline_style();
  bool is_spellchecking_needed(std::wstring_view word,
                               TextPosition word_start) const;
  TextPosition next_token_end(std::wstring_view target, TextPosition index) const;
  TextPosition prev_token_begin(std::wstring_view target, TextPosition index) const;

private:
  const Settings &m_settings;

  EditorInterface &m_editor;
  const SpellerContainer &m_speller_container;
};
