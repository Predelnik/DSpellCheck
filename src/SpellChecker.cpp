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

#include "SpellerInterface.h"
#include "AspellInterface.h"
#include "HunspellInterface.h"

#include "aspell.h"
#include "DownloadDicsDlg.h"
#include "iconv.h"
#include "CommonFunctions.h"
#include "LanguageInfo.h"
#include "LangList.h"
#include "MainDef.h"
#include "PluginInterface.h"
#include "Plugin.h"
#include "RemoveDictionariesDialog.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "Scintilla.h"
#include "SelectProxyDialog.h"
#include "SuggestionsButton.h"
#include "SciUtils.h"
#include "utils/string_utils.h"
#include "utils/utf8.h"
#include "resource.h"
#include "Settings.h"

#define DEFAULT_DELIMITERS                                                     \
  L",.!?\":;{}()[]\\/"                                                         \
  L"=+-^$*<>|#$@%&~"                                                           \
  L"\u2026\u2116\u2014\u00AB\u00BB\u2013\u2022\u00A9\u203A\u201C\u201D\u00B7"  \
  L"\u00A0\u0060\u2192\u00d7"

HWND get_scintilla_window(const NppData* npp_data_arg) {
    int which = -1;
    SendMessage(npp_data_arg->npp_handle, NPPM_GETCURRENTSCINTILLA, 0,
                (LPARAM)&which);
    if (which == -1)
        return nullptr;
    if (which == 1)
        return npp_data_arg->scintilla_second_handle;
    return (which == 0)
               ? npp_data_arg->scintilla_main_handle
               : npp_data_arg->scintilla_second_handle;
}

bool send_msg_to_both_editors(const NppData* npp_data_arg, UINT msg, WPARAM w_param,
                              LPARAM l_param) {
    SendMessage(npp_data_arg->scintilla_main_handle, msg, w_param, l_param);
    SendMessage(npp_data_arg->scintilla_second_handle, msg, w_param, l_param);
    return true;
}

LRESULT send_msg_to_editor(HWND scintilla_window, UINT msg,
                           WPARAM w_param /*= 0*/, LPARAM l_param /*= 0*/) {
    return SendMessage(scintilla_window, msg, w_param, l_param);
}

LRESULT send_msg_to_npp(const NppData* npp_data_arg, UINT msg,
                        WPARAM w_param /*= 0*/, LPARAM l_param /*= 0*/) {
    return SendMessage(npp_data_arg->npp_handle, msg, w_param, l_param);
}

// Remember: it's better to use PostMsg wherever possible, to avoid gui update
// on each message send etc etc
// Also it's better to avoid get current scintilla window too much times, since
// it's obviously uses 1 SendMsg call
LRESULT post_msg_to_active_editor(HWND scintilla_window, UINT msg,
                                  WPARAM w_param /*= 0*/, LPARAM l_param /*= 0*/) {
    return PostMessage(scintilla_window, msg, w_param, l_param);
}

SpellChecker::SpellChecker(const wchar_t* ini_file_path_arg,
                           SettingsDlg* settings_dlg_instance_arg,
                           NppData* npp_data_instance_arg,
                           SuggestionsButton* suggestions_instance_arg,
                           LangList* lang_list_instance_arg,
                           const Settings* settings) : m_settings(*settings) {
    m_current_position = 0;
    m_ini_file_path = ini_file_path_arg;
    m_settings_dlg_instance = settings_dlg_instance_arg;
    m_suggestions_instance = suggestions_instance_arg;
    m_npp_data_instance = npp_data_instance_arg;
    m_lang_list_instance = lang_list_instance_arg;
    m_cur_word_list = nullptr;
    m_word_under_cursor_length = 0;
    m_word_under_cursor_pos = 0;
    m_word_under_cursor_is_correct = true;
    m_current_scintilla = get_scintilla_window(m_npp_data_instance);
    m_aspell_speller = std::make_unique<AspellInterface>(m_npp_data_instance->npp_handle);
    m_hunspell_speller = std::make_unique<HunspellInterface>(m_npp_data_instance->npp_handle);
    m_current_speller = m_aspell_speller.get();
    reset_hot_spot_cache();
    m_settings_loaded = false;
    m_settings.settings_changed.connect([this] { on_settings_changed(); });
    bool res =
        (send_msg_to_npp(m_npp_data_instance, NPPM_ALLOCATESUPPORTED, 0, 0) != 0);

    if (res) {
        set_use_allocated_ids(true);
        int id;
        send_msg_to_npp(m_npp_data_instance, NPPM_ALLOCATECMDID, 350, (LPARAM)&id);
        set_context_menu_id_start(id);
        set_langs_menu_id_start(id + 103);
    }
}

SpellChecker::~SpellChecker() {
}

void insert_sugg_menu_item(HMENU menu, const wchar_t* text, BYTE id, int insert_pos,
                           bool separator) {
    MENUITEMINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(MENUITEMINFO);
    if (separator) {
        mi.fType = MFT_SEPARATOR;
    }
    else {
        mi.fType = MFT_STRING;
        mi.fMask = MIIM_ID | MIIM_TYPE;
        if (!get_use_allocated_ids())
            mi.wID = MAKEWORD(id, DSPELLCHECK_MENU_ID);
        else
            mi.wID = get_context_menu_id_start() + id;

        mi.dwTypeData = const_cast<wchar_t *>(text);
        mi.cch = static_cast<int>(wcslen(text)) + 1;
    }
    if (insert_pos == -1)
        InsertMenuItem(menu, GetMenuItemCount(menu), true, &mi);
    else
        InsertMenuItem(menu, insert_pos, true, &mi);
}

void SpellChecker::precalculate_menu() {
    std::vector<SuggestionsMenuItem> suggestion_menu_items;
    if (check_text_needed() && m_settings.suggestions_mode == SuggestionMode::context_menu) {
        long pos, length;
        m_word_under_cursor_is_correct = get_word_under_cursor_is_right(pos, length, true);
        if (!m_word_under_cursor_is_correct) {
            m_word_under_cursor_pos = pos;
            m_word_under_cursor_length = length;
            suggestion_menu_items = fill_suggestions_menu(nullptr);
        }
    }
    show_calculated_menu(std::move(suggestion_menu_items));
}

int SpellChecker::get_aspell_status() {
    return m_aspell_speller->is_working()
               ? 2 - (get_available_languages().empty() ? 1 : 0)
               : 0;
}

void SpellChecker::recheck_visible_both_views() {
    LRESULT old_lexer = m_lexer;
    EncodingType old_encoding = m_current_encoding;
    m_lexer = send_msg_to_editor(m_npp_data_instance->scintilla_main_handle, SCI_GETLEXER);
    m_current_scintilla = m_npp_data_instance->scintilla_main_handle;
    recheck_visible();

    m_current_scintilla = m_npp_data_instance->scintilla_second_handle;
    m_lexer = send_msg_to_editor(m_npp_data_instance->scintilla_second_handle, SCI_GETLEXER);
    recheck_visible();
    m_lexer = old_lexer;
    m_current_encoding = old_encoding;
    m_aspell_speller->set_encoding(m_current_encoding);
    m_hunspell_speller->set_encoding(m_current_encoding);
}

