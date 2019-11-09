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

#include "CommonFunctions.h"
#include "SpellerId.h"
#include "lsignal.h"
#include "utils/TemporaryAcessor.h"
#include "utils/enum_array.h"
#include "utils/string_utils.h"

#include <experimental/string>

class IniWorker;

const wchar_t *default_delimiters();
const wchar_t *default_delimiter_exclusions();

enum class LanguageNameStyle {
  original,
  english,
  native,
  localized,

  COUNT,
};

std::wstring gui_string(LanguageNameStyle value);

enum class SuggestionMode {
  button,
  context_menu,

  // ReSharper disable once CppInconsistentNaming
  COUNT,
};

std::wstring gui_string(SuggestionMode value);

enum class ProxyType {
  web_proxy,
  ftp_gateway,

  // ReSharper disable once CppInconsistentNaming
  COUNT,
};

std::wstring gui_string(ProxyType value);

enum class TokenizationStyle {
  by_non_alphabetic,
  by_non_ansi,
  by_delimiters,

  // ReSharper disable once CppInconsistentNaming
  COUNT,
};

enum class SettingsModificationStyle {
  normal,
  ignore_file_errors,
};

std::wstring gui_string(TokenizationStyle value);

class Settings {
  using Self = Settings;

public:
  Settings(std::wstring_view ini_filepath = L"");
  Settings(const Settings &);
  Settings &operator=(const Settings &);
  Settings(Settings &&);
  Settings &operator=(Settings &&);
  void on_settings_changed();
  void update_processed_delimiters();
  void process(IniWorker &worker);
  auto delimiter_tokenizer(std::wstring_view target) const { return make_delimiter_tokenizer(target, m_processed_delimiters, split_camel_case); }

  auto non_alphabetic_tokenizer(std::wstring_view target) const {
    return Tokenizer(target, [this](wchar_t c) { return !IsCharAlphaNumeric(c) && delimiter_exclusions.find(c) == std ::wstring_view::npos; },
                     split_camel_case);
  }

  template <typename FunctionType> auto do_with_tokenizer(std::wstring_view target, const FunctionType &function) const {
    switch (tokenization_style) {
    case TokenizationStyle::by_non_alphabetic:
      return function(this->non_alphabetic_tokenizer(target));
    case TokenizationStyle::by_non_ansi:
      return function(this->non_ansi_tokenizer(target));
    case TokenizationStyle::by_delimiters:
      return function(this->delimiter_tokenizer(target));
    case TokenizationStyle::COUNT:
      break;
    }
    throw std::runtime_error("Incorrect tokenization style");
  }

  auto non_ansi_tokenizer(std::wstring_view target) const {
    return Tokenizer(target,
                     [this](wchar_t c) {
                       static const auto ansi_str = []() {
                         constexpr auto char_cnt = 256;
                         std::string s;
                         for (int i = 1; i < char_cnt; ++i)
                           s.push_back(static_cast<char>(i));
                         auto ws = to_wstring(s);
                         std::experimental::erase_if(ws, [](wchar_t c) { return !IsCharAlphaNumeric(c); });
                         std::sort(ws.begin(), ws.end());
                         return ws;
                       }();
                       return !std::binary_search(ansi_str.begin(), ansi_str.end(), c) && delimiter_exclusions.find(c) == std ::wstring_view::npos;
                     },
                     split_camel_case);
  }

  void save(SettingsModificationStyle modification_style);
  void load();
  std::wstring &get_active_language();
  const std::wstring &get_active_language() const;
  std::wstring &get_active_multi_languages();
  const std::wstring &get_active_multi_languages() const;
  TemporaryAcessor<Self> modify(SettingsModificationStyle modification_style = SettingsModificationStyle::normal) const;
  TemporaryAcessor<Settings::Self> modify_without_saving() const;
  std::wstring_view get_dictionary_download_path() const;
  void reset_hunspell_lang_to_default();

public:
  // Settings:
  bool auto_check_text = true;
  std::wstring aspell_dll_path;
  std::wstring aspell_personal_dictionary_path;
  std::wstring hunspell_user_path;
  std::wstring hunspell_system_path;
  SuggestionMode suggestions_mode = SuggestionMode::button;
  std::wstring delimiters = default_delimiters();
  int suggestion_count = 0;
  bool ignore_yo = false;
  bool convert_single_quotes = false;
  bool remove_boundary_apostrophes = false;
  bool check_those = false;
  std::wstring file_types;
  bool check_comments = false;
  bool check_strings = false;
  bool check_variable_functions = false;
  int underline_color = 0;
  int underline_style = 0;
  bool ignore_containing_digit = true;
  bool ignore_starting_with_capital = false;
  bool ignore_having_a_capital = true;
  bool ignore_all_capital = false;
  bool ignore_one_letter = false;
  bool ignore_having_underscore = false;
  bool ignore_starting_or_ending_with_apostrophe = false;
  bool use_unified_dictionary = false;
  bool aspell_allow_run_together_words = false;
  bool check_default_udl_style = true;
  SpellerId active_speller_lib_id = SpellerId::aspell;
  int suggestion_button_size = 0;
  int suggestion_button_opacity = 0;
  bool download_show_only_recognized_dictionaries = false;
  bool download_install_dictionaries_for_all_users = false;
  bool ftp_use_passive_mode = true;
  int recheck_delay = 0;
  std::array<std::wstring, 3> server_names;
  int last_used_address_index = 0;
  bool remove_user_dictionaries = false;
  bool remove_system_dictionaries = false;
  bool use_proxy = false;
  int word_minimum_length = 0;
  std::wstring proxy_user_name;
  std::wstring proxy_host_name;
  std::wstring proxy_password;
  int proxy_port = 0;
  bool proxy_is_anonymous = false;
  ProxyType proxy_type = ProxyType::ftp_gateway;
  TokenizationStyle tokenization_style = TokenizationStyle::by_non_alphabetic;
  std::wstring delimiter_exclusions = default_delimiter_exclusions();
  bool split_camel_case = false;
  enum_array<SpellerId, std::wstring> speller_language;
  enum_array<SpellerId, std::wstring> speller_multi_languages;
  bool write_debug_log = false;
  LanguageNameStyle language_name_style = LanguageNameStyle::english;

  // Derivatives:
  std::wstring m_processed_delimiters;

  mutable lsignal::signal<void()> settings_changed;

private:
  std::wstring get_default_hunspell_path();

private:
  std::wstring m_ini_filepath;
};
