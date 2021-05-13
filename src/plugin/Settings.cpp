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

#include "Settings.h"

#include "aspell.h"
#include "Scintilla.h"
#include "common/enum_range.h"
#include "common/IniWorker.h"
#include "common/Utility.h"
#include "spellers/SpellerId.h"

#include <cassert>

const wchar_t *default_delimiters() {
  return L",.!?\":;{}()[]\\/"
      L"=+-^$*<>|#$@%&~"
      L"\u2026\u2116\u2014\u00AB\u00BB\u2013\u2022\u00A9\u203A\u201C\u201D"
      L"\u00B7"
      L"\u00A0\u0060\u2192\u00d7";
}

const wchar_t *default_delimiter_exclusions() { return L"'’_"; }

std::wstring gui_string(LanguageNameStyle value) {
  switch (value) {
  case LanguageNameStyle::original:
    return rc_str(IDS_LANGUAGE_NAME_STYLE_ORIGINAL);
  case LanguageNameStyle::english:
    return rc_str(IDS_LANGUAGE_NAME_STYLE_ENGLISH);
  case LanguageNameStyle::localized:
    return rc_str(IDS_LANGUAGE_NAME_STYLE_LOCALIZED);
  case LanguageNameStyle::native:
    return rc_str(IDS_LANGUAGE_NAME_STYLE_NATIVE);
  case LanguageNameStyle::COUNT:
    break;
  }
  throw std::runtime_error("Incorrect LanguageNameStyle value");
}

std::wstring gui_string(ProxyType value) {
  switch (value) {
  case ProxyType::web_proxy:
    return rc_str(IDS_FTP_WEB_PROXY);
  case ProxyType::ftp_gateway:
    return rc_str(IDS_FTP_GATEWAY);
  case ProxyType::COUNT:
    break;
  }
  throw std::runtime_error("Incorrect ProxyType value");
}

std::wstring gui_string(TokenizationStyle value) {
  switch (value) {
  case TokenizationStyle::by_non_alphabetic:
    return rc_str(IDS_SPLIT_WORDS_BY_NON_ALPHA);
  case TokenizationStyle::by_delimiters:
    return rc_str(IDS_SPLIT_WORDS_BY_DELIMS);
  case TokenizationStyle::by_non_ansi:
    return rc_str(IDS_SPLIT_WORDS_BY_NON_ANSI);
  case TokenizationStyle::COUNT:
    break;
  }
  throw std::runtime_error("Incorrect TokenizationStyle value");
}

std::wstring gui_string(SuggestionMode value) {
  switch (value) {
  case SuggestionMode::button:
    return rc_str(IDS_SPECIAL_SUGGESTION_BUTTON);
  case SuggestionMode::context_menu:
    return rc_str(IDS_USE_NPP_CONTEXT_MENU);
  case SuggestionMode::COUNT:
    break;
  }
  throw std::runtime_error("Incorrect SuggestionMode value");
}

std::wstring gui_string(SpellerId value) {
  switch (value) {
  case SpellerId::aspell:
    return rc_str(IDS_ASPELL);
  case SpellerId::hunspell:
    return rc_str(IDS_HUNSPELL);
  case SpellerId::native:
    return rc_str(IDS_NATIVE_WINDOWS);
  case SpellerId::COUNT:
    break;
  }
  throw std::runtime_error("Incorrect SpellerId value");
}

static const wchar_t *to_string(SpellerId value) {
  switch (value) {
  case SpellerId::aspell:
    return L"Aspell";
  case SpellerId::hunspell:
    return L"Hunspell";
  case SpellerId::native:
    return L"Native_Speller";
  case SpellerId::COUNT:
    break;
  }
  throw std::runtime_error("Incorrect SpellerId value");
}

static const wchar_t *default_language(SpellerId value) {
  switch (value) {
  case SpellerId::aspell:
    return L"en";
  case SpellerId::hunspell:
    return L"en_GB";
  case SpellerId::native:
    return L"en-US";
  case SpellerId::COUNT:
    break;
  }
  throw std::runtime_error("Incorrect SpellerId value");
}

Settings::Settings(std::wstring_view ini_filepath)
  : m_ini_filepath(ini_filepath) {
  settings_changed.connect([this] { on_settings_changed(); });
}


