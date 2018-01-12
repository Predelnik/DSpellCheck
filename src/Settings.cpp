#include "Settings.h"
#include "CommonFunctions.h"
#include "Scintilla.h"
#include "SpellerId.h"
#include "aspell.h"
#include "utils/IniWorker.h"
#include "utils/enum_range.h"
#include "utils/string_utils.h"
#include <cassert>

const wchar_t *default_delimiters() {
  return L",.!?\":;{}()[]\\/"
         L"=+-^$*<>|#$@%&~"
         L"\u2026\u2116\u2014\u00AB\u00BB\u2013\u2022\u00A9\u203A\u201C\u201D"
         L"\u00B7"
         L"\u00A0\u0060\u2192\u00d7";
}

const wchar_t *default_delimiter_exclusions() { return L"'_"; }

std::wstring gui_string(ProxyType value) {
  switch (value) {
  case ProxyType::web_proxy:
    return rc_str(IDS_FTP_WEB_PROXY);
  case ProxyType::ftp_gateway:
    return rc_str(IDS_FTP_GATEWAY);
  case ProxyType::COUNT:
    break;
  }
  return nullptr;
}

std::wstring gui_string(TokenizationStyle value) {
  switch (value) {
  case TokenizationStyle::by_non_alphabetic:
    return rc_str(IDS_SPLIT_WORDS_BY_NON_ALPHA);
  case TokenizationStyle::by_delimiters:
    return rc_str(IDS_SPLIT_WORDS_BY_DELIMS);
  case TokenizationStyle::COUNT:
    break;
  }
  return nullptr;
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
  return nullptr;
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
  default:;
  }
  return nullptr;
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
  default:;
  }
  return nullptr;
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
  default:;
  }
  return nullptr;
}

Settings::Settings(std::wstring_view ini_filepath)
    : m_ini_filepath(ini_filepath) {}

constexpr auto app_name = L"SpellCheck";

