#pragma once

#include "lsignal.h"
#include "utils/TemporaryAcessor.h"

class IniWorker;

enum class SpellerId {
    aspell,
    hunspell,

    // ReSharper disable once CppInconsistentNaming
    COUNT,
};

enum class SuggestionMode {
  button,
  context_menu,

  // ReSharper disable once CppInconsistentNaming
  COUNT,
};

const wchar_t *gui_string (SuggestionMode value);

const wchar_t *gui_string (SpellerId value);

class Settings {
    using Self = Settings;
public:
    Settings (std::wstring_view ini_filepath);
    void process(IniWorker& worker);
    void save ();
    void load ();
    std::wstring &get_current_language ();
    const std::wstring &get_current_language () const;
    std::wstring &get_current_multi_languages();
    const std::wstring& get_current_multi_languages() const;
    TemporaryAcessor<Self> modify() const;
public:
    bool auto_check_text = false;
    std::wstring aspell_path;
    std::wstring hunspell_user_path;
    std::wstring hunspell_system_path;
    std::wstring aspell_multi_languages;
    std::wstring hunspell_multi_languages;
    SuggestionMode suggestions_mode = SuggestionMode::button;
    std::wstring aspell_language;
    std::wstring hunspell_language;
    std::string delim_utf8;
    int suggestion_count = 0;
    bool ignore_yo = false;
    bool convert_single_quotes = false;
    bool remove_boundary_apostrophes = false;
    bool check_those = false;
    std::wstring file_types;
    bool check_only_comments_and_strings = false;
    int underline_color = 0;
    int underline_style = 0;
    bool ignore_containing_digit = false;
    bool ignore_starting_with_capital = false;
    bool ignore_having_a_capital = false;
    bool ignore_all_capital = false;
    bool ignore_one_letter = false;
    bool ignore_having_underscore = false;
    bool ignore_starting_or_ending_with_apostrophe = false;
    bool use_unified_dictionary = false;
    bool aspell_allow_run_together_words = false;
    SpellerId active_speller_lib_id = SpellerId::aspell;
    int suggestion_button_size = 0;
    int suggestion_button_opacity = 0;
    int find_next_buffer_size = 0;
    bool ftp_show_only_known_dictionaries = false;
    bool ftp_install_dictionaries_for_all_users = false;
    int recheck_delay = 0;
    std::array<std::wstring, 3> server_names;
    int last_used_address_index = 0;
    bool remove_user_dictionaries = false;
    bool remove_system_dictionaries = false;
    bool use_language_name_aliases = false;
    bool use_proxy = false;
    std::wstring proxy_user_name;
    std::wstring proxy_host_name;
    std::wstring proxy_password;
    int proxy_port = 0;
    bool proxy_is_anonymous = false;
    int proxy_type = 0; // TODO: change to enum

    mutable lsignal::signal<void()> settings_changed;

private:
    std::wstring get_default_hunspell_path();

private:
    std::wstring m_ini_filepath;
};