const SpellerInterface* SpellChecker::active_speller() const {
    switch (m_settings.active_speller_lib_id) {
    case SpellerId::aspell:
        return m_aspell_speller.get();
    case SpellerId::hunspell:
        return m_hunspell_speller.get();
    }
    return nullptr;
}

void SpellChecker::apply_proxy_settings() {
    get_select_proxy()->apply_choice();
    save_settings();
}

void SpellChecker::show_suggestion_menu() {
    fill_suggestions_menu(m_suggestions_instance->get_popup_menu());
    SendMessage(m_suggestions_instance->getHSelf(), WM_SHOWANDRECREATEMENU, 0, 0);
}

void SpellChecker::update_from_remove_dics_options() {
    get_remove_dics()->update_options();
    save_settings();
}

void SpellChecker::lang_change() {
    m_lexer = send_msg_to_editor(get_current_scintilla(), SCI_GETLEXER);
    recheck_visible();
}

void SpellChecker::do_plugin_menu_inclusion(bool invalidate) {
    MENUITEMINFO mif;
    HMENU dspellcheck_menu = get_dspellcheck_menu();
    if (!dspellcheck_menu)
        return;
    HMENU langs_sub_menu = get_langs_sub_menu(dspellcheck_menu);
    if (langs_sub_menu)
        DestroyMenu(langs_sub_menu);
    auto cur_lang = m_settings.get_current_language();
    HMENU new_menu = CreatePopupMenu();
    if (!invalidate) {
        auto langs = get_available_languages();
        if (!langs.empty()) {
            int i = 0;
            for (auto& lang : langs) {
                int checked = (cur_lang == lang.orig_name)
                                  ? (MFT_RADIOCHECK | MF_CHECKED)
                                  : MF_UNCHECKED;
                bool res = AppendMenu(new_menu, MF_STRING | checked,
                                      get_use_allocated_ids()
                                          ? i + get_langs_menu_id_start()
                                          : MAKEWORD(i, LANGUAGE_MENU_ID),
                                      m_settings.use_language_name_aliases
                                          ? lang.alias_name.c_str()
                                          : lang.orig_name.c_str());
                if (!res)
                    return;
                ++i;
            }
            int checked = (cur_lang == L"<MULTIPLE>")
                              ? (MFT_RADIOCHECK | MF_CHECKED)
                              : MF_UNCHECKED;
            AppendMenu(new_menu, MF_STRING | checked,
                       get_use_allocated_ids()
                           ? MULTIPLE_LANGS + get_langs_menu_id_start()
                           : MAKEWORD(MULTIPLE_LANGS, LANGUAGE_MENU_ID),
                       L"Multiple Languages");
            AppendMenu(new_menu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(new_menu, MF_STRING,
                       get_use_allocated_ids()
                           ? CUSTOMIZE_MULTIPLE_DICS + get_langs_menu_id_start()
                           : MAKEWORD(CUSTOMIZE_MULTIPLE_DICS, LANGUAGE_MENU_ID),
                       L"Set Multiple Languages...");
            if (m_settings.active_speller_lib_id == SpellerId::hunspell) // Only Hunspell supported
            {
                AppendMenu(new_menu, MF_STRING,
                           get_use_allocated_ids()
                               ? DOWNLOAD_DICS + get_langs_menu_id_start()
                               : MAKEWORD(DOWNLOAD_DICS, LANGUAGE_MENU_ID),
                           L"Download More Languages...");
                AppendMenu(new_menu, MF_STRING,
                           get_use_allocated_ids()
                               ? REMOVE_DICS + get_langs_menu_id_start()
                               : MAKEWORD(REMOVE_DICS, LANGUAGE_MENU_ID),
                           L"Remove Unneeded Languages...");
            }
        }
        else if (m_settings.active_speller_lib_id == SpellerId::hunspell)
            AppendMenu(new_menu, MF_STRING,
                       get_use_allocated_ids()
                           ? DOWNLOAD_DICS + get_langs_menu_id_start()
                           : MAKEWORD(DOWNLOAD_DICS, LANGUAGE_MENU_ID),
                       L"Download Languages...");
    }

    mif.fMask = MIIM_SUBMENU | MIIM_STATE;
    mif.cbSize = sizeof(MENUITEMINFO);
    mif.hSubMenu = new_menu;
    mif.fState = MFS_ENABLED;

    SetMenuItemInfo(dspellcheck_menu, QUICK_LANG_CHANGE_ITEM, true, &mif);
}

void SpellChecker::check_file_name() {
    wchar_t full_path[MAX_PATH];
    m_check_text_enabled = !m_settings.check_those;
    send_msg_to_npp(m_npp_data_instance, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)full_path);
    for (auto token : tokenize<wchar_t>(m_settings.file_types, LR"(;)")) {
        if (m_settings.check_those) {
            m_check_text_enabled = m_check_text_enabled || PathMatchSpec(full_path, std::wstring(token).c_str());
            if (m_check_text_enabled)
                break;
        }
        else {
            m_check_text_enabled &= m_check_text_enabled && (!PathMatchSpec(full_path, std::wstring(token).c_str()));
            if (!m_check_text_enabled)
                break;
        }
    }

    m_lexer = send_msg_to_editor(get_current_scintilla(), SCI_GETLEXER);
}

LRESULT SpellChecker::get_style(int pos) {
    return send_msg_to_editor(get_current_scintilla(), SCI_GETSTYLEAT, pos);
}

bool SpellChecker::check_text_needed() {
    return m_check_text_enabled && m_settings.auto_check_text;
}

void SpellChecker::hide_suggestion_box() { m_suggestions_instance->display(false); }

