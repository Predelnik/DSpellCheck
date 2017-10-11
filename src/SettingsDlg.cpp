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

#include "SettingsDlg.h"

#include "aspell.h"
#include "CheckedList/CheckedList.h"
#include "CommonFunctions.h"
#include "LangList.h"
#include "DownloadDicsDlg.h"
#include "Plugin.h"
#include "RemoveDictionariesDialog.h"
#include "SpellChecker.h"

#include "resource.h"
#include "LanguageName.h"
#include "HunspellInterface.h"
#include "utils/winapi.h"
#include "utils/string_utils.h"
#include <uxtheme.h>

SimpleDlg::SimpleDlg() : StaticDialog()
{
    m_h_ux_theme = ::LoadLibrary(TEXT("uxtheme.dll"));
    if (m_h_ux_theme)
        m_open_theme_data = (otd_proc)::GetProcAddress(m_h_ux_theme, "OpenThemeData");
}

void SimpleDlg::init_settings(HINSTANCE h_inst, HWND parent, NppData npp_data)
{
    m_npp_data_instance = npp_data;
    return Window::init(h_inst, parent);
}

void SimpleDlg::disable_language_combo(bool disable)
{
    ComboBox_ResetContent(m_h_combo_language);
    EnableWindow(m_h_combo_language, !disable);
    ListBox_ResetContent(get_remove_dics()->get_list_box());
    EnableWindow(m_h_remove_dics, !disable);
}

// Called from main thread, beware!
bool SimpleDlg::add_available_languages(const std::vector<LanguageName>& langs_available,
                                      const wchar_t* current_language,
                                      std::wstring_view multi_languages,
                                      HunspellInterface* hunspell_speller)
{
    ComboBox_ResetContent(m_h_combo_language);
    ListBox_ResetContent(get_lang_list()->get_list_box());
    ListBox_ResetContent(get_remove_dics()->get_list_box());

    int selected_index = 0;

    {
        unsigned int i = 0;
        for (auto &lang : langs_available)
        {
            if (current_language == lang.orig_name)
                selected_index = i;

            ComboBox_AddString(m_h_combo_language, lang.alias_name.c_str ());
            ListBox_AddString(get_lang_list()->get_list_box(),
                lang.alias_name.c_str ());
            if (hunspell_speller)
            {
                wchar_t buf[DEFAULT_BUF_SIZE];
                wcscpy(buf, lang.alias_name.c_str());
                if (hunspell_speller->get_lang_only_system(lang.orig_name.c_str ()))
                    wcscat(buf, L" [!For All Users]");

                ListBox_AddString(get_remove_dics()->get_list_box(), buf);
            }
            ++i;
        }

        if (wcscmp(current_language, L"<MULTIPLE>") == 0)
            selected_index = i;
    }

    if (!langs_available.empty ())
        ComboBox_AddString(m_h_combo_language, L"Multiple Languages...");

    ComboBox_SetCurSel(m_h_combo_language, selected_index);

    CheckedListBox_EnableCheckAll(get_lang_list()->get_list_box(), BST_UNCHECKED);
    for (auto &token : tokenize<wchar_t> (multi_languages, LR"(\|)"))
    {
        int index = -1;
        int i = 0;
        for (auto &lang : langs_available)
        {
            if (token == lang.orig_name)
            {
                index = i;
                break;
            }
            ++i;
        }
        if (index != -1)
            CheckedListBox_SetCheckState(get_lang_list()->get_list_box(), index,
            BST_CHECKED);
    }
    return true;
}

static HWND create_tool_tip(int tool_id, HWND h_dlg, const wchar_t* psz_text)
{
    if (!tool_id || !h_dlg || !psz_text)
    {
        return nullptr;
    }
    // Get the window of the tool.
    HWND hwnd_tool = GetDlgItem(h_dlg, tool_id);

    // Create the tooltip. g_hInst is the global instance handle.
    HWND hwnd_tip =
        CreateWindowEx(NULL, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP,
                       CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                       h_dlg, nullptr, (HINSTANCE)get_h_module(), nullptr);

    if (!hwnd_tool || !hwnd_tip)
    {
        return (HWND)nullptr;
    }

    // Associate the tooltip with the tool.
    TOOLINFO tool_info;
    memset(&tool_info, 0, sizeof (tool_info));
    tool_info.cbSize = sizeof(tool_info);
    tool_info.hwnd = h_dlg;
    tool_info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    tool_info.uId = (UINT_PTR)hwnd_tool;
    tool_info.lpszText = const_cast<wchar_t *>(psz_text);
    SendMessage(hwnd_tip, TTM_ADDTOOL, 0, (LPARAM)&tool_info);

    return hwnd_tip;
}

SimpleDlg::~SimpleDlg()
{
    if (m_h_ux_theme)
        FreeLibrary(m_h_ux_theme);
}

