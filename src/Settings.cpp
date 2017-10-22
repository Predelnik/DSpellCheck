#include "Settings.h"
#include "utils/IniWorker.h"
#include "aspell.h"
#include "CommonFunctions.h"
#include "Scintilla.h"

Settings::Settings(std::wstring_view ini_filepath) : m_ini_filepath(ini_filepath) {
}

constexpr auto app_name = L"SpellCheck";
constexpr auto default_delimiters =
    L",.!?\":;{}()[]\\/"
    L"=+-^$*<>|#$@%&~"
    L"\u2026\u2116\u2014\u00AB\u00BB\u2013\u2022\u00A9\u203A\u201C\u201D\u00B7"
    L"\u00A0\u0060\u2192\u00d7";

void Settings::save() {
    FILE* fp;
    _wfopen_s(&fp, m_ini_filepath.c_str(), L"w"); // Cleaning settings file (or creating it)
    fclose(fp);
    IniWorker worker(app_name, m_ini_filepath, IniWorker::Action::save);
    process(worker);
}

void Settings::load() {
    IniWorker worker(app_name, m_ini_filepath, IniWorker::Action::load);
    process(worker);
    settings_changed();
}

std::wstring& Settings::get_current_language() {
    switch (active_speller_lib_id) {
    case 0:
        return aspell_language;
    case 1:
        return hunspell_language;
    default:
        break;
    }
    return aspell_language;
}

const std::wstring& Settings::get_current_language() const {
    return const_cast<Self *>(this)->get_current_language();
}

std::wstring& Settings::get_current_multi_languages() {
    switch (active_speller_lib_id) {
    case 0:
        return aspell_multi_languages;
    case 1:
        return hunspell_multi_languages;
    default:
        break;
    }
    return hunspell_multi_languages;
}

const std::wstring& Settings::get_current_multi_languages() const {
    return const_cast<Self *>(this)->get_current_multi_languages();
}

TemporaryAcessor<Settings::Self> Settings::modify() const {
    auto non_const_this = const_cast<Self *>(this);
    return {
        *non_const_this, [non_const_this]()
        {
            non_const_this->save();
            non_const_this->settings_changed();
        }
    };
}

std::wstring Settings::get_default_hunspell_path() {
    return m_ini_filepath.substr(0, m_ini_filepath.rfind(L'\\')) + L"\\Hunspell";
}

void Settings::process(IniWorker& worker) {
    worker.process(L"Aspell_Path", aspell_path, get_default_aspell_path());
    worker.process(L"User_Hunspell_Path", hunspell_path, get_default_hunspell_path());
    worker.process(L"System_Hunspell_Path", additional_hunspell_path, L".\\plugins\\config\\Hunspell");
    worker.process(L"Suggestions_Control", suggestions_mode, 1);
    worker.process(L"Autocheck", auto_check_text, true);
    worker.process(L"Aspell_Multiple_Languages", aspell_multi_languages, L"");
    worker.process(L"Hunspell_Multiple_Languages", hunspell_multi_languages, L"");
    worker.process(L"Aspell_Language", aspell_language, L"en");
    worker.process(L"Hunspell_Language", hunspell_language, L"en_GB");
    worker.process_utf8(L"Delimiters", delim_utf8, to_utf8_string(default_delimiters).c_str(), true);
    worker.process(L"Suggestions_Number", suggestion_count, 5);
    worker.process(L"Ignore_Yo", ignore_yo, false);
    worker.process(L"Convert_Single_Quotes_To_Apostrophe", convert_single_quotes, true);
    worker.process(L"Remove_Ending_And_Beginning_Apostrophe", remove_boundary_apostrophes, true);
    worker.process(L"Check_Those_\\_Not_Those", check_those, true);
    worker.process(L"File_Types", file_types, L"*.*");
    worker.process(L"Check_Only_Comments_And_Strings", check_only_comments_and_strings, true);
    worker.process(L"Underline_Color", underline_color, 0x0000ff); //red
    worker.process(L"Underline_Style", underline_style, INDIC_SQUIGGLE);
    worker.process(L"Ignore_Having_Number", ignore_containing_digit, true);
    worker.process(L"Ignore_Start_Capital", ignore_starting_with_capital, false);
    worker.process(L"Ignore_Have_Capital", ignore_having_a_capital, true);
    worker.process(L"Ignore_All_Capital", ignore_all_capital, true);
    worker.process(L"Ignore_One_Letter", ignore_one_letter, false);
    worker.process(L"Ignore_With_", ignore_having_underscore, true);
    worker.process(L"United_User_Dictionary(Hunspell)", use_unified_dictionary, false);
    worker.process(L"Ignore_That_Start_or_End_with_", ignore_starting_or_ending_with_apostrophe, false);
    worker.process(L"Library", active_speller_lib_id, 1);
    worker.process(L"Suggestions_Button_Size", suggestion_button_size, 15);
    worker.process(L"Suggestions_Button_Opacity", suggestion_button_opacity, 70);
    worker.process(L"Find_Next_Buffer_Size", find_next_buffer_size, 4);
    worker.process(L"Show_Only_Known", ftp_show_only_known_dictionaries, true);
    worker.process(L"Install_Dictionaries_For_All_Users", ftp_install_dictionaries_for_all_users, false);
    worker.process(L"Recheck_Delay", recheck_delay, 500);
    for (int i = 0; i < static_cast<int>(server_names.size()); ++i)
        worker.process(wstring_printf(L"Server_Address[%d]", i).c_str(), server_names[i], L"");
    worker.process(L"Last_Used_Address_Index", last_used_address_index, 0);
    worker.process(L"Remove_User_Dics_On_Dic_Remove", remove_user_dictionaries, false);
    worker.process(L"Remove_Dics_For_All_Users", remove_system_dictionaries, false);
    worker.process(L"Decode_Language_Names", use_language_name_aliases, true);
    worker.process(L"Use_Proxy", use_proxy, false);
    worker.process(L"Proxy_User_Name", proxy_user_name, L"anonymous");
    worker.process(L"Proxy_Host_Name", proxy_host_name, L"");
    worker.process(L"Proxy_Password", proxy_password, L"");
    worker.process(L"Proxy_Port", proxy_port, 808);
    worker.process(L"Proxy_Is_Anonymous", proxy_is_anonymous, true);
    worker.process(L"Proxy_Type", proxy_type, 0);
}