void SpellChecker::find_next_mistake() {
    m_current_position =
        send_msg_to_editor(get_current_scintilla(), SCI_GETCURRENTPOS);
    auto cur_line = send_msg_to_editor(get_current_scintilla(), SCI_LINEFROMPOSITION,
                                       m_current_position);
    auto line_start_pos = send_msg_to_editor(get_current_scintilla(), SCI_POSITIONFROMLINE,
                                             cur_line);
    auto doc_length =
        send_msg_to_editor(get_current_scintilla(), SCI_GETLENGTH);
    auto iterator_pos = line_start_pos;
    Sci_TextRange range;
    bool full_check = false;

    while (true) {
        range.chrg.cpMin = static_cast<long>(iterator_pos);
        range.chrg.cpMax = static_cast<long>(iterator_pos + m_settings.find_next_buffer_size * 1024);
        int ignore_offsetting = 0;
        if (range.chrg.cpMax > doc_length) {
            ignore_offsetting = 1;
            range.chrg.cpMax = static_cast<long>(doc_length);
        }
        std::vector<char> text(range.chrg.cpMax - range.chrg.cpMin + 1 + 1);
        range.lpstrText = text.data();
        send_msg_to_editor(get_current_scintilla(), SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&range));
        char* iterating_start =
            range.lpstrText + range.chrg.cpMax - range.chrg.cpMin - 1;
        char* iterating_char = iterating_start;
        if (!ignore_offsetting) {
            if (m_current_encoding == EncodingType::utf8) {
                while (utf8_is_cont(*iterating_char) && range.lpstrText < iterating_char)
                    iterating_char--;

                while ((!utf8_chr(m_delim_utf8_converted.c_str(), iterating_char)) &&
                    range.lpstrText < iterating_char) {
                    iterating_char = utf8_dec(range.lpstrText, iterating_char);
                }
            }
            else {
                while (!strchr(m_delim_converted.c_str(), *iterating_char) &&
                    range.lpstrText < iterating_char)
                    iterating_char--;
            }

            *iterating_char = '\0';
        }
        send_msg_to_editor(get_current_scintilla(), SCI_COLOURISE, range.chrg.cpMin,
                           range.chrg.cpMax);
        SCNotification scn;
        scn.nmhdr.code = SCN_SCROLLED;
        send_msg_to_npp(m_npp_data_instance, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scn));
        // To fix bug with hotspots being removed
        bool result = check_text(to_mapped_wstring(range.lpstrText), static_cast<long>(iterator_pos), CheckTextMode::find_first);
        if (result)
            break;

        iterator_pos += (m_settings.find_next_buffer_size * 1024 + iterating_char - iterating_start);

        if (iterator_pos > doc_length) {
            if (!full_check) {
                m_current_position = 0;
                iterator_pos = 0;
                full_check = true;
            }
            else
                break;

            if (full_check && iterator_pos > m_current_position)
                break; // So nothing was found TODO: Message probably
        }
    }
}

void SpellChecker::find_prev_mistake() {
    m_current_position =
        send_msg_to_editor(get_current_scintilla(), SCI_GETCURRENTPOS);
    auto cur_line = send_msg_to_editor(get_current_scintilla(), SCI_LINEFROMPOSITION,
                                       m_current_position);
    auto doc_length =
        send_msg_to_editor(get_current_scintilla(), SCI_GETLENGTH);
    auto line_end_pos = send_msg_to_editor(get_current_scintilla(), SCI_GETLINEENDPOSITION,
                                           cur_line);

    auto iterator_pos = line_end_pos;
    Sci_TextRange range;
    bool full_check = false;

    while (true) {
        range.chrg.cpMin = static_cast<long>(iterator_pos - m_settings.find_next_buffer_size * 1024);
        range.chrg.cpMax = static_cast<long>(iterator_pos);
        int ignore_offsetting = 0;
        if (range.chrg.cpMin < 0) {
            range.chrg.cpMin = 0;
            ignore_offsetting = 1;
        }
        std::vector<char> text(range.chrg.cpMax - range.chrg.cpMin + 1 + 1);
        range.lpstrText = text.data();
        send_msg_to_editor(get_current_scintilla(), SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&range));
        char* iterating_start = range.lpstrText;
        char* iterating_char = iterating_start;
        if (!ignore_offsetting) {
            if (m_current_encoding == EncodingType::utf8) {
                while (utf8_is_cont(*iterating_char) && *iterating_char)
                    iterating_char++;

                while ((!utf8_chr(m_delim_utf8_converted.c_str(), iterating_char)) &&
                    *iterating_char) {
                    iterating_char = utf8_inc(iterating_char);
                }
            }
            else {
                while (!strchr(m_delim_converted.c_str(), *iterating_char) && iterating_char)
                    iterating_char++;
            }
        }
        auto offset = iterating_char - iterating_start;
        send_msg_to_editor(get_current_scintilla(), SCI_COLOURISE, range.chrg.cpMin + offset,
                           range.chrg.cpMax);
        SCNotification scn;
        scn.nmhdr.code = SCN_SCROLLED;
        send_msg_to_npp(m_npp_data_instance, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scn));

        bool result = check_text(to_mapped_wstring (range.lpstrText + offset),
                                 static_cast<long>(range.chrg.cpMin + offset), CheckTextMode::find_last);
        if (result)
            break;

        iterator_pos -= (m_settings.find_next_buffer_size * 1024 - offset);

        if (iterator_pos < 0) {
            if (!full_check) {
                m_current_position = doc_length + 1;
                iterator_pos = doc_length;
                full_check = true;
            }
            else
                break;

            if (full_check && iterator_pos < m_current_position - 1)
                break; // So nothing was found TODO: Message probably
        }
    }
}

void SpellChecker::set_default_delimiters() {
    m_settings_dlg_instance->get_advanced_dlg()->set_delimeters_edit(DEFAULT_DELIMITERS);
}

HWND SpellChecker::get_current_scintilla() {
    return get_scintilla_window(m_npp_data_instance); // TODO: optimize
}

bool SpellChecker::get_word_under_cursor_is_right(long& pos, long& length,
                                                  bool use_text_cursor) {
    bool ret = true;
    POINT p;
    std::ptrdiff_t init_char_pos;
    LRESULT selection_start = 0;
    LRESULT selection_end = 0;

    if (!use_text_cursor) {
        if (GetCursorPos(&p) == 0)
            return true;

        auto* scintilla = get_scintilla_window(m_npp_data_instance);
        if (!scintilla)
            return true;
        ScreenToClient(scintilla, &p);

        init_char_pos = send_msg_to_editor(
            get_current_scintilla(), SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);
    }
    else {
        selection_start = send_msg_to_editor(get_current_scintilla(), SCI_GETSELECTIONSTART
        );
        selection_end =
            send_msg_to_editor(get_current_scintilla(), SCI_GETSELECTIONEND);
        init_char_pos =
            send_msg_to_editor(get_current_scintilla(), SCI_GETCURRENTPOS);
    }

    if (init_char_pos != -1) {
        auto line = send_msg_to_editor(get_current_scintilla(), SCI_LINEFROMPOSITION,
                                       init_char_pos);
        auto line_length =
            send_msg_to_editor(get_current_scintilla(), SCI_LINELENGTH, line);
        std::vector<char> buf(line_length + 1);
        send_msg_to_editor(get_current_scintilla(), SCI_GETLINE, line, reinterpret_cast<LPARAM>(buf.data()));
        buf[line_length] = 0;
        auto offset = send_msg_to_editor(get_current_scintilla(), SCI_POSITIONFROMLINE,
                                         line);
        auto word = get_word_at(static_cast<long>(init_char_pos), buf.data(),
                                static_cast<long>(offset));
        if (word.empty()) {
            ret = true;
        }
        else {
            auto mapped_str = to_mapped_wstring(word);
            auto sv = std::wstring_view (mapped_str.str);
            cut_apostrophes(sv);
            pos = static_cast<long>(sv.data() - mapped_str.str.data() + offset);
            long pos_end = pos + static_cast<long>(sv.length());
            long word_len = pos_end - pos;
            if (selection_start != selection_end &&
                (selection_start != pos || selection_end != pos + word_len)) {
                return true;
            }
            if (check_word(mapped_str.str, pos, pos + word_len - 1)) {
                ret = true;
            }
            else {
                ret = false;
                length = word_len;
            }
        }
    }
    return ret;
}