void SimpleDlg::apply_lib_change(SpellChecker* spell_checker_instance)
{
    spell_checker_instance->SetLibMode(get_selected_lib());
    spell_checker_instance->ReinitLanguageLists(true);
    spell_checker_instance->DoPluginMenuInclusion();
}

// Called from main thread, beware!
void SimpleDlg::apply_settings(SpellChecker* spell_checker_instance)
{
    int lang_count = ComboBox_GetCount(m_h_combo_language);
    int cur_sel = ComboBox_GetCurSel(m_h_combo_language);

    if (IsWindowEnabled(m_h_combo_language))
    {
        if (cur_sel == lang_count - 1)
        {
            if (get_selected_lib())
                spell_checker_instance->SetHunspellLanguage(L"<MULTIPLE>");
            else
                spell_checker_instance->SetAspellLanguage(L"<MULTIPLE>");
        }
        else
        {
            if (get_selected_lib() == 0)
                spell_checker_instance->SetAspellLanguage(
                    spell_checker_instance->GetLangByIndex(cur_sel));
            else
                spell_checker_instance->SetHunspellLanguage(
                    spell_checker_instance->GetLangByIndex(cur_sel));
        }
    }
    spell_checker_instance->RecheckVisible();
    spell_checker_instance->SetSuggestionsNum(_wtoi(get_edit_text (m_h_suggestions_num).c_str ()));
    if (get_selected_lib() == 0)
        spell_checker_instance->SetAspellPath(get_edit_text (m_h_lib_path).c_str ());
    else
    {
        spell_checker_instance->SetHunspellPath(get_edit_text (m_h_lib_path).c_str ());
        spell_checker_instance->SetHunspellAdditionalPath(get_edit_text (m_h_system_path).c_str ());
    }

    spell_checker_instance->SetCheckThose(
        Button_GetCheck(m_h_check_only_those) == BST_CHECKED ? 1 : 0);
    spell_checker_instance->SetFileTypes(get_edit_text (m_h_file_types).c_str ());
    spell_checker_instance->SetSuggType(ComboBox_GetCurSel(m_h_sugg_type));
    spell_checker_instance->SetCheckComments(Button_GetCheck(m_h_check_comments) ==
        BST_CHECKED);
    spell_checker_instance->SetDecodeNames(Button_GetCheck(m_h_decode_names) ==
        BST_CHECKED);
    spell_checker_instance->SetOneUserDic(Button_GetCheck(m_h_one_user_dic) ==
        BST_CHECKED);
    update_langs_menu();
}

void SimpleDlg::set_lib_mode(int lib_mode)
{
    ComboBox_SetCurSel(m_h_library, lib_mode);
}

void SimpleDlg::fill_lib_info(int status, const wchar_t* aspell_path,
                            const wchar_t* hunspell_path,
                            const wchar_t* hunspell_additional_path)
{
    if (get_selected_lib() == 0)
    {
        ShowWindow(m_h_aspell_status, 1);
        ShowWindow(m_h_download_dics, 0);
        if (status == 2)
        {
            m_aspell_status_color = COLOR_OK;
            Static_SetText(m_h_aspell_status, L"Aspell Status: OK");
        }
        else if (status == 1)
        {
            m_aspell_status_color = COLOR_FAIL;
            Static_SetText(m_h_aspell_status, L"Aspell Status: No Dictionaries");
        }
        else
        {
            m_aspell_status_color = COLOR_FAIL;
            Static_SetText(m_h_aspell_status, L"Aspell Status: Fail");
        }
        Edit_SetText(m_h_lib_path, get_actual_aspell_path(aspell_path).c_str ());

        Static_SetText(m_h_lib_group_box, L"Aspell Location");
        ShowWindow(m_h_lib_link, 1);
        ShowWindow(m_h_remove_dics, 0);
        ShowWindow(m_h_decode_names, 0);
        ShowWindow(m_h_one_user_dic, 0);
        ShowWindow(m_h_aspell_reset_path, 1);
        ShowWindow(m_h_hunspell_reset_path, 0);
        ShowWindow(m_h_hunspell_path_group_box, 0);
        ShowWindow(m_h_hunspell_path_type, 0);
        ShowWindow(m_h_lib_path, 1);
        ShowWindow(m_h_system_path, 0);
        // SetWindowText (HLibLink, L"<A HREF=\"http://aspell.net/win32/\">Aspell
        // Library and Dictionaries for Win32</A>");
    }
    else
    {
        ShowWindow(m_h_aspell_status, 0);
        ShowWindow(m_h_download_dics, 1);
        ShowWindow(m_h_lib_link,
                   0); // Link to dictionaries doesn't seem to be working anyway
        ShowWindow(m_h_remove_dics, 1);
        ShowWindow(m_h_decode_names, 1);
        ShowWindow(m_h_one_user_dic, 1);
        ShowWindow(m_h_aspell_reset_path, 0);
        ShowWindow(m_h_hunspell_reset_path, 1);
        ShowWindow(m_h_hunspell_path_group_box, 1);
        ShowWindow(m_h_hunspell_path_type, 1);
        if (ComboBox_GetCurSel(m_h_hunspell_path_type) == 0)
        {
            ShowWindow(m_h_lib_path, 1);
            ShowWindow(m_h_system_path, 0);
        }
        else
        {
            ShowWindow(m_h_lib_path, 0);
            ShowWindow(m_h_system_path, 1);
        }
        // SetWindowText (HLibLink, L"<A
        // HREF=\"http://wiki.openoffice.org/wiki/Dictionaries\">Hunspell
        // Dictionaries</A>");
        Static_SetText(m_h_lib_group_box, L"Hunspell Settings");
        Edit_SetText(m_h_lib_path, hunspell_path);
    }
    Edit_SetText(m_h_system_path, hunspell_additional_path);
}