void Settings::on_settings_changed() {
  update_processed_delimiters();
}

void Settings::update_processed_delimiters() { data.m_processed_delimiters = L" \n\r\t\v" + parse_string(data.delimiters.c_str()); }

constexpr auto app_name = L"SpellCheck";

void Settings::save(SettingsModificationStyle modification_style) {
  if (m_ini_filepath.empty())
    return;
  FILE *fp;
  auto path = std::wstring_view(m_ini_filepath);
  path.remove_suffix(path.length() - path.rfind(L'\\'));
  check_for_directory_existence(std::wstring(path));
  // Cleaning settings file (or creating it)
  if (_wfopen_s(&fp, m_ini_filepath.c_str(), L"w") != NULL) {
    if (modification_style == SettingsModificationStyle::ignore_file_errors)
      return;

    MessageBox(nullptr,
               wstring_printf(L"Setting file %s cannot be written. All settings will "
                              L"be lost when you close Notepad++.",
                              m_ini_filepath.c_str())
               .c_str(),
               L"Saving of Settings Failed", MB_OK | MB_ICONHAND);
    return;
  }
  write_unicode_bom(fp);
  fclose(fp);
  IniWorker worker(app_name, m_ini_filepath, IniWorker::Action::save);
  process(worker);
}

void Settings::load() {
  if (m_ini_filepath.empty())
    return;
  IniWorker worker(app_name, m_ini_filepath, IniWorker::Action::load);
  process(worker);
  delete_log();
  print_to_log(L"void Settings::load()", nullptr);
  settings_changed();
}

std::wstring &Settings::get_active_language() { return data.speller_language[data.active_speller_lib_id]; }

const std::wstring &Settings::get_active_language() const { return const_cast<Self *>(this)->get_active_language(); }

std::wstring &Settings::get_active_multi_languages() { return data.speller_multi_languages[data.active_speller_lib_id]; }

const std::wstring &Settings::get_active_multi_languages() const { return const_cast<Self *>(this)->get_active_multi_languages(); }

TemporaryAcessor<Settings::Self> Settings::modify(SettingsModificationStyle modification_style) const {
  auto non_const_this = const_cast<Self *>(this);
  return {*non_const_this, [non_const_this, modification_style]() {
    non_const_this->save(modification_style);
    non_const_this->settings_changed();
  }};
}

TemporaryAcessor<Settings::Self> Settings::modify_without_saving() const {
  auto non_const_this = const_cast<Self *>(this);
  return {*non_const_this, [non_const_this]() {
    non_const_this->settings_changed();
  }};
}

std::wstring_view Settings::get_dictionary_download_path() const {
  return data.download_install_dictionaries_for_all_users ? data.hunspell_system_path : data.hunspell_user_path;
}

void Settings::reset_hunspell_lang_to_default() { data.speller_language[SpellerId::hunspell] = default_language(SpellerId::hunspell); }

std::wstring Settings::get_default_hunspell_path() { return m_ini_filepath.substr(0, m_ini_filepath.rfind(L'\\')) + L"\\Hunspell"; }