std::string_view SpellChecker::get_word_at(long char_pos, char* text, long offset) const {
    char* iterator = text + char_pos - offset;

    if (m_current_encoding == EncodingType::utf8) {
        if (utf8_chr(m_delim_utf8_converted.c_str(), iterator))
            iterator = utf8_dec(text, iterator);

        while ((!utf8_chr(m_delim_utf8_converted.c_str(), iterator)) && text < iterator)
            iterator = utf8_dec(text, iterator);
    }
    else {
        if (strchr(m_delim_converted.c_str(), *iterator))
            iterator--;
        if (iterator < text)
            return nullptr;

        while (!strchr(m_delim_converted.c_str(), *iterator) && text < iterator)
            iterator--;

        if (iterator < text)
            return nullptr;
    }

    char* used_text = iterator;

    if (m_current_encoding == EncodingType::utf8) {
        if (utf8_chr(m_delim_utf8_converted.c_str(), used_text))
            used_text = utf8_inc(used_text); // Then find first token after this zero
    }
    else {
        if (strchr(m_delim_converted.c_str(), *used_text))
            used_text++;
    }

    // We're just taking the first token (basically repeating the same code as an
    // in CheckVisible

    std::string_view res;
    if (m_current_encoding == EncodingType::utf8) {
        auto end = utf8_pbrk(used_text, m_delim_utf8_converted.c_str());
        if (!end) return used_text; // if we didn't meet any delimiters until the end of line
        res = {used_text, static_cast<size_t>(end - used_text)};
    }
    else {
        auto end = strpbrk(used_text, m_delim_converted.c_str());
        if (!end) return {};
        res = {used_text, static_cast<size_t>(end - used_text)};
    }
    if (res.data() - text + offset > char_pos)
        return {};
    else
        return res;
}

void SpellChecker::set_suggestions_box_transparency() {
    // Set WS_EX_LAYERED on this window
    SetWindowLong(m_suggestions_instance->getHSelf(), GWL_EXSTYLE,
                  GetWindowLong(m_suggestions_instance->getHSelf(), GWL_EXSTYLE) |
                  WS_EX_LAYERED);
    SetLayeredWindowAttributes(m_suggestions_instance->getHSelf(), 0,
                               static_cast<BYTE>((255 * m_settings.suggestion_button_opacity) / 100),
                               LWA_ALPHA);
    m_suggestions_instance->display(true);
    m_suggestions_instance->display(false);
}

void SpellChecker::init_suggestions_box() {
    if (m_settings.suggestions_mode != SuggestionMode::button)
        return;
    if (!m_current_speller->is_working())
        return;
    POINT p;
    if (!check_text_needed()) // If there's no red underline let's do nothing
    {
        m_suggestions_instance->display(false);
        return;
    }

    GetCursorPos(&p);
    auto* scintilla = get_scintilla_window(m_npp_data_instance);
    if (!scintilla || WindowFromPoint(p) != scintilla) {
        return;
    }

    long pos, length;
    if (get_word_under_cursor_is_right(pos, length)) {
        return;
    }
    m_word_under_cursor_length = length;
    m_word_under_cursor_pos = pos;
    auto line = send_msg_to_editor(get_current_scintilla(), SCI_LINEFROMPOSITION,
                                   m_word_under_cursor_pos);
    auto text_height =
        send_msg_to_editor(get_current_scintilla(), SCI_TEXTHEIGHT, line);
    auto x_pos = send_msg_to_editor(get_current_scintilla(), SCI_POINTXFROMPOSITION,
                                    0, m_word_under_cursor_pos);
    auto y_pos = send_msg_to_editor(get_current_scintilla(), SCI_POINTYFROMPOSITION,
                                    0, m_word_under_cursor_pos);

    p.x = static_cast<LONG>(x_pos);
    p.y = static_cast<LONG>(y_pos);
    RECT r;
    GetWindowRect(get_current_scintilla(), &r);
    scintilla = get_scintilla_window(m_npp_data_instance);
    if (!scintilla)
        return;

    ClientToScreen(scintilla, &p);
    if (r.top > p.y + text_height - 3 || r.left > p.x ||
        r.bottom < p.y + text_height - 3 + m_settings.suggestion_button_size || r.right < p.x + m_settings.
        suggestion_button_size)
        return;
    MoveWindow(m_suggestions_instance->getHSelf(), p.x,
               p.y + static_cast<int>(text_height) - 3, m_settings.suggestion_button_size,
               m_settings.suggestion_button_size, true);
    m_suggestions_instance->display(true, false);
}

void SpellChecker::process_menu_result(WPARAM menu_id) {
    if ((!get_use_allocated_ids() && HIBYTE(menu_id) != DSPELLCHECK_MENU_ID &&
            HIBYTE(menu_id) != LANGUAGE_MENU_ID) ||
        (get_use_allocated_ids() && ((int)menu_id < get_context_menu_id_start() ||
            (int)menu_id > get_context_menu_id_start() + 350)))
        return;
    int used_menu_id;
    if (get_use_allocated_ids()) {
        used_menu_id = ((int)menu_id < get_langs_menu_id_start()
                            ? DSPELLCHECK_MENU_ID
                            : LANGUAGE_MENU_ID);
    }
    else {
        used_menu_id = HIBYTE(menu_id);
    }

    switch (used_menu_id) {
    case DSPELLCHECK_MENU_ID:
        {
            WPARAM result;
            if (!get_use_allocated_ids())
                result = LOBYTE(menu_id);
            else
                result = menu_id - get_context_menu_id_start();

            if (result != 0) {
                if (result == MID_IGNOREALL) {
                    apply_conversions(m_selected_word.str);
                    m_current_speller->ignore_all(m_selected_word.str.c_str());
                    m_word_under_cursor_length = m_selected_word.str.length();
                    send_msg_to_editor(get_current_scintilla(), SCI_SETSEL,
                                       m_word_under_cursor_pos + m_word_under_cursor_length,
                                       m_word_under_cursor_pos + m_word_under_cursor_length);
                    recheck_visible_both_views();
                }
                else if (result == MID_ADDTODICTIONARY) {
                    apply_conversions(m_selected_word.str);
                    m_current_speller->add_to_dictionary(m_selected_word.str.c_str());
                    m_word_under_cursor_length = m_selected_word.str.length();
                    send_msg_to_editor(get_current_scintilla(), SCI_SETSEL,
                                       m_word_under_cursor_pos + m_word_under_cursor_length,
                                       m_word_under_cursor_pos + m_word_under_cursor_length);
                    recheck_visible_both_views();
                }
                else if ((unsigned int)result <= m_last_suggestions.size()) {
                    std::string encoded_str;
                    if (m_current_encoding == EncodingType::ansi)
                        encoded_str = to_string(m_last_suggestions[result - 1].c_str());
                    else
                        encoded_str = to_utf8_string(m_last_suggestions[result - 1]);

                    send_msg_to_editor(get_current_scintilla(), SCI_REPLACESEL, 0,
                                       reinterpret_cast<LPARAM>(encoded_str.c_str()));
                }
            }
        }
        break;
    case LANGUAGE_MENU_ID:
        {
            WPARAM result;
            if (!get_use_allocated_ids())
                result = LOBYTE(menu_id);
            else
                result = menu_id - get_langs_menu_id_start();

            std::wstring lang_string;
            if (result == MULTIPLE_LANGS) {
                lang_string = L"<MULTIPLE>";
            }
            else if (result == CUSTOMIZE_MULTIPLE_DICS || result == DOWNLOAD_DICS ||
                result == REMOVE_DICS) {
                // All actions are done in GUI thread in that case
                return;
            }
            else
                lang_string = get_available_languages()[result].orig_name;
            do_plugin_menu_inclusion(true);

            auto mut_settings = m_settings.modify();
            mut_settings->get_current_language() = lang_string;
            break;
        }
    default:
        break;
    }
}

