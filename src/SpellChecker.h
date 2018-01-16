/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once
// Class that will do most of the job with spellchecker

#include "CommonFunctions.h"
#include "npp/EditorInterface.h"

#include <string_view>

struct AspellSpeller;
struct AspellWordList;

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
class MappedWstring;
class NativeSpellerInterface;
class WordForSpeller;
class SpellerContainer;

class SpellChecker {
  enum class CheckTextMode {
    underline_errors,
    find_first,
    find_last,
    find_all,
  };

public:
  SpellChecker(const Settings *settings, EditorInterface &editor,
               const SpellerContainer &speller_container);
  ~SpellChecker();
  void recheck_visible_both_views();
  void recheck_visible(EditorViewType view, bool not_intersection_only = false);

  std::wstring get_all_misspellings_as_string () const;
  void update_delimiters();
  void on_settings_changed();
  void find_next_mistake();
  void find_prev_mistake();
  WordForSpeller to_word_for_speller(std::wstring_view word) const;
  void lang_change();
  bool is_word_under_cursor_correct(long &pos, long &length,
                                    bool use_text_cursor = false) const;

private:
  void create_word_underline(EditorViewType view, long start, long end) const;
  void remove_underline(EditorViewType view, long start, long end) const;
  void clear_all_underlines(EditorViewType view) const;
  bool check_word(EditorViewType view, std::wstring_view word, long word_start) const;
  void get_visible_limits(EditorViewType view, long &start, long &finish);
  MappedWstring get_visible_text(EditorViewType view, long *offset,
                                 bool not_intersection_only = false);
  int check_text(EditorViewType view, const MappedWstring &text_to_check,
                 long offset, CheckTextMode mode) const;
  void check_visible(EditorViewType view, bool not_intersection_only = false);

  std::wstring_view get_word_at(long char_pos, const MappedWstring &text,
                                long offset) const;
  void refresh_underline_style();
  void reset_hot_spot_cache();
  bool is_spellchecking_needed(EditorViewType view, std::wstring_view word,
                               long word_start) const;
  std::optional<long> next_token_end(std::wstring_view target,
                                     long index) const;
  std::optional<long> prev_token_begin(std::wstring_view target,
                                       long index) const;
  auto non_alphabetic_tokenizer(std::wstring_view target) const;
  auto delimiter_tokenizer(std::wstring_view target) const;

private:
  // converted to corresponding symbols
  LRESULT m_hot_spot_cache[256]; // STYLE_MAX = 255
  const Settings &m_settings;
  std::wstring m_delimiters;

  long m_current_position;
  EditorInterface &m_editor;
  mutable std::vector<std::wstring_view> m_misspellings;
  const SpellerContainer &m_speller_container;
};