void SimpleDlg::fill_sugestions_num(int suggestions_num)
{
    wchar_t buf[10];
    _itow_s(suggestions_num, buf, 10);
    Edit_SetText(m_h_suggestions_num, buf);
}

void SimpleDlg::set_file_types(bool check_those, const wchar_t* file_types)
{
    if (!check_those)
    {
        Button_SetCheck(m_h_check_not_those, BST_CHECKED);
        Button_SetCheck(m_h_check_only_those, BST_UNCHECKED);
        Edit_SetText(m_h_file_types, file_types);
    }
    else
    {
        Button_SetCheck(m_h_check_only_those, BST_CHECKED);
        Button_SetCheck(m_h_check_not_those, BST_UNCHECKED);
        Edit_SetText(m_h_file_types, file_types);
    }
}

void SimpleDlg::set_sugg_type(int sugg_type)
{
    ComboBox_SetCurSel(m_h_sugg_type, sugg_type);
}

void SimpleDlg::set_check_comments(bool value)
{
    Button_SetCheck(m_h_check_comments, value ? BST_CHECKED : BST_UNCHECKED);
}

void SimpleDlg::set_decode_names(bool value)
{
    Button_SetCheck(m_h_decode_names, value ? BST_CHECKED : BST_UNCHECKED);
}

void SimpleDlg::set_one_user_dic(bool value)
{
    Button_SetCheck(m_h_one_user_dic, value ? BST_CHECKED : BST_UNCHECKED);
}

int SimpleDlg::get_selected_lib() { return ComboBox_GetCurSel(m_h_library); }

static int CALLBACK browse_callback_proc(HWND hwnd, UINT u_msg, LPARAM /*lParam*/,
                                       LPARAM lp_data)
{
    // If the BFFM_INITIALIZED message is received
    // set the path to the start path.
    switch (u_msg)
    {
    case BFFM_INITIALIZED:
        {
            if (NULL != lp_data)
            {
                SendMessage(hwnd, BFFM_SETSELECTION, true, lp_data);
            }
        }
    default:
        break;
    }

    return 0; // The function should always return 0.
}