std::vector<SuggestionsMenuItem> SpellChecker::fill_suggestions_menu(HMENU menu) {
    if (!m_current_speller->is_working())
        return {}; // Word is already off-screen

    int pos = m_word_under_cursor_pos;
    Sci_TextRange range;
    range.chrg.cpMin = m_word_under_cursor_pos;
    range.chrg.cpMax = m_word_under_cursor_pos + static_cast<long>(m_word_under_cursor_length);
    std::vector<char> text(m_word_under_cursor_length + 1);
    range.lpstrText = text.data();
    post_msg_to_active_editor(get_current_scintilla(), SCI_SETSEL, pos,
                              pos + m_word_under_cursor_length);
    std::vector<SuggestionsMenuItem> suggestion_menu_items;
    send_msg_to_editor(get_current_scintilla(), SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&range));

    m_selected_word = to_mapped_wstring (range.lpstrText);
    apply_conversions(m_selected_word.str);

    m_last_suggestions = m_current_speller->get_suggestions(m_selected_word.str.c_str ());

    for (int i = 0; i < static_cast<int>(m_last_suggestions.size()); i++) {
        if (i >= m_settings.suggestion_count)
            break;

        auto item = m_last_suggestions[i].c_str ();
        if (m_settings.suggestions_mode == SuggestionMode::button)
            insert_sugg_menu_item(menu, item, static_cast<BYTE>(i + 1), -1);
        else
            suggestion_menu_items.emplace_back(item, static_cast<BYTE>(i + 1));
    }

    if (!m_last_suggestions.empty()) {
        if (m_settings.suggestions_mode == SuggestionMode::button)
            insert_sugg_menu_item(menu, L"", 0, 103, true);
        else
            suggestion_menu_items.emplace_back(L"", 0, true);
    }

    auto mapped_str = to_mapped_wstring(range.lpstrText);
    apply_conversions(mapped_str.str);
    auto menu_string = wstring_printf(L"Ignore \"%s\" for Current Session", mapped_str.str.c_str());
    if (m_settings.suggestions_mode == SuggestionMode::button)
        insert_sugg_menu_item(menu, menu_string.c_str(), MID_IGNOREALL, -1);
    else
        suggestion_menu_items.emplace_back(menu_string.c_str(), MID_IGNOREALL);
    menu_string = wstring_printf(L"Add \"%s\" to Dictionary", mapped_str.str.c_str());;
    if (m_settings.suggestions_mode == SuggestionMode::button)
        insert_sugg_menu_item(menu, menu_string.c_str(), MID_ADDTODICTIONARY, -1);
    else
        suggestion_menu_items.emplace_back(menu_string.c_str(), MID_ADDTODICTIONARY);

    if (m_settings.suggestions_mode == SuggestionMode::context_menu)
        suggestion_menu_items.emplace_back(L"", 0, true);

    return suggestion_menu_items;
}

void SpellChecker::refresh_underline_style() {
    send_msg_to_both_editors(m_npp_data_instance, SCI_INDICSETSTYLE, SCE_ERROR_UNDERLINE,
                             m_settings.underline_style);
    send_msg_to_both_editors(m_npp_data_instance, SCI_INDICSETFORE, SCE_ERROR_UNDERLINE,
                             m_settings.underline_color);
}

std::wstring SpellChecker::get_default_hunspell_path() {
    return m_ini_file_path.substr(0, m_ini_file_path.rfind(L'\\')) + L"\\Hunspell";
}

void SpellChecker::save_settings() {
    FILE* fp;
    _wfopen_s(&fp, m_ini_file_path.c_str(), L"w"); // Cleaning settings file (or creating it)
    fclose(fp);
    if (!m_settings_loaded)
        return;
    auto path = get_default_aspell_path();

}

void SpellChecker::init_speller() {
    switch (m_settings.active_speller_lib_id) {

    case SpellerId::aspell:
        aspell_reinit_settings();
        m_current_speller = m_aspell_speller.get();
        break;
    case SpellerId::hunspell:
        m_current_speller = m_hunspell_speller.get();
        hunspell_reinit_settings(false);
        break;
    }
}

void SpellChecker::on_settings_changed() {
    m_hunspell_speller->set_use_one_dic(m_settings.use_unified_dictionary);
    send_msg_to_npp(m_npp_data_instance, NPPM_SETMENUITEMCHECK, get_func_item()[0].cmd_id,
                    m_settings.auto_check_text);
    m_settings_loaded = true;
    update_aspell_language_options();
    update_hunspell_language_options();
    m_hunspell_speller->set_directory(m_settings.hunspell_user_path.c_str());
    m_hunspell_speller->set_additional_directory(m_settings.hunspell_system_path.c_str());
    m_aspell_speller->init(m_settings.aspell_path.c_str());
    int x;
    load_from_ini(x, L"Library", 1);
    init_speller();
    int size, trans;
    load_from_ini(size, L"Suggestions_Button_Size", 15);
    load_from_ini(trans, L"Suggestions_Button_Opacity", 70);
    update_suggestion_box();
    load_from_ini(size, L"Find_Next_Buffer_Size", 4);
    refresh_underline_style();
    check_file_name();
    refresh_underline_style();
    recheck_visible_both_views();
    do_plugin_menu_inclusion();
    get_download_dics()->update_list_box();
    speller_status_changed();
}