void Settings::save() {
  if (m_ini_filepath.empty())
    return;
  FILE *fp;
  auto path = std::wstring_view(m_ini_filepath);
  path.remove_prefix(path.length() - path.rfind(L'\\'));
  check_for_directory_existence(std::wstring(path));
  // Cleaning settings file (or creating it)
  if (_wfopen_s(&fp, m_ini_filepath.c_str(), L"w") != NULL) {
    MessageBox(
        nullptr,
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
  settings_changed();
}

std::wstring &Settings::get_active_language() {
  return speller_language[active_speller_lib_id];
}

const std::wstring &Settings::get_active_language() const {
  return const_cast<Self *>(this)->get_active_language();
}

std::wstring &Settings::get_active_multi_languages() {
  return speller_multi_languages[active_speller_lib_id];
}

const std::wstring &Settings::get_active_multi_languages() const {
  return const_cast<Self *>(this)->get_active_multi_languages();
}

TemporaryAcessor<Settings::Self> Settings::modify() const {
  auto non_const_this = const_cast<Self *>(this);
  return {*non_const_this, [non_const_this]() {
            non_const_this->save();
            non_const_this->settings_changed();
          }};
}

std::wstring Settings::get_default_hunspell_path() {
  return m_ini_filepath.substr(0, m_ini_filepath.rfind(L'\\')) + L"\\Hunspell";
}

void Settings::process(IniWorker &worker) {
  worker.process(L"Aspell_Path", aspell_path, get_default_aspell_path());
  worker.process(L"User_Hunspell_Path", hunspell_user_path,
                 get_default_hunspell_path());
  worker.process(L"System_Hunspell_Path", hunspell_system_path,
                 L".\\plugins\\config\\Hunspell");
  worker.process(L"Aspell_Allow_Run_Together_Words",
                 aspell_allow_run_together_words, false);
  worker.process(L"Suggestions_Control", suggestions_mode,
                 SuggestionMode::context_menu);
  worker.process(L"Autocheck", auto_check_text, true);
  for (auto id : enum_range<SpellerId>()) {
    worker.process(
        wstring_printf(L"%s_Multiple_Languages", to_string(id)).c_str(),
        speller_multi_languages[id], L"");
    worker.process(wstring_printf(L"%s_Language", to_string(id)).c_str(),
                   speller_language[id], default_language(id));
  }
  worker.process(L"Tokenization_Style", tokenization_style,
                 TokenizationStyle::by_non_alphabetic);
  worker.process(L"Split_CamelCase", split_camel_case, false);
  worker.process(L"Delimiter_Exclusions", delimiter_exclusions,
                 default_delimiter_exclusions());
  worker.process(L"Delimiters", delimiters, default_delimiters(), true);
  worker.process(L"Suggestions_Number", suggestion_count, 5);
  worker.process(L"Ignore_Yo", ignore_yo, false);
  worker.process(L"Convert_Single_Quotes_To_Apostrophe", convert_single_quotes,
                 true);
  worker.process(L"Remove_Ending_And_Beginning_Apostrophe",
                 remove_boundary_apostrophes, true);
  worker.process(L"Check_Those_\\_Not_Those", check_those, true);
  worker.process(L"File_Types", file_types, L"*.*");
  worker.process(L"Check_Comments", check_comments, true);
  worker.process(L"Check_String", check_strings, true);
  worker.process(L"Check_Variable_Function_Names", check_variable_functions,
                 false);
  worker.process(L"Underline_Color", underline_color, 0x0000ff); // red
  worker.process(L"Underline_Style", underline_style, INDIC_SQUIGGLE);
  worker.process(L"Ignore_Having_Number", ignore_containing_digit, true);
  worker.process(L"Ignore_Start_Capital", ignore_starting_with_capital, false);
  worker.process(L"Ignore_Have_Capital", ignore_having_a_capital, true);
  worker.process(L"Ignore_All_Capital", ignore_all_capital, true);
  worker.process(L"Ignore_One_Letter", ignore_one_letter, false);
  worker.process(L"Word_Minimum_Length", word_minimum_length, false);
  worker.process(L"Ignore_With_", ignore_having_underscore, true);
  worker.process(L"United_User_Dictionary(Hunspell)", use_unified_dictionary,
                 false);
  worker.process(L"Ignore_That_Start_or_End_with_",
                 ignore_starting_or_ending_with_apostrophe, false);
  worker.process(L"Library", active_speller_lib_id, SpellerId::hunspell);
  worker.process(L"Suggestions_Button_Size", suggestion_button_size, 15);
  worker.process(L"Suggestions_Button_Opacity", suggestion_button_opacity, 70);
  worker.process(L"Find_Next_Buffer_Size", find_next_buffer_size, 4);
  worker.process(L"Show_Only_Known", ftp_show_only_known_dictionaries, true);
  worker.process(L"Install_Dictionaries_For_All_Users",
                 ftp_install_dictionaries_for_all_users, false);
  worker.process(L"Recheck_Delay", recheck_delay, 500);
  for (int i = 0; i < static_cast<int>(server_names.size()); ++i)
    worker.process(wstring_printf(L"Server_Address[%d]", i).c_str(),
                   server_names[i], L"");
  worker.process(L"Last_Used_Address_Index", last_used_address_index, 0);
  worker.process(L"Remove_User_Dics_On_Dic_Remove", remove_user_dictionaries,
                 false);
  worker.process(L"Remove_Dics_For_All_Users", remove_system_dictionaries,
                 false);
  worker.process(L"Decode_Language_Names", use_language_name_aliases, true);
  worker.process(L"Use_Proxy", use_proxy, false);
  worker.process(L"Proxy_User_Name", proxy_user_name, L"anonymous");
  worker.process(L"Proxy_Host_Name", proxy_host_name, L"");
  worker.process(L"Proxy_Password", proxy_password, L"");
  worker.process(L"Proxy_Port", proxy_port, 808);
  worker.process(L"Proxy_Is_Anonymous", proxy_is_anonymous, true);
  worker.process(L"Proxy_Type", proxy_type, ProxyType::web_proxy);
  worker.process(L"Write_Debug_Log", write_debug_log, false);
}
