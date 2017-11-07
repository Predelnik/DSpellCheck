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

#include "MainDef.h"
#include "lsignal.h"
#include "CommonFunctions.h"

class Settings;
struct AspellSpeller;
class LanguageInfo;
struct AspellWordList;
class SettingsDlg;
class LangList;
class SuggestionsButton;
class SpellerInterface;
class AspellInterface;
class HunspellInterface;
class SelectProxy;
class MappedWstring;

struct SuggestionsMenuItem {
    std::wstring text;
    BYTE id;
    bool separator;
    SuggestionsMenuItem(const wchar_t* text_arg, int id_arg, bool separator_arg = false);

    ~SuggestionsMenuItem() {
    };
};

void insert_sugg_menu_item(HMENU menu, const wchar_t* text, BYTE id, int insert_pos,
                           bool separator = false);

HWND get_scintilla_window(const NppData* npp_data_arg);
LRESULT send_msg_to_editor(HWND scintilla_window, UINT msg,
                                  WPARAM w_param = 0, LPARAM l_param = 0);
bool send_msg_to_both_editors(const NppData* npp_data_arg, UINT msg,
                              WPARAM w_param = 0, LPARAM l_param = 0);
LRESULT post_msg_to_active_editor(HWND scintilla_window, UINT msg, WPARAM w_param = 0,
                                  LPARAM l_param = 0);

class SpellChecker {
    enum class CheckTextMode {
        underline_errors = 0,
        find_first = 1,
        find_last = 2,
        get_first = 3,
        // Returns position of first (for recurring usage)
    };

public:
    mutable lsignal::signal<void()> speller_status_changed;

public:
    SpellChecker(const wchar_t* ini_file_path_arg,
                 SettingsDlg* settings_dlg_instance_arg, NppData* npp_data_instance_arg,
                 SuggestionsButton* suggestions_instance_arg,
                 LangList* lang_list_instance_arg,
                 const Settings *settings);
    ~SpellChecker();
    void recheck_visible_both_views();
    const SpellerInterface* active_speller() const;
    void apply_proxy_settings();
    void show_suggestion_menu();
    void precalculate_menu();
    void recheck_visible(bool not_intersection_only = false);
    void recheck_modified();
    MappedWstring to_mapped_wstring(std::string_view str);
    void error_msg_box(const wchar_t* message);

    bool aspell_reinit_settings();
    void update_hunspell_language_options();
    void update_aspell_language_options();
    std::wstring get_default_hunspell_path();
    void update_suggestion_box();

    void init_speller();
    int get_aspell_status();;
    void do_plugin_menu_inclusion(bool invalidate = false);
    HunspellInterface* get_hunspell_speller() { return m_hunspell_speller.get(); };
    bool hunspell_reinit_settings(bool reset_directory);
    long previous_a, previous_b;
    void set_suggestions_box_transparency();
    void process_menu_result(WPARAM menu_id);
    void copy_misspellings_to_clipboard();
    void update_delimiters();
    void on_settings_changed();
    void init_suggestions_box();
    void hide_suggestion_box();
    void set_default_delimiters();
    void find_next_mistake();
    void find_prev_mistake();
    void check_file_name();
    void update_from_remove_dics_options();

    void lang_change();
    std::vector<LanguageInfo> get_available_languages() const;

private:
    HWND get_current_scintilla();
    static void create_word_underline(HWND scintilla_window, long start, long end);
    static void remove_underline(HWND scintilla_window, long start, long end);
    void clear_all_underlines();
    void clear_visible_underlines();
    bool check_word(std::wstring word, long start, long end);
    void get_visible_limits(long& start, long& finish);
    MappedWstring get_visible_text(long* offset, bool not_intersection_only = false);
    int check_text(const MappedWstring& text_to_check, long offset, CheckTextMode mode, size_t skip_chars = 0);
    void check_visible(bool not_intersection_only = false);
    void set_encoding_by_id(int enc_id);
    void save_settings();
    std::vector<SuggestionsMenuItem> fill_suggestions_menu(HMENU menu);
    bool get_word_under_cursor_is_right(long& pos, long& length,
                                        bool use_text_cursor = false);
    std::wstring_view get_word_at(long char_pos, const MappedWstring& text, long offset) const;
    bool check_text_needed();
    LRESULT get_style(int pos);
    void refresh_underline_style();
    void apply_conversions(std::wstring& word);
    void reset_hot_spot_cache();
    void cut_apostrophes(std::wstring_view& word);

    void save_to_ini(const wchar_t* name, const wchar_t* value,
                     const wchar_t* default_value, bool in_quotes = false);
    void save_to_ini(const wchar_t* name, int value, int default_value);
    void save_to_ini_utf8(const wchar_t* name, const char* value,
                          const char* default_value, bool in_quotes = false);

    std::wstring load_from_ini(const wchar_t* name,
                             const wchar_t* default_value, bool in_quotes = false);
    void load_from_ini(bool& value, const wchar_t* name, bool default_value);
    void load_from_ini(int& value, const wchar_t* name, int default_value);
    std::string load_from_ini_utf8(const wchar_t* name,
                                const char* default_value, bool in_quotes = false);
    static int check_text_default_answer(CheckTextMode mode);
    std::optional<ptrdiff_t> next_token_end(std::wstring_view target, ptrdiff_t index) const;
    std::optional<ptrdiff_t> prev_token_begin(std::wstring_view target, ptrdiff_t index) const;
    auto non_alphabetic_tokenizer (std::wstring_view target) const;
    auto delimiter_tokenizer(std::wstring_view target) const;

private:
    bool m_settings_loaded;
    bool m_check_text_enabled; // cache for check_those
    bool m_word_under_cursor_is_correct;
    // converted to corresponding symbols
    const AspellWordList* m_cur_word_list;
    HWND m_current_scintilla;
    LRESULT m_hot_spot_cache[256]; // STYLE_MAX = 255
    const Settings &m_settings;
    std::wstring m_delimiters;

    LRESULT m_lexer;
    std::vector<std::wstring> m_last_suggestions;
    long m_modified_start;
    long m_modified_end;
    long m_word_under_cursor_pos;
    std::size_t m_word_under_cursor_length;
    LRESULT m_current_position;
    NppData* m_npp_data_instance;
    EncodingType m_current_encoding;
    std::wstring m_ini_file_path;
    MappedWstring m_selected_word;
    SettingsDlg* m_settings_dlg_instance;
    SuggestionsButton* m_suggestions_instance;
    LangList* m_lang_list_instance;
    long m_visible_text_offset;

    SpellerInterface* m_current_speller;
    std::unique_ptr<AspellInterface> m_aspell_speller;
    std::unique_ptr<HunspellInterface> m_hunspell_speller;
};
