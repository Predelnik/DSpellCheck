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
#include "MainDef.h"
#include "lsignal.h"
#include "npp/EditorInterface.h"

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

struct SuggestionsMenuItem {
  std::wstring text;
  BYTE id;
  bool separator;
  SuggestionsMenuItem(const wchar_t *text_arg, int id_arg,
                      bool separator_arg = false);
};

void insert_sugg_menu_item(HMENU menu, const wchar_t *text, BYTE id,
                           int insert_pos, bool separator = false);

class SpellChecker {
  enum class CheckTextMode {
    underline_errors,
    find_first,
    find_last,
    find_all,
  };

public:
  SpellChecker(NppData *npp_data_instance_arg,
               const Settings *settings, EditorInterface &editor,
               SpellerContainer &speller_container);
  ~SpellChecker();
  void recheck_visible_both_views();
  void show_suggestion_menu();
  void precalculate_menu();
  void recheck_visible(EditorViewType view, bool not_intersection_only = false);
  MappedWstring to_mapped_wstring(EditorViewType view, std::string_view str) const;

  void update_suggestion_box();

  void do_plugin_menu_inclusion(bool invalidate = false);
  void set_suggestions_box_transparency();
  void process_menu_result(WPARAM menu_id);
  void copy_misspellings_to_clipboard();
  void update_delimiters();
  void on_settings_changed();
  void init_suggestions_box(SuggestionsButton& suggestion_button);
  void find_next_mistake();
  void find_prev_mistake();
  WordForSpeller to_word_for_speller(std::wstring_view word) const;
  void check_file_name();
  void lang_change();
  std::vector<SuggestionsMenuItem> fill_suggestions_menu(HMENU menu);

private:
  void create_word_underline(EditorViewType view, long start, long end);
  void remove_underline(EditorViewType view, long start, long end);
  void clear_all_underlines(EditorViewType view);
  bool check_word(EditorViewType view, std::wstring_view word, long word_start);
  void get_visible_limits(EditorViewType view, long &start, long &finish);
  MappedWstring get_visible_text(EditorViewType view, long *offset,
                                 bool not_intersection_only = false);
  void add_periods(const std::wstring_view &parent_string_view,
                   std::wstring_view &target);
  int check_text(EditorViewType view, const MappedWstring &text_to_check,
                 long offset, CheckTextMode mode);
  void check_visible(EditorViewType view, bool not_intersection_only = false);
  
  bool is_word_under_cursor_correct(long &pos, long &length,
                                    bool use_text_cursor = false);
  std::wstring_view get_word_at(long char_pos, const MappedWstring &text,
                                long offset) const;
  bool check_text_needed();
  void refresh_underline_style();
  void apply_conversions(std::wstring &word) const;
  void reset_hot_spot_cache();
  bool is_spellchecking_needed(EditorViewType view, std::wstring_view word,
                               long word_start);
  void cut_apostrophes(std::wstring_view &word);
  std::optional<long> next_token_end(std::wstring_view target,
                                     long index) const;
  std::optional<long> prev_token_begin(std::wstring_view target,
                                       long index) const;
  auto non_alphabetic_tokenizer(std::wstring_view target) const;
  auto delimiter_tokenizer(std::wstring_view target) const;

private:
  bool m_check_text_enabled; // cache for check_those
  bool m_word_under_cursor_is_correct;
  // converted to corresponding symbols
  LRESULT m_hot_spot_cache[256]; // STYLE_MAX = 255
  const Settings &m_settings;
  std::wstring m_delimiters;

  std::vector<std::wstring> m_last_suggestions;
  long m_word_under_cursor_pos;
  long m_word_under_cursor_length;
  long m_current_position;
  NppData *m_npp_data_instance;
  MappedWstring m_selected_word;
  SettingsDlg *m_settings_dlg_instance;  
  EditorInterface &m_editor;
  std::vector<std::wstring_view> m_misspellings;
  SpellerContainer &m_speller_container;
};