void Settings::process(IniWorker &worker) {
  worker.process(L"Aspell_Path", data.aspell_dll_path, get_default_aspell_path());
  worker.process(L"Aspell_Personal_Dictionary_Path", data.aspell_personal_dictionary_path, L"");
  worker.process(L"User_Hunspell_Path", data.hunspell_user_path, get_default_hunspell_path());
  worker.process(L"System_Hunspell_Path", data.hunspell_system_path, L".\\plugins\\config\\Hunspell");
  worker.process(L"Aspell_Allow_Run_Together_Words", data.aspell_allow_run_together_words, false);
  worker.process(L"Suggestions_Control", data.suggestions_mode, SuggestionMode::context_menu);
  worker.process(L"Autocheck", data.auto_check_text, true);
  for (auto id : enum_range<SpellerId>()) {
    worker.process(wstring_printf(L"%s_Multiple_Languages", to_string(id)).c_str(), data.speller_multi_languages[id], L"");
    worker.process(wstring_printf(L"%s_Language", to_string(id)).c_str(), data.speller_language[id], default_language(id));
  }
  worker.process(L"Tokenization_Style", data.tokenization_style, TokenizationStyle::by_non_alphabetic);
  worker.process(L"Split_CamelCase", data.split_camel_case, false);
  worker.process(L"Delimiter_Exclusions", data.delimiter_exclusions, default_delimiter_exclusions());
  worker.process(L"Delimiters", data.delimiters, default_delimiters(), true);
  worker.process(L"Suggestions_Number", data.suggestion_count, 5);
  worker.process(L"Ignore_Yo", data.ignore_yo, false);
  worker.process(L"Convert_Single_Quotes_To_Apostrophe", data.convert_single_quotes, true);
  worker.process(L"Remove_Ending_And_Beginning_Apostrophe", data.remove_boundary_apostrophes, true);
  worker.process(L"Check_Those_\\_Not_Those", data.check_those, true);
  worker.process(L"File_Types", data.file_types, L"*.*");
  worker.process(L"Check_Comments", data.check_comments, true);
  worker.process(L"Check_String", data.check_strings, true);
  worker.process(L"Check_Variable_Function_Names", data.check_variable_functions, false);
  worker.process(L"Underline_Color", data.underline_color, 0x0000ff); // red
  worker.process(L"Underline_Style", data.underline_style, INDIC_SQUIGGLE);
  worker.process(L"Ignore_Having_Number", data.ignore_containing_digit, true);
  worker.process(L"Ignore_Start_Capital", data.ignore_starting_with_capital, false);
  worker.process(L"Ignore_Have_Capital", data.ignore_having_a_capital, true);
  worker.process(L"Ignore_All_Capital", data.ignore_all_capital, true);
  worker.process(L"Ignore_One_Letter", data.ignore_one_letter, false);
  worker.process(L"Word_Minimum_Length", data.word_minimum_length, false);
  worker.process(L"Check_Default_UDL_style", data.check_default_udl_style, true);
  worker.process(L"Ignore_With_", data.ignore_having_underscore, true);
  worker.process(L"United_User_Dictionary(Hunspell)", data.use_unified_dictionary, false);
  worker.process(L"Ignore_That_Start_or_End_with_", data.ignore_starting_or_ending_with_apostrophe, false);
  worker.process(L"Library", data.active_speller_lib_id, SpellerId::hunspell);
  worker.process(L"Suggestions_Button_Size", data.suggestion_button_size, 15);
  worker.process(L"Suggestions_Button_Opacity", data.suggestion_button_opacity, 70);
  worker.process(L"Show_Only_Known", data.download_show_only_recognized_dictionaries, false);
  worker.process(L"Install_Dictionaries_For_All_Users", data.download_install_dictionaries_for_all_users, false);
  worker.process(L"Recheck_Delay", data.recheck_delay, 500);
  for (int i = 0; i < static_cast<int>(data.server_names.size()); ++i)
    worker.process(wstring_printf(L"Server_Address[%d]", i).c_str(), data.server_names[i], L"");
  worker.process(L"Last_Used_Address_Index", data.last_used_address_index, 0);
  worker.process(L"Remove_User_Dics_On_Dic_Remove", data.remove_user_dictionaries, false);
  worker.process(L"Remove_Dics_For_All_Users", data.remove_system_dictionaries, false);
  worker.process(L"Language_Name_Style", data.language_name_style, LanguageNameStyle::english);
  if (worker.get_action() == IniWorker::Action::load) {
    bool use_language_name_aliases;
    worker.process(L"Decode_Language_Names", use_language_name_aliases, true);
    if (!use_language_name_aliases)
      data.language_name_style = LanguageNameStyle::original;
  }
  worker.process(L"Use_Proxy", data.use_proxy, false);
  worker.process(L"Proxy_User_Name", data.proxy_user_name, L"anonymous");
  worker.process(L"Proxy_Host_Name", data.proxy_host_name, L"");
  worker.process(L"Proxy_Password", data.proxy_password, L"");
  worker.process(L"Proxy_Port", data.proxy_port, 808);
  worker.process(L"Proxy_Is_Anonymous", data.proxy_is_anonymous, true);
  worker.process(L"Proxy_Type", data.proxy_type, ProxyType::web_proxy);
  worker.process(L"Write_Debug_Log", data.write_debug_log, false);
  worker.process(L"FTP_use_passive_mode", data.ftp_use_passive_mode, true);
}