INT_PTR SimpleDlg::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param)
{
    wchar_t* end_ptr;

    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Retrieving handles of dialog controls
            m_h_combo_language = ::GetDlgItem(_hSelf, IDC_COMBO_LANGUAGE);
            m_h_suggestions_num = ::GetDlgItem(_hSelf, IDC_SUGGESTIONS_NUM);
            m_h_aspell_status = ::GetDlgItem(_hSelf, IDC_ASPELL_STATUS);
            m_h_lib_path = ::GetDlgItem(_hSelf, IDC_ASPELLPATH);
            m_h_check_not_those = ::GetDlgItem(_hSelf, IDC_FILETYPES_CHECKNOTTHOSE);
            m_h_check_only_those = ::GetDlgItem(_hSelf, IDC_FILETYPES_CHECKTHOSE);
            m_h_file_types = ::GetDlgItem(_hSelf, IDC_FILETYPES);
            m_h_check_comments = ::GetDlgItem(_hSelf, IDC_CHECKCOMMENTS);
            m_h_lib_link = ::GetDlgItem(_hSelf, IDC_LIB_LINK);
            m_h_sugg_type = ::GetDlgItem(_hSelf, IDC_SUGG_TYPE);
            m_h_library = ::GetDlgItem(_hSelf, IDC_LIBRARY);
            m_h_lib_group_box = ::GetDlgItem(_hSelf, IDC_LIB_GROUPBOX);
            m_h_download_dics = ::GetDlgItem(_hSelf, IDC_DOWNLOADDICS);
            m_h_remove_dics = ::GetDlgItem(_hSelf, IDC_REMOVE_DICS);
            m_h_decode_names = ::GetDlgItem(_hSelf, IDC_DECODE_NAMES);
            m_h_one_user_dic = ::GetDlgItem(_hSelf, IDC_ONE_USER_DIC);
            m_h_hunspell_path_group_box = ::GetDlgItem(_hSelf, IDC_HUNSPELL_PATH_GROUPBOX);
            m_h_hunspell_path_type = ::GetDlgItem(_hSelf, IDC_HUNSPELL_PATH_TYPE);
            m_h_aspell_reset_path = ::GetDlgItem(_hSelf, IDC_RESETASPELLPATH);
            m_h_hunspell_reset_path = ::GetDlgItem(_hSelf, IDC_RESETHUNSPELLPATH);
            m_h_system_path = ::GetDlgItem(_hSelf, IDC_SYSTEMPATH);
            ComboBox_AddString(m_h_library, L"Aspell");
            ComboBox_AddString(m_h_library, L"Hunspell");
            ComboBox_AddString(m_h_hunspell_path_type, L"For Current User");
            ComboBox_AddString(m_h_hunspell_path_type, L"For All Users");
            ComboBox_SetCurSel(m_h_hunspell_path_type, 0);
            ShowWindow(m_h_lib_path, 1);
            ShowWindow(m_h_system_path, 0);

            ComboBox_AddString(m_h_sugg_type, L"Special Suggestion Button");
            ComboBox_AddString(m_h_sugg_type, L"Use N++ Context Menu");
            m_default_brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

            m_aspell_status_color = RGB(0, 0, 0);
            return true;
        }
    case WM_CLOSE:
        {
            EndDialog(_hSelf, 0);
            DeleteObject(m_default_brush);
            return false;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(w_param))
            {
            case IDC_COMBO_LANGUAGE:
                if (HIWORD(w_param) == CBN_SELCHANGE)
                {
                    if (ComboBox_GetCurSel(m_h_combo_language) ==
                        ComboBox_GetCount(m_h_combo_language) - 1)
                    {
                        get_lang_list()->do_dialog();
                    }
                }
                break;
            case IDC_LIBRARY:
                if (HIWORD(w_param) == CBN_SELCHANGE)
                {
                    get_spell_checker()->libChange();
                }
                break;
            case IDC_HUNSPELL_PATH_TYPE:
                if (HIWORD(w_param) == CBN_SELCHANGE)
                {
                    if (ComboBox_GetCurSel(m_h_hunspell_path_type) == 0 ||
                        get_selected_lib() == 0)
                    {
                        ShowWindow(m_h_lib_path, 1);
                        ShowWindow(m_h_system_path, 0);
                    }
                    else
                    {
                        ShowWindow(m_h_lib_path, 0);
                        ShowWindow(m_h_system_path, 1);
                    }
                }
                break;
            case IDC_SUGGESTIONS_NUM:
                {
                    if (HIWORD(w_param) == EN_CHANGE)
                    {
                        wchar_t buf[DEFAULT_BUF_SIZE];
                        Edit_GetText(m_h_suggestions_num, buf, DEFAULT_BUF_SIZE);
                        if (!*buf)
                            return true;

                        int x = wcstol(buf, &end_ptr, 10);
                        if (*end_ptr)
                            Edit_SetText(m_h_suggestions_num, L"5");
                        else if (x > 20)
                            Edit_SetText(m_h_suggestions_num, L"20");
                        else if (x < 1)
                            Edit_SetText(m_h_suggestions_num, L"1");

                        return true;
                    }
                }
                break;
            case IDC_REMOVE_DICS:
                if (HIWORD(w_param) == BN_CLICKED)
                {
                    get_remove_dics()->do_dialog();
                }
                break;
            case IDC_RESETASPELLPATH:
            case IDC_RESETHUNSPELLPATH:
                {
                    if (HIWORD(w_param) == BN_CLICKED)
                    {
                        std::wstring path;
                        if (get_selected_lib() == 0)
                            path = get_default_aspell_path();
                        else
                            path = get_default_hunspell_path();

                        if (get_selected_lib() == 0 || ComboBox_GetCurSel(m_h_hunspell_path_type) == 0)
                            Edit_SetText(m_h_lib_path, path.c_str ());
                        else
                            Edit_SetText(m_h_system_path, L".\\plugins\\config\\Hunspell");
                        return true;
                    }
                }
                break;
            case IDC_DOWNLOADDICS:
                {
                    if (HIWORD(w_param) == BN_CLICKED)
                    {
                        get_download_dics()->do_dialog();
                    }
                }
                break;
            case IDC_BROWSEASPELLPATH:
                if (get_selected_lib() == 0)
                {
                    OPENFILENAME ofn;
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = _hSelf;
                    std::vector<wchar_t> filename(MAX_PATH);
                    auto lib_path = get_edit_text(m_h_lib_path);
                    std::copy (lib_path.begin (), lib_path.end (), filename.begin ());
                    // TODO: add possibility to use modern browse dialog
                    ofn.lpstrFile = filename.data();
                    ofn.nMaxFile = DEFAULT_BUF_SIZE;
                    ofn.lpstrFilter = L"Aspell Library (*.dll)\0*.dll\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFileTitle = nullptr;
                    ofn.nMaxFileTitle = 0;
                    ofn.lpstrInitialDir = nullptr;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileName(&ofn))
                        Edit_SetText(m_h_lib_path, filename.data ());
                }
                else
                {
                    // Thanks to http://vcfaq.mvps.org/sdk/20.htm
                    BROWSEINFO bi;
                    memset(&bi, 0, sizeof(bi));
                    std::vector<wchar_t> path;

                    LPITEMIDLIST pidl_root = nullptr;
                    SHGetFolderLocation(_hSelf, 0, nullptr, NULL, &pidl_root);

                    bi.pidlRoot = pidl_root;
                    bi.lpszTitle = L"Pick a Directory";
                    bi.pszDisplayName = path.data();
                    bi.ulFlags = BIF_RETURNONLYFSDIRS;
                    bi.lpfn = browse_callback_proc;
                    auto lib_path = get_edit_text(m_h_lib_path);
                    std::vector<wchar_t> npp_path(MAX_PATH);
                    std::vector<wchar_t> final_path (MAX_PATH);
                    send_msg_to_npp(&m_npp_data_instance, NPPM_GETNPPDIRECTORY, MAX_PATH,
                                 reinterpret_cast<LPARAM>(npp_path.data()));
                    PathCombine(final_path.data (), npp_path.data (), lib_path.c_str());
                    bi.lParam = reinterpret_cast<LPARAM>(final_path.data ());
                    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
                    if (pidl != nullptr)
                    {
                        // get the name of the folder
                        std::vector<wchar_t> sz_path (MAX_PATH);
                        SHGetPathFromIDList(pidl, sz_path.data ());
                        Edit_SetText(m_h_lib_path, sz_path.data ());
                        CoTaskMemFree(pidl);
                        // free memory used

                        CoUninitialize();
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:

        switch (reinterpret_cast<LPNMHDR>(l_param)->code)
        {
        case NM_CLICK: // Fall through to the next case.
        case NM_RETURN:
            {
                PNMLINK p_nm_link = reinterpret_cast<PNMLINK>(l_param);
                LITEM item = p_nm_link->item;

                if ((reinterpret_cast<LPNMHDR>(l_param)->hwndFrom == m_h_lib_link) && (item.iLink == 0))
                {
                    ShellExecute(nullptr, L"open", item.szUrl, nullptr, nullptr, SW_SHOW);
                }

                break;
            }
        default:
            break;
        }
        break;
    case WM_CTLCOLORSTATIC:
        if (GetDlgItem(_hSelf, IDC_ASPELL_STATUS) == reinterpret_cast<HWND>(l_param))
        {
            HDC h_dc = (HDC)w_param;
            HTHEME h_theme;

            if (m_open_theme_data)
                h_theme = m_open_theme_data(_hSelf, L"TAB");
            else
                h_theme = nullptr;

            SetBkColor(h_dc, GetSysColor(COLOR_BTNFACE));
            SetBkMode(h_dc, TRANSPARENT);
            SetTextColor(h_dc, m_aspell_status_color);
            if (h_theme)
                return 0;
            else
                return reinterpret_cast<INT_PTR>(m_default_brush);
        }
        break;
    default:
        break;
    }
    return false;
}

