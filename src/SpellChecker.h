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
LRESULT send_msg_to_active_editor(HWND scintilla_window, UINT msg,
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
    mutable lsignal::signal<void()> lang_list_changed;

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
    void error_msg_box(const wchar_t* message);

    bool aspell_reinit_settings();
    void set_hunspell_language(const wchar_t* str);
    void set_aspell_language(const wchar_t* str);
    void set_delimiters(const char* str);
    void set_suggestions_num(int num);
    void set_aspell_path(const wchar_t* path);
    void set_multiple_languages(std::wstring_view multi_string,
                                SpellerInterface* speller);
    void set_hunspell_path(const wchar_t* path);
    void set_hunspell_additional_path(const wchar_t* path);
    void set_conversion_options(bool convert_yo, bool convert_single_quotes_arg,
                                bool remove_boundary_apostrophes_arg);
    void set_check_those(int check_those_arg);
    void set_file_types(const wchar_t* file_types_arg);
    void set_check_comments(bool value);
    void set_hunspell_multiple_languages(const char* multi_languages_arg);
    void set_aspell_multiple_languages(const char* multi_languages_arg);
    void set_underline_color(int value);
    void set_underline_style(int value);
    void set_proxy_user_name(const wchar_t* str);
    void set_proxy_host_name(const wchar_t* str);
    void set_proxy_password(const wchar_t* str);
    void set_proxy_port(int value);
    void set_use_proxy(bool value);
    void set_proxy_anonymous(bool value);
    void set_proxy_type(int value);
    void set_ignore(bool ignore_numbers_arg, bool ignore_c_start_arg,
                    bool ignore_c_have_arg, bool ignore_c_all_arg, bool ignore_arg,
                    bool ignore_se_apostrophe_arg, bool ignore_one_letter_arg);
    std::wstring get_default_hunspell_path();
    void set_sugg_box_settings(int size, int transparency, int save_ini = 1);
    void set_buffer_size(int size);
    void set_sugg_type(int sugg_type);
    void set_lib_mode(int i);
    void set_decode_names(bool value);
    void set_show_only_know(bool value);
    void set_install_system(bool value);
    const wchar_t* get_hunspell_path() const { return m_hunspell_path.c_str(); }
    int get_aspell_status();;
    const wchar_t* get_hunspell_additional_path() const { return m_additional_hunspell_path.c_str(); };
    bool get_show_only_known();
    bool get_install_system();
    bool get_decode_names();
    void do_plugin_menu_inclusion(bool invalidate = false);
    HunspellInterface* get_hunspell_speller() { return m_hunspell_speller.get(); };
    int get_lib_mode();
    bool hunspell_reinit_settings(bool reset_directory);
    void set_remove_user_dics(bool value);
    void set_remove_system(bool value);
    bool get_remove_user_dics();
    bool get_remove_system();
    const wchar_t* get_proxy_user_name() const;
    const wchar_t* get_proxy_host_name() const;
    const wchar_t* get_proxy_password() const;
    int get_proxy_port();
    bool get_use_proxy();
    bool get_proxy_anonymous();
    int get_proxy_type();
    long previous_a, previous_b;
    void set_suggestions_box_transparency();
    void add_user_server(std::wstring server);
    void process_menu_result(WPARAM menu_id);
    void copy_misspellings_to_clipboard();
    void on_settings_changed();
    void init_suggestions_box();
    void hide_suggestion_box();
    void set_default_delimiters();
    void find_next_mistake();
    void find_prev_mistake();
    void check_file_name();
    void fill_download_dics_dialog();
    void update_select_proxy();
    void update_from_remove_dics_options();
    void update_remove_dics_options();
    void update_from_download_dics_options();
    void update_from_download_dics_options_no_update();
    void lang_change();
    void reset_download_combobox();
    void set_recheck_delay(int value);
    int get_recheck_delay();
    std::vector<LanguageInfo> get_available_languages() const;

private:
    HWND get_current_scintilla();
    static void create_word_underline(HWND scintilla_window, long start, long end);
    static void remove_underline(HWND scintilla_window, long start, long end);
    void clear_all_underlines();
    void clear_visible_underlines();
    const char* get_delimiters();
    bool check_word(std::string word, long start, long end);
    void get_visible_limits(long& start, long& finish);
    std::vector<char> get_visible_text(long* offset, bool not_intersection_only = false);
    int check_text(char* text_to_check, long offset, CheckTextMode mode);
    void check_visible(bool not_intersection_only = false);
    void set_encoding_by_id(int enc_id);
    void save_settings();
    std::vector<SuggestionsMenuItem> fill_suggestions_menu(HMENU menu);
    bool get_word_under_cursor_is_right(long& pos, long& length,
                                        bool use_text_cursor = false);
    std::string_view get_word_at(long char_pos, char* text, long offset) const;
    bool check_text_needed();
    LRESULT get_style(int pos);
    void refresh_underline_style();
    void apply_conversions(std::string& word);
    void prepare_string_for_conversion();
    void preserve_current_address_index();
    void fill_download_dics();
    void reset_hot_spot_cache();
    void cut_apostrophes(std::string_view& word);

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

private:
    bool m_settings_loaded;
    bool m_check_text_enabled;
    bool m_word_under_cursor_is_correct;
    std::wstring m_hunspell_language;
    std::wstring m_hunspell_multi_languages;
    std::wstring m_aspell_language;
    std::wstring m_aspell_multi_languages;
    int m_lib_mode; // 0 - Aspell, 1 - Hunspell
    int m_multi_lang_mode;
    int m_suggestions_num;
    int m_suggestions_mode;
    std::string m_delim_utf8; // String without special characters but maybe with escape
    // characters (like '\n' and stuff)
    std::string m_delim_utf8_converted; // String where escape characters are properly
    // converted to corresponding symbols
    std::string m_delim_converted; // Same but in ANSI encoding
    std::wstring m_server_names[3]; // Only user ones, there'll also be bunch of
    // predetermined ones
    std::wstring m_default_server_names[3];
    int m_last_used_address; // equals USER_SERVER_CONST + num if user address is
    // used, otherwise equals number of default server
    int m_address_is_set;
    std::wstring m_file_types;
    std::wstring m_aspell_path;
    std::wstring m_hunspell_path;
    std::wstring m_additional_hunspell_path;
    bool m_ignore_yo;
    bool m_convert_single_quotes;
    bool m_remove_boundary_apostrophes;
    bool m_check_those;
    bool m_check_only_comments_and_string;
    int m_underline_color;
    int m_underline_style;
    bool m_ignore_numbers;
    bool m_ignore_starting_with_capital;
    bool m_ignore_having_a_capital;
    bool m_ignore_all_capital;
    bool m_ignore_having_underscore;
    bool m_ignore_starting_or_ending_with_apostrophe;
    bool m_ignore_one_letter;
    bool m_decode_names;
    bool m_show_only_known;
    bool m_install_system;
    int m_sb_size;
    int m_sb_trans;
    int m_buffer_size;
    const AspellWordList* m_cur_word_list;
    HWND m_current_scintilla;
    LRESULT m_hot_spot_cache[256]; // STYLE_MAX = 255
    bool m_use_proxy;
    bool m_proxy_anonymous;
    int m_proxy_type;
    std::wstring m_proxy_host_name;
    std::wstring m_proxy_user_name;
    int m_proxy_port;
    std::wstring m_proxy_password;
    int m_recheck_delay;
    const Settings &m_settings;

    LRESULT m_lexer;
    std::vector<std::string> m_last_suggestions;
    long m_modified_start;
    long m_modified_end;
    long m_word_under_cursor_pos;
    std::size_t m_word_under_cursor_length;
    LRESULT m_current_position;
    NppData* m_npp_data_instance;
    EncodingType m_current_encoding;
    std::wstring m_ini_file_path;
    std::string m_selected_word;
    SettingsDlg* m_settings_dlg_instance;
    SuggestionsButton* m_suggestions_instance;
    LangList* m_lang_list_instance;
    std::vector<char> m_visible_text;
    long m_visible_text_offset;
    bool m_remove_user_dics;
    bool m_remove_system;

    SpellerInterface* m_current_speller;
    std::unique_ptr<AspellInterface> m_aspell_speller;
    std::unique_ptr<HunspellInterface> m_hunspell_speller;

    std::string m_yo_capital_ansi;
    std::string m_ye_capital_ansi;
    std::string m_yo_ansi;
    std::string m_ye_ansi;
    std::string m_punctuation_apostrophe_ansi;
};