void SpellChecker::create_word_underline(HWND scintilla_window, long start,
                                         long end) {
    post_msg_to_active_editor(scintilla_window, SCI_SETINDICATORCURRENT,
                              SCE_ERROR_UNDERLINE);
    post_msg_to_active_editor(scintilla_window, SCI_INDICATORFILLRANGE, start,
                              (end - start + 1));
}

void SpellChecker::remove_underline(HWND scintilla_window, long start, long end) {
    if (end < start)
        return;
    post_msg_to_active_editor(scintilla_window, SCI_SETINDICATORCURRENT,
                              SCE_ERROR_UNDERLINE);
    post_msg_to_active_editor(scintilla_window, SCI_INDICATORCLEARRANGE, start,
                              (end - start + 1));
}

void SpellChecker::get_visible_limits(long& start, long& finish) {
    auto top = send_msg_to_editor(get_current_scintilla(), SCI_GETFIRSTVISIBLELINE
    );
    auto bottom = top + send_msg_to_editor(get_current_scintilla(), SCI_LINESONSCREEN
    );
    top = send_msg_to_editor(get_current_scintilla(), SCI_DOCLINEFROMVISIBLE,
                             top);

    bottom = send_msg_to_editor(get_current_scintilla(), SCI_DOCLINEFROMVISIBLE,
                                bottom);
    auto line_count =
        send_msg_to_editor(get_current_scintilla(), SCI_GETLINECOUNT);
    start = static_cast<long>(send_msg_to_editor(get_current_scintilla(), SCI_POSITIONFROMLINE,
                                                 top));
    // Not using end of line position cause utf-8 symbols could be more than one
    // char
    // So we use next line start as the end of our visible text
    if (bottom + 1 < line_count) {
        finish = static_cast<long>(send_msg_to_editor(
            get_current_scintilla(), SCI_POSITIONFROMLINE, bottom + 1));
    }
    else {
        finish = static_cast<long>(
            send_msg_to_editor(get_current_scintilla(), SCI_GETTEXTLENGTH));
    }
    return;
}

MappedWstring SpellChecker::get_visible_text(long* offset, bool not_intersection_only) {
    Sci_TextRange range;
    get_visible_limits(range.chrg.cpMin, range.chrg.cpMax);

    if (range.chrg.cpMax < 0 || range.chrg.cpMin > range.chrg.cpMax)
        return {};

    previous_a = range.chrg.cpMin;
    previous_b = range.chrg.cpMax;

    if (not_intersection_only) {
        if (range.chrg.cpMin < previous_a && range.chrg.cpMax >= previous_a)
            range.chrg.cpMax = previous_a - 1;
        else if (range.chrg.cpMax > previous_b && range.chrg.cpMin <= previous_b)
            range.chrg.cpMin = previous_b + 1;
    }

    std::vector<char> buf(range.chrg.cpMax - range.chrg.cpMin + 1);
    range.lpstrText = buf.data();
    send_msg_to_editor(get_current_scintilla(), SCI_GETTEXTRANGE, NULL, reinterpret_cast<LPARAM>(&range));
    *offset = range.chrg.cpMin;
    buf[range.chrg.cpMax - range.chrg.cpMin] = 0;
    return to_mapped_wstring(buf.data());
}

void SpellChecker::clear_all_underlines() {
    auto length =
        send_msg_to_editor(get_current_scintilla(), SCI_GETLENGTH);
    if (length > 0) {
        post_msg_to_active_editor(get_current_scintilla(), SCI_SETINDICATORCURRENT,
                                  SCE_ERROR_UNDERLINE);
        post_msg_to_active_editor(get_current_scintilla(), SCI_INDICATORCLEARRANGE, 0,
                                  length);
    }
}

void SpellChecker::save_to_ini(const wchar_t* name, const wchar_t* value,
                               const wchar_t* default_value, bool in_quotes) {
    if (!name || !value)
        return;

    if (default_value && wcscmp(value, default_value) == 0)
        return;

    if (in_quotes) {
        WritePrivateProfileString(L"SpellCheck", name, wstring_printf(LR"("%s")", value).c_str(),
                                  m_ini_file_path.c_str());
    }
    else {
        WritePrivateProfileString(L"SpellCheck", name, value, m_ini_file_path.c_str());
    }
}

void SpellChecker::save_to_ini(const wchar_t* name, int value, int default_value) {
    if (!name)
        return;

    if (value == default_value)
        return;

    wchar_t buf[DEFAULT_BUF_SIZE];
    _itow_s(value, buf, 10);
    save_to_ini(name, buf, nullptr);
}

void SpellChecker::save_to_ini_utf8(const wchar_t* name, const char* value,
                                    const char* default_value, bool in_quotes) {
    if (!name || !value)
        return;

    if (default_value && strcmp(value, default_value) == 0)
        return;

    save_to_ini(name, utf8_to_wstring(value).c_str(), nullptr, in_quotes);
}

std::wstring SpellChecker::load_from_ini(const wchar_t* name,
                                         const wchar_t* default_value, bool in_quotes) {
    assert (name && default_value);

    auto value = read_ini_value(L"Spellcheck", name, default_value, m_ini_file_path.c_str());

    if (in_quotes) {
        // Proof check for quotes
        if (value.front() != '\"' || value.back() != '\"' || value.length() < 2) {
            return default_value;
        }

        return value.substr(1, value.length() - 2);
    }
    return value;
}

void SpellChecker::load_from_ini(int& value, const wchar_t* name,
                                 int default_value) {
    if (!name)
        return;

    auto buf = load_from_ini(name, std::to_wstring(default_value).c_str());
    value = _wtoi(buf.c_str());
}

void SpellChecker::load_from_ini(bool& value, const wchar_t* name,
                                 bool default_value) {
    if (!name)
        return;

    auto buf = load_from_ini(name, std::to_wstring(default_value).c_str());
    value = _wtoi(buf.c_str()) != 0;
}

std::string SpellChecker::load_from_ini_utf8(const wchar_t* name,
                                             const char* default_value, bool in_quotes) {
    if (!name || !default_value)
        return default_value;
    return to_utf8_string(load_from_ini(name, utf8_to_wstring(default_value).c_str(), in_quotes).c_str());
}

// Here parameter is in ANSI (may as well be utf-8 cause only English I guess)
void SpellChecker::update_aspell_language_options() {
    if (m_settings.aspell_language == L"<MULTIPLE>") {
        set_multiple_languages(m_settings.aspell_multi_languages, m_aspell_speller.get());
        m_aspell_speller->set_mode(1);
    }
    else {
        m_aspell_speller->set_language(m_settings.aspell_language.c_str());
        m_current_speller->set_mode(0);
    }
}