void AdvancedDlg::set_underline_settings(int color, int style)
{
    m_underline_color_btn = color;
    ComboBox_SetCurSel(m_h_underline_style, style);
}

void AdvancedDlg::fill_delimiters(const char* delimiters)
{
    Edit_SetText(m_h_edit_delimiters, utf8_to_wstring (delimiters).c_str ());
}

void AdvancedDlg::set_conversion_opts(bool convert_yo, bool convert_single_quotes_arg,
                                    bool remove_boundary_apostrophes)
{
    Button_SetCheck(m_h_ignore_yo, convert_yo ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_convert_single_quotes,
        convert_single_quotes_arg ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_remove_boundary_apostrophes,
        remove_boundary_apostrophes ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedDlg::set_ignore(bool ignore_numbers_arg, bool ignore_c_start_arg,
                            bool ignore_c_have_arg, bool ignore_c_all_arg,
                            bool ignore_arg, bool ignore_sa_apostrophe_arg,
                            bool ignore_one_letter)
{
    Button_SetCheck(m_h_ignore_numbers,
        ignore_numbers_arg ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_ignore_c_start, ignore_c_start_arg ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_ignore_c_have, ignore_c_have_arg ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_ignore_c_all, ignore_c_all_arg ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_ignore_one_letter,
        ignore_one_letter ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_ignore_, ignore_arg ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_ignore_se_apostrophe,
        ignore_sa_apostrophe_arg ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedDlg::set_sugg_box_settings(LRESULT size, LRESULT trans)
{
    SendMessage(m_h_slider_size, TBM_SETPOS, true, size);
    SendMessage(m_h_slider_transparency, TBM_SETPOS, true, trans);
}

void AdvancedDlg::set_buffer_size(int size)
{
    wchar_t buf[DEFAULT_BUF_SIZE];
    _itow_s(size, buf, 10);
    Edit_SetText(m_h_buffer_size, buf);
}

const wchar_t*const indic_names[] = {
    L"Plain", L"Squiggle", L"TT", L"Diagonal",
    L"Strike", L"Hidden", L"Box", L"Round Box",
    L"Straight Box", L"Dash", L"Dots", L"Squiggle Low"
};

INT_PTR AdvancedDlg::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param)
{
    wchar_t* end_ptr = nullptr;
    wchar_t buf[DEFAULT_BUF_SIZE];
    int x;
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Retrieving handles of dialog controls
            m_h_edit_delimiters = ::GetDlgItem(_hSelf, IDC_DELIMETERS);
            m_h_default_delimiters = ::GetDlgItem(_hSelf, IDC_DEFAULT_DELIMITERS);
            m_h_ignore_yo = ::GetDlgItem(_hSelf, IDC_COUNT_YO_AS_YE);
            m_h_convert_single_quotes =
                ::GetDlgItem(_hSelf, IDC_COUNT_SINGLE_QUOTES_AS_APOSTROPHE);
            m_h_remove_boundary_apostrophes =
                ::GetDlgItem(_hSelf, IDC_REMOVE_ENDING_APOSTROPHE);
            m_h_recheck_delay = ::GetDlgItem(_hSelf, IDC_RECHECK_DELAY);
            m_h_underline_color = ::GetDlgItem(_hSelf, IDC_UNDERLINE_COLOR);
            m_h_underline_style = ::GetDlgItem(_hSelf, IDC_UNDERLINE_STYLE);
            m_h_ignore_numbers = ::GetDlgItem(_hSelf, IDC_IGNORE_NUMBERS);
            m_h_ignore_c_start = ::GetDlgItem(_hSelf, IDC_IGNORE_CSTART);
            m_h_ignore_c_have = ::GetDlgItem(_hSelf, IDC_IGNORE_CHAVE);
            m_h_ignore_c_all = ::GetDlgItem(_hSelf, IDC_IGNORE_CALL);
            m_h_ignore_one_letter = ::GetDlgItem(_hSelf, IDC_IGNORE_ONE_LETTER);
            m_h_ignore_ = ::GetDlgItem(_hSelf, IDC_IGNORE_);
            m_h_ignore_se_apostrophe = ::GetDlgItem(_hSelf, IDC_IGNORE_SE_APOSTROPHE);
            m_h_slider_size = ::GetDlgItem(_hSelf, IDC_SLIDER_SIZE);
            m_h_slider_transparency = ::GetDlgItem(_hSelf, IDC_SLIDER_TRANSPARENCY);
            m_h_buffer_size = ::GetDlgItem(_hSelf, IDC_BUFFER_SIZE);
            SendMessage(m_h_slider_size, TBM_SETRANGE, true, MAKELPARAM(5, 22));
            SendMessage(m_h_slider_transparency, TBM_SETRANGE, true, MAKELPARAM(5, 100));

            m_brush = nullptr;

            SetWindowText(m_h_ignore_yo, L"Cyrillic: Count io (\u0451) as e");
            create_tool_tip(IDC_DELIMETERS, _hSelf,
                          L"Standard white-space symbols such as New Line ('\\n'), "
                          L"Carriage Return ('\\r'), Tab ('\\t'), Space (' ') are "
                          L"always counted as delimiters");
            create_tool_tip(IDC_RECHECK_DELAY, _hSelf, L"Delay between the end of typing "
                          L"and rechecking the the text "
                          L"after it");
            create_tool_tip(IDC_IGNORE_CSTART, _hSelf, L"Remember that words at the "
                          L"beginning of sentences would "
                          L"also be ignored that way.");
            create_tool_tip(
                IDC_IGNORE_SE_APOSTROPHE, _hSelf,
                L"Words like this sadly cannot be added to Aspell user dictionary");
            create_tool_tip(IDC_REMOVE_ENDING_APOSTROPHE, _hSelf,
                          L"Words like this are mostly mean plural possessive form in "
                          L"English, if you want to add such forms of words to "
                          L"dictionary manually, please uncheck");

            ComboBox_ResetContent(m_h_underline_style);
            for (auto name : indic_names)
                ComboBox_AddString(m_h_underline_style, name);
            return true;
        }
    case WM_CLOSE:
        {
            if (m_brush)
                DeleteObject(m_brush);
            EndDialog(_hSelf, 0);
            return false;
        }
    case WM_DRAWITEM:
        switch (w_param)
        {
        case IDC_UNDERLINE_COLOR:
            PAINTSTRUCT ps;
            DRAWITEMSTRUCT* ds = (LPDRAWITEMSTRUCT)l_param;
            HDC dc = ds->hDC;
            /*
            Pen = CreatePen (PS_SOLID, 1, RGB (128, 128, 128));
            SelectPen (Dc, Pen);
            */
            if (ds->itemState & ODS_SELECTED)
            {
                DrawEdge(dc, &ds->rcItem, BDR_SUNKENINNER | BDR_SUNKENOUTER, BF_RECT);
            }
            else
            {
                DrawEdge(dc, &ds->rcItem, BDR_RAISEDINNER | BDR_RAISEDOUTER, BF_RECT);
            }
            // DeleteObject (Pen);
            EndPaint(m_h_underline_color, &ps);
            return true;
        }
        break;
    case WM_CTLCOLORBTN:
        if (GetDlgItem(_hSelf, IDC_UNDERLINE_COLOR) == (HWND)l_param)
        {
            HDC h_dc = (HDC)w_param;
            if (m_brush)
                DeleteObject(m_brush);

            SetBkColor(h_dc, m_underline_color_btn);
            SetBkMode(h_dc, TRANSPARENT);
            m_brush = CreateSolidBrush(m_underline_color_btn);
            return (INT_PTR)m_brush;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(w_param))
        {
        case IDC_DEFAULT_DELIMITERS:
            if (HIWORD(w_param) == BN_CLICKED)
                get_spell_checker()->SetDefaultDelimiters();
            return true;
        case IDC_RECHECK_DELAY:
            if (HIWORD(w_param) == EN_CHANGE)
            {
                Edit_GetText(m_h_recheck_delay, buf, DEFAULT_BUF_SIZE);
                if (!*buf)
                    return true;

                x = wcstol(buf, &end_ptr, 10);
                if (*end_ptr)
                    Edit_SetText(m_h_recheck_delay, L"0");
                else if (x > 30000)
                    Edit_SetText(m_h_recheck_delay, L"30000");
                else if (x < 0)
                    Edit_SetText(m_h_recheck_delay, L"0");

                return true;
            }
            break;
        case IDC_BUFFER_SIZE:
            if (HIWORD(w_param) == EN_CHANGE)
            {
                Edit_GetText(m_h_buffer_size, buf, DEFAULT_BUF_SIZE);
                if (!*buf)
                    return true;

                x = wcstol(buf, &end_ptr, 10);
                if (*end_ptr)
                    Edit_SetText(m_h_buffer_size, L"1024");
                else if (x > 10 * 1024)
                    Edit_SetText(m_h_buffer_size, L"10240");
                else if (x < 1)
                    Edit_SetText(m_h_buffer_size, L"1");

                return true;
            }
            break;
        case IDC_UNDERLINE_COLOR:
            if (HIWORD(w_param) == BN_CLICKED)
            {
                CHOOSECOLOR cc;
                static COLORREF acr_cust_clr[16];
                memset(&cc, 0, sizeof(cc));
                cc.hwndOwner = _hSelf;
                cc.lStructSize = sizeof(CHOOSECOLOR);
                cc.rgbResult = m_underline_color_btn;
                cc.lpCustColors = acr_cust_clr;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                if (ChooseColor(&cc))
                {
                    m_underline_color_btn = cc.rgbResult;
                    RedrawWindow(m_h_underline_color, nullptr, nullptr,
                                 RDW_UPDATENOW | RDW_INVALIDATE | RDW_ERASE);
                }
            }
            break;
        }
    default:
        break;
    }
    return false;
}

void AdvancedDlg::set_delimeters_edit(const wchar_t* delimiters)
{
    Edit_SetText(m_h_edit_delimiters, delimiters);
}

void AdvancedDlg::set_recheck_delay(int delay)
{
    wchar_t buf[DEFAULT_BUF_SIZE];
    _itow(delay, buf, 10);
    Edit_SetText(m_h_recheck_delay, buf);
}

int AdvancedDlg::get_recheck_delay()
{
    wchar_t buf[DEFAULT_BUF_SIZE];
    Edit_GetText(m_h_recheck_delay, buf, DEFAULT_BUF_SIZE);
    wchar_t* end_ptr;
    int x = wcstol(buf, &end_ptr, 10);
    return x;
}

// Called from main thread, beware!
void AdvancedDlg::apply_settings(SpellChecker* spell_checker_instance)
{
    spell_checker_instance->SetDelimiters(to_utf8_string(get_edit_text (m_h_edit_delimiters).c_str ()).c_str ());
    spell_checker_instance->SetConversionOptions(
        Button_GetCheck(m_h_ignore_yo) == BST_CHECKED ? true : false,
        Button_GetCheck(m_h_convert_single_quotes) == BST_CHECKED ? true : false,
        Button_GetCheck(m_h_remove_boundary_apostrophes) == BST_CHECKED
            ? true
            : false);
    spell_checker_instance->SetUnderlineColor(m_underline_color_btn);
    spell_checker_instance->SetUnderlineStyle(ComboBox_GetCurSel(m_h_underline_style));
    spell_checker_instance->setRecheckDelay(get_recheck_delay());
    spell_checker_instance->SetIgnore(
        Button_GetCheck(m_h_ignore_numbers) == BST_CHECKED,
        Button_GetCheck(m_h_ignore_c_start) == BST_CHECKED,
        Button_GetCheck(m_h_ignore_c_have) == BST_CHECKED,
        Button_GetCheck(m_h_ignore_c_all) == BST_CHECKED,
        Button_GetCheck(m_h_ignore_) == BST_CHECKED,
        Button_GetCheck(m_h_ignore_se_apostrophe) == BST_CHECKED,
        Button_GetCheck(m_h_ignore_one_letter) == BST_CHECKED);
    spell_checker_instance->SetSuggBoxSettings(
        static_cast<int>(SendMessage(m_h_slider_size, TBM_GETPOS, 0, 0)),
        static_cast<int>(SendMessage(m_h_slider_transparency, TBM_GETPOS, 0, 0)));
    wchar_t* end_ptr;
    auto text = get_edit_text (m_h_buffer_size);
    int x = wcstol(text.c_str (), &end_ptr, 10);
    spell_checker_instance->SetBufferSize(x);
    get_download_dics()->update_list_box();
}

SimpleDlg* SettingsDlg::get_simple_dlg() { return &m_simple_dlg_instance; }

AdvancedDlg* SettingsDlg::get_advanced_dlg() { return &m_advanced_dlg_instance; }

void SettingsDlg::init_settings(HINSTANCE h_inst, HWND parent, NppData npp_data)
{
    m_npp_data_instance = npp_data;
    return Window::init(h_inst, parent);
}

void SettingsDlg::destroy()
{
    m_simple_dlg_instance.destroy();
    m_advanced_dlg_instance.destroy();
};

// Send appropriate event and set some npp thread properties
void SettingsDlg::apply_settings()
{
    get_spell_checker()->applySettings();
}

INT_PTR SettingsDlg::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            m_controls_tab_instance.initTabBar(_hInst, _hSelf, false, true, false);
            m_controls_tab_instance.setFont(L"Tahoma", 13);

            m_simple_dlg_instance.init_settings(_hInst, _hSelf, m_npp_data_instance);
            m_simple_dlg_instance.create(IDD_SIMPLE, false, false);
            m_simple_dlg_instance.display();
            m_advanced_dlg_instance.init(_hInst, _hSelf);
            m_advanced_dlg_instance.create(IDD_ADVANCED, false, false);

            m_window_vector_instance.push_back(
                DlgInfo(&m_simple_dlg_instance, TEXT("Simple"), TEXT("Simple Options")));
            m_window_vector_instance.push_back(DlgInfo(
                &m_advanced_dlg_instance, TEXT("Advanced"), TEXT("Advanced Options")));
            m_controls_tab_instance.createTabs(m_window_vector_instance);
            m_controls_tab_instance.display();
            RECT rc;
            getClientRect(rc);
            m_controls_tab_instance.reSizeTo(rc);
            rc.bottom -= 30;

            m_simple_dlg_instance.reSizeTo(rc);
            m_advanced_dlg_instance.reSizeTo(rc);

            // This stuff is copied from npp source to make tabbed window looked totally
            // nice and white
            ETDTProc enable_dlg_theme =
                (ETDTProc)::SendMessage(_hParent, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
            if (enable_dlg_theme)
                enable_dlg_theme(_hSelf, ETDT_ENABLETAB);

            get_spell_checker()->FillDialogs();

            return true;
        }
    case WM_CONTEXTMENU:
        {
            HMENU menu = CreatePopupMenu();
            AppendMenu(menu, MF_STRING, 0,
                       L"Copy All Misspelled Words in Current Document to Clipboard");
            TrackPopupMenuEx(menu, 0, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param),
                             _hSelf, nullptr);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR* nmhdr = (NMHDR *)l_param;
            if (nmhdr->code == TCN_SELCHANGE)
            {
                if (nmhdr->hwndFrom == m_controls_tab_instance.getHSelf())
                {
                    m_controls_tab_instance.clickedUpdate();
                    return false;
                }
            }
            break;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(w_param))
            {
            case 0: // Menu
                {
                    get_spell_checker()->copyMisspellingsToClipboard();
                }
                break;
            case IDAPPLY:
                if (HIWORD(w_param) == BN_CLICKED)
                {
                    apply_settings();

                    return false;
                }
                break;
            case IDOK:
                if (HIWORD(w_param) == BN_CLICKED)
                {
                    display(false);
                    apply_settings();
                    return false;
                }
                break;
            case IDCANCEL:
                if (HIWORD(w_param) == BN_CLICKED)
                {
                    display(false);
                    return false;
                }
                break;
            }
        }
    }
    return false;
}

UINT SettingsDlg::do_dialog()
{
    if (!isCreated())
    {
        create(IDD_SETTINGS);
        goToCenter();
    }
    else
    {
        goToCenter();
        get_spell_checker()->FillDialogs();
    }

    return true;
    // return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SETTINGS),
    // _hParent, (DLGPROC)dlgProc, (LPARAM)this);
}
