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
#include "npp/EditorInterface.h"

class EditorInterface;
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
class NativeSpellerInterface;

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

struct WordToCheck
{
    std::wstring str;
    long word_start;
    long word_end;
    bool is_correct;
};

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
    SpellChecker(NppData* npp_data_instance_arg,
                 SuggestionsButton* suggestions_instance_arg,
                 const Settings *settings,
                 EditorInterface &editor);
    ~SpellChecker();
    void recheck_visible_both_views();
    const SpellerInterface* active_speller() const;
    SpellerInterface* active_speller();
    void show_suggestion_menu();
    void precalculate_menu();
    void recheck_visible(EditorViewType view, bool not_intersection_only = false);
    MappedWstring to_mapped_wstring(EditorViewType view, std::string_view str);

    bool aspell_reinit_settings();
    void update_hunspell_language_options();
    void update_aspell_language_options();
    void update_suggestion_box();

    void init_speller();
    int get_aspell_status();
    void cleanup();
    void do_plugin_menu_inclusion(bool invalidate = false);
    HunspellInterface* get_hunspell_speller() const { return m_hunspell_speller.get(); };
    bool hunspell_reinit_settings(bool reset_directory);
    void set_suggestions_box_transparency();
    void process_menu_result(WPARAM menu_id);
    void copy_misspellings_to_clipboard();
    void update_delimiters();
    void on_settings_changed();
    void init_suggestions_box();
    void hide_suggestion_box();
    void find_next_mistake();
    void find_prev_mistake();
    void check_file_name();

    void lang_change();
    std::vector<LanguageInfo> get_available_languages() const;

private:
    void create_word_underline(EditorViewType view, long start, long end);
    void remove_underline(EditorViewType view, long start, long end);
    void clear_all_underlines(EditorViewType view);
    bool check_word(EditorViewType view, std::wstring word, long word_start, std::vector<WordToCheck>* words_to_check = nullptr);
    void get_visible_limits(EditorViewType view, long& start, long& finish);
    MappedWstring get_visible_text(EditorViewType view, long* offset, bool not_intersection_only = false);
    int check_text(EditorViewType view, const MappedWstring& text_to_check, long offset, CheckTextMode mode, size_t skip_chars = 0);
    void check_visible(EditorViewType view, bool not_intersection_only = false);
    void set_encoding_by_id(int enc_id);
    std::vector<SuggestionsMenuItem> fill_suggestions_menu(HMENU menu);
    bool is_word_under_cursor_correct(long& pos, long& length,
                                        bool use_text_cursor = false);
    std::wstring_view get_word_at(long char_pos, const MappedWstring& text, long offset) const;
    bool check_text_needed();
    void refresh_underline_style();
    void apply_conversions(std::wstring& word);
    void reset_hot_spot_cache();
    void cut_apostrophes(std::wstring_view& word);
    std::optional<long> next_token_end(std::wstring_view target, long index) const;
    std::optional<long> prev_token_begin(std::wstring_view target, long index) const;
    auto non_alphabetic_tokenizer (std::wstring_view target) const;
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
    NppData* m_npp_data_instance;
    MappedWstring m_selected_word;
    SettingsDlg* m_settings_dlg_instance;
    SuggestionsButton* m_suggestions_instance;
    long m_visible_text_offset;
    EditorInterface &m_editor;
    std::wstring_view last_result; // workaround for getting latest misspelling

    std::unique_ptr<AspellInterface> m_aspell_speller;
    std::unique_ptr<HunspellInterface> m_hunspell_speller;
    std::unique_ptr<NativeSpellerInterface> m_native_speller;
};