void SpellChecker::update_hunspell_language_options() {
    if (m_settings.hunspell_language == L"<MULTIPLE>") {
        set_multiple_languages(m_settings.hunspell_multi_languages.c_str(), m_hunspell_speller.get());
        m_hunspell_speller->set_mode(1);
    }
    else {
        m_hunspell_speller->set_language(m_settings.hunspell_language.c_str());
        m_hunspell_speller->set_mode(0);
    }
}

void SpellChecker::set_multiple_languages(std::wstring_view multi_string,
                                          SpellerInterface* speller) {
    std::vector<std::wstring> multi_lang_list;
    for (auto token : tokenize<wchar_t>(multi_string, LR"(\|)"))
        multi_lang_list.push_back(std::wstring{token});

    speller->set_multiple_languages(multi_lang_list);
}

bool SpellChecker::hunspell_reinit_settings(bool reset_directory) {
    if (reset_directory) {
        m_hunspell_speller->set_directory(m_settings.hunspell_user_path.c_str());
        m_hunspell_speller->set_additional_directory(m_settings.hunspell_system_path.c_str());
    }
    if (m_settings.hunspell_language != L"<MULTIPLE>")
        m_hunspell_speller->set_language(m_settings.hunspell_language.c_str());
    else
        set_multiple_languages(m_settings.hunspell_multi_languages.c_str(), m_hunspell_speller.get());
    return true;
}

bool SpellChecker::aspell_reinit_settings() {
    m_aspell_speller->init(m_settings.aspell_path.c_str());
    m_aspell_speller->set_allow_run_together(m_settings.aspell_allow_run_together_words);

    if (m_settings.aspell_language != L"<MULTIPLE>") {
        m_aspell_speller->set_language(m_settings.aspell_language.c_str());
    }
    else
        set_multiple_languages(m_settings.aspell_multi_languages, m_aspell_speller.get());
    return true;
}

void SpellChecker::update_suggestion_box() {
    hide_suggestion_box();
    SetLayeredWindowAttributes(m_suggestions_instance->getHSelf(), 0,
                               static_cast<BYTE>((255 * m_settings.suggestion_button_opacity) / 100),
                               LWA_ALPHA);
}

void SpellChecker::apply_conversions(
    std::wstring& word) {
    for (auto& c : word) {
        if (m_settings.ignore_yo) {
            if (c == L'¸')
                c = L'å';
            if (c == L'¨')
                c = L'¸';
        }
        if (m_settings.convert_single_quotes) {
            if (c == L'’')
                c = L'\'';
        }
    }
}

void SpellChecker::reset_hot_spot_cache() {
    memset(m_hot_spot_cache, -1, sizeof(m_hot_spot_cache));
}

bool SpellChecker::check_word(std::wstring word, long start, long /*End*/) {
    if (!m_current_speller->is_working() || word.empty())
        return true;
    // Well Numbers have same codes for ANSI and Unicode I guess, so
    // If word contains number then it's probably just a number or some crazy name
    auto style = get_style(start);
    if (m_settings.check_only_comments_and_strings && !SciUtils::is_comment_or_string(m_lexer, style))
        return true;

    if (m_hot_spot_cache[style] == -1) {
        m_hot_spot_cache[style] = send_msg_to_editor(get_current_scintilla(), SCI_STYLEGETHOTSPOT,
                                                     style);
    }

    if (m_hot_spot_cache[style] == 1)
        return true;

    apply_conversions(word);

    auto symbols_num = word.length();
    if (symbols_num == 0) {
        return true;
    }

    if (m_settings.ignore_one_letter && symbols_num == 1) {
        return true;
    }

    if (m_settings.ignore_containing_digit &&
        wcspbrk(word.c_str(), L"0123456789") != nullptr) // Same for UTF-8 and not
    {
        return true;
    }

    if (m_settings.ignore_starting_with_capital || m_settings.ignore_having_a_capital || m_settings.ignore_all_capital
    ) {
        if (m_settings.ignore_starting_with_capital && IsCharUpper(word.front ())) {
            return true;
        }
        if (m_settings.ignore_having_a_capital || m_settings.ignore_all_capital) {
            bool all_upper = IsCharUpper(word.front()), any_upper = false;
            for (auto c : std::wstring_view(word).substr(1)) {
                if (IsCharUpper(c)) {
                    any_upper = true;
                }
                else
                    all_upper = false;
            }

            if (!all_upper && any_upper && m_settings.ignore_having_a_capital)
                return true;

            if (all_upper && m_settings.ignore_all_capital)
                return true;
        }
    }

    if (m_settings.ignore_having_underscore && wcschr(word.c_str(), L'_') != nullptr)
        // I guess the same for UTF-8 and ANSI
    {
        return true;
    }

    auto len = word.length();

    if (m_settings.ignore_starting_or_ending_with_apostrophe) {
        if (word[0] == '\'' || word[len - 1] == '\'') {
            return true;
        }
    }

    bool res = m_current_speller->check_word(word.c_str());
    return res;
}

void SpellChecker::cut_apostrophes(std::wstring_view& word) {
    if (m_settings.remove_boundary_apostrophes) {
        while (!word.empty() && word.front() == L'\'')
            word.remove_prefix(1);

        while (!word.empty() && word.back() == L'\'')
            word.remove_suffix(1);
    }
}

int SpellChecker::check_text_default_answer(CheckTextMode mode) {
    switch (mode) {
    case CheckTextMode::underline_errors:
    case CheckTextMode::find_first:
    case CheckTextMode::find_last:
        return false;
    case CheckTextMode::get_first:
        return -1;
    }
    return -1;
}

std::vector<LanguageInfo> SpellChecker::get_available_languages() const {
    if (!active_speller()->is_working())
        return {};

    auto langs = active_speller()->get_language_list();
    std::sort(langs.begin(), langs.end(), m_settings.use_language_name_aliases ? less_aliases : less_original);
    return langs;
}

int SpellChecker::check_text(const MappedWstring& text_to_check, long offset,
                             CheckTextMode mode, size_t skip_chars) {
    if (text_to_check.str.empty()) {
        return check_text_default_answer(mode);
    }

    HWND scintilla_window = get_current_scintilla();
    send_msg_to_editor(scintilla_window, SCI_GETINDICATORCURRENT);
    bool stop = false;
    long resulting_word_end = -1, resulting_word_start = -1;
    auto text_len = text_to_check.str.length();
    std::vector<long> underline_buffer;
    long word_start = 0;
    long word_end = 0;

    auto sv = std::wstring_view (text_to_check.str);
    sv.remove_prefix(skip_chars);
    auto tokens = tokenize<wchar_t>(sv, m_settings.delimiters);

    for (auto token : tokens) {
        cut_apostrophes(token);
        word_start = static_cast<long> (offset + text_to_check.get_original_index(token.data() - text_to_check.str.data()));
        word_end = static_cast<long> (word_start + token.length());
        if (word_end < word_start)
            continue;

        if (!check_word(std::wstring(token), word_start, word_end)) {
            switch (mode) {
            case CheckTextMode::underline_errors:
                underline_buffer.push_back(word_start);
                underline_buffer.push_back(word_end);
                break;
            case CheckTextMode::find_first:
                if (word_end > m_current_position) {
                    send_msg_to_editor(get_current_scintilla(), SCI_SETSEL, word_start,
                                       word_end);
                    stop = true;
                }
                break;
            case CheckTextMode::find_last:
                {
                    if (word_end >= m_current_position) {
                        stop = true;
                        break;
                    }
                    resulting_word_start = word_start;
                    resulting_word_end = word_end;
                }
                break;
            case CheckTextMode::get_first:
                return word_start;
            }
            if (stop)
                break;
        }
        else {
        }

    }

    if (mode == CheckTextMode::underline_errors) {
        long prev_pos = offset;
        for (long i = 0; i < (long)underline_buffer.size() - 1; i += 2) {
            remove_underline(scintilla_window, prev_pos, underline_buffer[i] - 1);
            create_word_underline(scintilla_window, underline_buffer[i],
                                  underline_buffer[i + 1] - 1);
            prev_pos = underline_buffer[i + 1];
        }
        remove_underline(scintilla_window, prev_pos,
                         offset + static_cast<long>(text_len) - 1);
    }

    // PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT,
    // oldid);

    switch (mode) {
    case CheckTextMode::underline_errors:
        return true;
    case CheckTextMode::find_first:
        return stop;
    case CheckTextMode::get_first:
        return -1;
    case CheckTextMode::find_last:
        if (resulting_word_start == -1)
            return false;
        else {
            send_msg_to_editor(get_current_scintilla(), SCI_SETSEL, resulting_word_start,
                               resulting_word_end);
            return true;
        }
    };
    return false;
}

void SpellChecker::clear_visible_underlines() {
    auto length =
        send_msg_to_editor(get_current_scintilla(), SCI_GETLENGTH);
    if (length > 0) {
        post_msg_to_active_editor(get_current_scintilla(), SCI_SETINDICATORCURRENT,
                                  SCE_ERROR_UNDERLINE);
        post_msg_to_active_editor(get_current_scintilla(), SCI_INDICATORCLEARRANGE, 0,
                                  length);
    }
}

void SpellChecker::check_visible(bool not_intersection_only) {
    check_text(get_visible_text(&m_visible_text_offset, not_intersection_only), m_visible_text_offset,
               CheckTextMode::underline_errors);
}

void SpellChecker::set_encoding_by_id(int enc_id) {
    switch (enc_id) {
    case SC_CP_UTF8:
        m_current_encoding = EncodingType::utf8;
        break;
    default:
        {
            m_current_encoding = EncodingType::ansi;
        }
    }
    m_hunspell_speller->set_encoding(m_current_encoding);
    m_aspell_speller->set_encoding(m_current_encoding);
}

void SpellChecker::recheck_modified() {
    if (!m_current_speller->is_working()) {
        clear_all_underlines();
        return;
    }

    auto first_modified_line = send_msg_to_editor(
        get_current_scintilla(), SCI_LINEFROMPOSITION, m_modified_start);
    auto last_modified_line = send_msg_to_editor(
        get_current_scintilla(), SCI_LINEFROMPOSITION, m_modified_end);
    auto line_count =
        send_msg_to_editor(get_current_scintilla(), SCI_GETLINECOUNT);
    auto first_possibly_modified_pos = send_msg_to_editor(
        get_current_scintilla(), SCI_POSITIONFROMLINE, first_modified_line);

    LRESULT last_possibly_modified_pos;
    if (last_modified_line + 1 < line_count) {
        last_possibly_modified_pos = send_msg_to_editor(
            get_current_scintilla(), SCI_POSITIONFROMLINE, last_modified_line + 1);
    }
    else {
        last_possibly_modified_pos =
            send_msg_to_editor(get_current_scintilla(), SCI_GETLENGTH);
    }

    Sci_TextRange range;
    range.chrg.cpMin = static_cast<long>(first_possibly_modified_pos);
    range.chrg.cpMax = static_cast<long>(last_possibly_modified_pos);
    std::vector<char> buf(range.chrg.cpMax - range.chrg.cpMin + 1 + 1);
    range.lpstrText = buf.data();
    send_msg_to_editor(get_current_scintilla(), SCI_GETTEXTRANGE, 0, (LPARAM)&range);

    check_text(to_mapped_wstring(range.lpstrText), static_cast<long>(first_possibly_modified_pos),
               CheckTextMode::underline_errors);
}

MappedWstring SpellChecker::to_mapped_wstring(std::string_view str) {
    if (m_current_encoding == EncodingType::utf8)
        return utf8_to_mapped_wstring(str);
    else
        return to_mapped_wstring(str);
}

void SpellChecker::recheck_visible(bool not_intersection_only) {
    if (!m_current_speller->is_working()) {
        clear_all_underlines();
        return;
    }

    int codepage_id = (int)send_msg_to_editor(get_current_scintilla(), SCI_GETCODEPAGE,
                                              0, 0);
    set_encoding_by_id(codepage_id); // For now it just changes should we convert it
    // to utf-8 or no
    if (check_text_needed())
        check_visible(not_intersection_only);
    else
        clear_all_underlines();
}

void SpellChecker::error_msg_box(const wchar_t* message) {
    wchar_t buf[DEFAULT_BUF_SIZE];
    swprintf_s(buf, L"DSpellCheck Error: %ws", message);
    MessageBox(m_npp_data_instance->npp_handle, message, L"Error Happened!",
               MB_OK | MB_ICONSTOP);
}

void SpellChecker::copy_misspellings_to_clipboard() {
    auto length_doc =
        (send_msg_to_editor(get_current_scintilla(), SCI_GETLENGTH) + 1);

    std::vector<char> buf(length_doc);
    send_msg_to_editor(get_current_scintilla(), SCI_GETTEXT, length_doc, reinterpret_cast<LPARAM>(buf.data()));
    auto mapped_str = to_mapped_wstring(buf.data ());
    int res = 0;
    std::wstring str;
    do {
        res = check_text(mapped_str, res, CheckTextMode::get_first);
        if (res != -1) {
            str += mapped_str.str.data() + res;
            str += L"\n";
        }
        else
            break;

        while (*(buf.data() + res) != 0)
            res++;

        while (*(buf.data() + res) == 0)
            res++;

        if (res >= length_doc)
            break;
    }
    while (true);
    const size_t len = (str.length() + 1) * 2;
    HGLOBAL h_mem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(h_mem), str.c_str(), len);
    GlobalUnlock(h_mem);
    OpenClipboard(nullptr);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, h_mem);
    CloseClipboard();
}

SuggestionsMenuItem::SuggestionsMenuItem(const wchar_t* text_arg, int id_arg,
                                         bool separator_arg /*= false*/) {
    text = text_arg;
    id = static_cast<BYTE>(id_arg);
    separator = separator_arg;
}
