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

#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "LangList.h"
#include "Plugin.h"
#include "RemoveDictionariesDialog.h"
#include "SpellChecker.h"
#include "aspell.h"

#include "LanguageInfo.h"
#include "Settings.h"
#include "npp/NppInterface.h"
#include "resource.h"
#include "utils/winapi.h"
#include <uxtheme.h>

SimpleDlg::SimpleDlg(SettingsDlg &parent, const Settings &settings,
                     NppInterface &npp)
    : m_npp(npp), m_settings(settings), m_parent(parent) {
  m_h_ux_theme = ::LoadLibrary(TEXT("uxtheme.dll"));
  if (m_h_ux_theme != nullptr)
    m_open_theme_data = reinterpret_cast<OtdProc>(
        ::GetProcAddress(m_h_ux_theme, "OpenThemeData"));
}

void SimpleDlg::disable_language_combo(bool disable) {
  auto enable = disable ? FALSE : TRUE;
  ComboBox_ResetContent(m_h_combo_language);
  EnableWindow(m_h_combo_language, enable);
  ListBox_ResetContent(get_remove_dics()->get_list_box());
  EnableWindow(m_h_remove_dics, enable);
}

void SimpleDlg::update_language_controls(const Settings &settings) {
  if (m_h_combo_language == nullptr)
    return;

  ComboBox_ResetContent(m_h_combo_language);
  auto langs = get_spell_checker()->get_available_languages();

  int selected_index = 0;

  auto langs_available = get_spell_checker()->get_available_languages();

  int i = 0;
  for (auto &lang : langs_available) {
    if (settings.get_current_language() == lang.orig_name)
      selected_index = i;

    ComboBox_AddString(
        m_h_combo_language,
        lang.get_aliased_name(settings.use_language_name_aliases));
    ++i;
  }
  if (settings.get_current_language() == L"<MULTIPLE>")
    selected_index = i;

  if (!langs_available.empty())
    ComboBox_AddString(m_h_combo_language, L"Multiple Languages...");

  ComboBox_SetCurSel(m_h_combo_language, selected_index);
  EnableWindow(m_h_combo_language, langs.empty() ? FALSE : TRUE);
}

static HWND create_tool_tip(int tool_id, HWND h_dlg, const wchar_t *psz_text) {
  if (tool_id == 0 || h_dlg == nullptr || psz_text == nullptr) {
    return nullptr;
  }
  // Get the window of the tool.
  HWND hwnd_tool = GetDlgItem(h_dlg, tool_id);

  // Create the tooltip. g_hInst is the global instance handle.
  HWND hwnd_tip = CreateWindowEx(
      NULL, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP, CW_USEDEFAULT,
      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, h_dlg, nullptr,
      static_cast<HINSTANCE>(get_h_module()), nullptr);

  if (hwnd_tool == nullptr || hwnd_tip == nullptr) {
    return nullptr;
  }

  // Associate the tooltip with the tool.
  TOOLINFO tool_info;
  memset(&tool_info, 0, sizeof(tool_info));
  tool_info.cbSize = sizeof(tool_info);
  tool_info.hwnd = h_dlg;
  tool_info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  tool_info.uId = reinterpret_cast<UINT_PTR>(hwnd_tool);
  tool_info.lpszText = const_cast<wchar_t *>(psz_text);
  SendMessage(hwnd_tip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&tool_info));

  return hwnd_tip;
}

SimpleDlg::~SimpleDlg() {
  if (m_h_ux_theme != nullptr)
    FreeLibrary(m_h_ux_theme);
}

void SimpleDlg::apply_settings(Settings &settings) {
  int lang_count = ComboBox_GetCount(m_h_combo_language);
  int cur_sel = ComboBox_GetCurSel(m_h_combo_language);

  if (IsWindowEnabled(m_h_combo_language) != FALSE) {
    *[&]() -> std::wstring * {
      switch (settings.active_speller_lib_id) {
      case SpellerId::aspell:
        return &settings.aspell_language;
      case SpellerId::hunspell:
        return &settings.hunspell_language;
      case SpellerId::native:
        return &settings.native_speller_language;
      case SpellerId::COUNT:
        break;
      }
      return nullptr;
    }() = cur_sel == lang_count - 1 ? L"<MULTIPLE>"
                                    : get_spell_checker()
                                          ->get_available_languages()[cur_sel]
                                          .orig_name.c_str();
  }
  settings.suggestion_count =
      (_wtoi(get_edit_text(m_h_suggestions_num).c_str()));
  switch (settings.active_speller_lib_id) {
  case SpellerId::aspell:
    settings.aspell_path = get_edit_text(m_h_lib_path);
    break;
  case SpellerId::hunspell:
    settings.hunspell_user_path = get_edit_text(m_h_lib_path);
    settings.hunspell_system_path = get_edit_text(m_h_system_path);
    break;
  case SpellerId::native:
    break;
  case SpellerId::COUNT:
    break;
  default:;
  }

  settings.check_those = Button_GetCheck(m_h_check_only_those) == BST_CHECKED;
  settings.file_types = get_edit_text(m_h_file_types);
  settings.suggestions_mode = m_suggestion_mode_cmb.current_data();
  settings.check_comments = Button_GetCheck(m_h_check_comments) == BST_CHECKED;
  settings.check_strings = Button_GetCheck(m_h_check_strings) == BST_CHECKED;
  settings.check_variable_functions =
      Button_GetCheck(m_h_check_varfunc) == BST_CHECKED;
  settings.use_language_name_aliases =
      Button_GetCheck(m_h_decode_names) == BST_CHECKED;
  settings.use_unified_dictionary =
      Button_GetCheck(m_h_one_user_dic) == BST_CHECKED;
  settings.aspell_allow_run_together_words =
      Button_GetCheck(m_h_aspell_run_together_cb) == BST_CHECKED;
}

void SimpleDlg::fill_lib_info(int status, const Settings &settings) {
  ShowWindow(m_h_aspell_run_together_cb,
             static_cast<int>(settings.active_speller_lib_id == SpellerId::aspell));

  auto is_aspell = settings.active_speller_lib_id == SpellerId::aspell ? TRUE : FALSE;
  ShowWindow(m_h_lib_link, is_aspell);
  ShowWindow(m_h_aspell_status, is_aspell);
  auto is_hunspell = settings.active_speller_lib_id == SpellerId::hunspell ? TRUE : FALSE;
  ShowWindow(m_h_download_dics, is_hunspell);
  ShowWindow(m_h_remove_dics, is_hunspell);
  ShowWindow(m_h_decode_names, is_hunspell);
  ShowWindow(m_h_one_user_dic, is_hunspell);
  ShowWindow(m_h_hunspell_path_group_box, is_hunspell);
  ShowWindow(m_h_hunspell_path_type, is_hunspell);
  ShowWindow(m_h_system_path, 0);
  auto not_native = settings.active_speller_lib_id == SpellerId::native ? FALSE : TRUE;
  ShowWindow(m_h_lib_group_box, not_native);
  ShowWindow(m_h_lib_path, not_native);
  ShowWindow(m_h_reset_speller_path, not_native);
  ShowWindow(m_browse_btn, not_native);

  switch (settings.active_speller_lib_id) {
  case SpellerId::aspell:

    if (status == 2) {
      m_aspell_status_color = COLOR_OK;
      Static_SetText(m_h_aspell_status, L"Aspell Status: OK");
    } else if (status == 1) {
      m_aspell_status_color = COLOR_FAIL;
      Static_SetText(m_h_aspell_status, L"Aspell Status: No Dictionaries");
    } else {
      m_aspell_status_color = COLOR_FAIL;
      Static_SetText(m_h_aspell_status, L"Aspell Status: Fail");
    }
    Edit_SetText(m_h_lib_path,
                 get_actual_aspell_path(settings.aspell_path).c_str());

    Static_SetText(m_h_lib_group_box, L"Aspell Location");
    break;
  case SpellerId::hunspell: {
    auto is_lib_path = ComboBox_GetCurSel(m_h_hunspell_path_type) == 0;
    ShowWindow(m_h_lib_path, static_cast<int>(is_lib_path));
    ShowWindow(m_h_system_path, static_cast<int>(!is_lib_path));
    Static_SetText(m_h_lib_group_box, L"Hunspell Settings");
    Edit_SetText(m_h_lib_path, settings.hunspell_user_path.c_str());
  } break;
  case SpellerId::native:
    break;
  case SpellerId::COUNT:
    break;
  default:;
  }
  Edit_SetText(m_h_system_path, settings.hunspell_system_path.c_str());
}

void SimpleDlg::fill_sugestions_num(int suggestions_num) {
  wchar_t buf[10];
  _itow_s(suggestions_num, buf, 10);
  Edit_SetText(m_h_suggestions_num, buf);
}

void SimpleDlg::set_file_types(bool check_those, const wchar_t *file_types) {
  if (!check_those) {
    Button_SetCheck(m_h_check_not_those, BST_CHECKED);
    Button_SetCheck(m_h_check_only_those, BST_UNCHECKED);
    Edit_SetText(m_h_file_types, file_types);
  } else {
    Button_SetCheck(m_h_check_only_those, BST_CHECKED);
    Button_SetCheck(m_h_check_not_those, BST_UNCHECKED);
    Edit_SetText(m_h_file_types, file_types);
  }
}

void SimpleDlg::set_sugg_type(SuggestionMode mode) {
  m_suggestion_mode_cmb.set_index(mode);
}

void SimpleDlg::set_decode_names(bool value) {
  Button_SetCheck(m_h_decode_names, value ? BST_CHECKED : BST_UNCHECKED);
}

void SimpleDlg::set_one_user_dic(bool value) {
  Button_SetCheck(m_h_one_user_dic, value ? BST_CHECKED : BST_UNCHECKED);
}

static int CALLBACK browse_callback_proc(HWND hwnd, UINT u_msg,
                                         LPARAM /*lParam*/, LPARAM lp_data) {
  // If the BFFM_INITIALIZED message is received
  // set the path to the start path.
  switch (u_msg) {
  case BFFM_INITIALIZED: {
    if (NULL != lp_data) {
      SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lp_data);
    }
  }
  default:
    break;
  }

  return 0; // The function should always return 0.
}

INT_PTR SimpleDlg::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) {
  wchar_t *end_ptr;

  switch (message) {
  case WM_INITDIALOG: {
    // Retrieving handles of dialog controls
    m_h_combo_language = ::GetDlgItem(_hSelf, IDC_COMBO_LANGUAGE);
    m_h_suggestions_num = ::GetDlgItem(_hSelf, IDC_SUGGESTIONS_NUM);
    m_h_aspell_status = ::GetDlgItem(_hSelf, IDC_ASPELL_STATUS);
    m_h_lib_path = ::GetDlgItem(_hSelf, IDC_ASPELLPATH);
    m_h_check_not_those = ::GetDlgItem(_hSelf, IDC_FILETYPES_CHECKNOTTHOSE);
    m_h_check_only_those = ::GetDlgItem(_hSelf, IDC_FILETYPES_CHECKTHOSE);
    m_h_file_types = ::GetDlgItem(_hSelf, IDC_FILETYPES);
    m_h_check_comments = ::GetDlgItem(_hSelf, IDC_CHECKCOMMENTS_CB);
    m_h_check_strings = ::GetDlgItem(_hSelf, IDC_CHECKSTRINGS_CB);
    m_h_check_varfunc = ::GetDlgItem(_hSelf, IDC_CHECKFUNCVAR_CB);
    m_h_lib_link = ::GetDlgItem(_hSelf, IDC_LIB_LINK);
    m_suggestion_mode_cmb.init(GetDlgItem(_hSelf, IDC_SUGG_TYPE));
    m_speller_cmb.init(::GetDlgItem(_hSelf, IDC_LIBRARY));
    m_h_lib_group_box = ::GetDlgItem(_hSelf, IDC_LIB_GROUPBOX);
    m_h_download_dics = ::GetDlgItem(_hSelf, IDC_DOWNLOADDICS);
    m_h_remove_dics = ::GetDlgItem(_hSelf, IDC_REMOVE_DICS);
    m_h_decode_names = ::GetDlgItem(_hSelf, IDC_DECODE_NAMES);
    m_h_one_user_dic = ::GetDlgItem(_hSelf, IDC_ONE_USER_DIC);
    m_h_hunspell_path_group_box =
        ::GetDlgItem(_hSelf, IDC_HUNSPELL_PATH_GROUPBOX);
    m_h_hunspell_path_type = ::GetDlgItem(_hSelf, IDC_HUNSPELL_PATH_TYPE);
    m_h_reset_speller_path = ::GetDlgItem(_hSelf, IDC_RESETSPELLERPATH);
    m_h_system_path = ::GetDlgItem(_hSelf, IDC_SYSTEMPATH);
    m_h_aspell_run_together_cb =
        ::GetDlgItem(_hSelf, IDC_ASPELL_RUNTOGETHER_CB);
    m_browse_btn = ::GetDlgItem(_hSelf, IDC_BROWSEASPELLPATH);

    ComboBox_AddString(m_h_hunspell_path_type, L"For Current User");
    ComboBox_AddString(m_h_hunspell_path_type, L"For All Users");
    ComboBox_SetCurSel(m_h_hunspell_path_type, 0);
    ShowWindow(m_h_lib_path, 1);
    ShowWindow(m_h_system_path, 0);
    m_default_brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

    m_aspell_status_color = RGB(0, 0, 0);
    return TRUE;
  }
  case WM_CLOSE: {
    EndDialog(_hSelf, 0);
    DeleteObject(m_default_brush);
    return FALSE;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDC_COMBO_LANGUAGE:
      if (HIWORD(w_param) == CBN_SELCHANGE) {
        if (ComboBox_GetCurSel(m_h_combo_language) ==
            ComboBox_GetCount(m_h_combo_language) - 1) {
          get_lang_list()->do_dialog();
        }
      }
      break;
    case IDC_LIBRARY:
      if (HIWORD(w_param) == CBN_SELCHANGE) {
        m_parent.apply_lib_change(m_speller_cmb.current_data());
      }
      break;
    case IDC_HUNSPELL_PATH_TYPE:
      if (HIWORD(w_param) == CBN_SELCHANGE) {
        if (ComboBox_GetCurSel(m_h_hunspell_path_type) == 0 ||
            m_settings.active_speller_lib_id == SpellerId::aspell) {
          ShowWindow(m_h_lib_path, 1);
          ShowWindow(m_h_system_path, 0);
        } else {
          ShowWindow(m_h_lib_path, 0);
          ShowWindow(m_h_system_path, 1);
        }
      }
      break;
    case IDC_SUGGESTIONS_NUM: {
      if (HIWORD(w_param) == EN_CHANGE) {
        wchar_t buf[DEFAULT_BUF_SIZE];
        Edit_GetText(m_h_suggestions_num, buf, DEFAULT_BUF_SIZE);
        if (*buf == L'\0')
          return TRUE;

        int x = wcstol(buf, &end_ptr, 10);
        if (*end_ptr != L'\0')
          Edit_SetText(m_h_suggestions_num, L"5");
        else if (x > 20)
          Edit_SetText(m_h_suggestions_num, L"20");
        else if (x < 1)
          Edit_SetText(m_h_suggestions_num, L"1");

        return TRUE;
      }
    } break;
    case IDC_REMOVE_DICS:
      if (HIWORD(w_param) == BN_CLICKED) {
        get_remove_dics()->do_dialog();
      }
      break;
    case IDC_RESETSPELLERPATH: {
      if (HIWORD(w_param) == BN_CLICKED) {
        std::wstring path;
        switch (m_settings.active_speller_lib_id) {
        case SpellerId::aspell:
          path = get_default_aspell_path();
          break;
        case SpellerId::hunspell:
          path = get_default_hunspell_path();
          break;
        case SpellerId::COUNT:
          break;
        case SpellerId::native:
          break;
        }

        if (m_settings.active_speller_lib_id == SpellerId::aspell ||
            ComboBox_GetCurSel(m_h_hunspell_path_type) == 0)
          Edit_SetText(m_h_lib_path, path.c_str());
        else
          Edit_SetText(m_h_system_path, L".\\plugins\\config\\Hunspell");
        return TRUE;
      }
    } break;
    case IDC_DOWNLOADDICS: {
      if (HIWORD(w_param) == BN_CLICKED) {
        get_download_dics()->do_dialog();
      }
    } break;
    case IDC_BROWSEASPELLPATH:
      switch (m_settings.active_speller_lib_id) {
      case SpellerId::aspell: {
        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = _hSelf;
        std::vector<wchar_t> filename(MAX_PATH);
        auto lib_path = get_edit_text(m_h_lib_path);
        std::copy(lib_path.begin(), lib_path.end(), filename.begin());
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
          Edit_SetText(m_h_lib_path, filename.data());
      } break;
      case SpellerId::hunspell: {
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
        std::vector<wchar_t> final_path(MAX_PATH);

        auto npp_path = m_npp.get_npp_directory();
        PathCombine(final_path.data(), npp_path.data(), lib_path.c_str());
        bi.lParam = reinterpret_cast<LPARAM>(final_path.data());
        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl != nullptr) {
          // get the name of the folder
          std::vector<wchar_t> sz_path(MAX_PATH);
          SHGetPathFromIDList(pidl, sz_path.data());
          Edit_SetText(m_h_lib_path, sz_path.data());
          CoTaskMemFree(pidl);
          // free memory used

          CoUninitialize();
        }
      } break;
      case SpellerId::COUNT:
        break;
      case SpellerId::native:
        break;
      default:;
      }
      break;
    }
  } break;
  case WM_NOTIFY:

    switch (reinterpret_cast<LPNMHDR>(l_param)->code) {
    case NM_CLICK: // Fall through to the next case.
    case NM_RETURN: {
      auto p_nm_link = reinterpret_cast<PNMLINK>(l_param);
      LITEM item = p_nm_link->item;

      if ((reinterpret_cast<LPNMHDR>(l_param)->hwndFrom == m_h_lib_link) &&
          (item.iLink == 0)) {
        ShellExecute(nullptr, L"open", item.szUrl, nullptr, nullptr, SW_SHOW);
      }

      break;
    }
    default:
      break;
    }
    break;
  case WM_CTLCOLORSTATIC:
    if (GetDlgItem(_hSelf, IDC_ASPELL_STATUS) ==
        reinterpret_cast<HWND>(l_param)) {
      auto h_dc = reinterpret_cast<HDC>(w_param);
      HTHEME h_theme;

      if (m_open_theme_data != nullptr)
        h_theme = m_open_theme_data(_hSelf, L"TAB");
      else
        h_theme = nullptr;

      SetBkColor(h_dc, GetSysColor(COLOR_BTNFACE));
      SetBkMode(h_dc, TRANSPARENT);
      SetTextColor(h_dc, m_aspell_status_color);
      if (h_theme != nullptr)
        return 0;

      return reinterpret_cast<INT_PTR>(m_default_brush);
    }
    break;
  default:
    break;
  }
  return 0;
}

void AdvancedDlg::set_underline_settings(int color, int style) {
  m_underline_color_btn = color;
  ComboBox_SetCurSel(m_h_underline_style, style);
}

void AdvancedDlg::set_conversion_opts(bool convert_yo,
                                      bool convert_single_quotes_arg,
                                      bool remove_boundary_apostrophes) {
  Button_SetCheck(m_h_ignore_yo, convert_yo ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_convert_single_quotes,
                  convert_single_quotes_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_remove_boundary_apostrophes,
                  remove_boundary_apostrophes ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedDlg::set_ignore(bool ignore_numbers_arg, bool ignore_c_start_arg,
                             bool ignore_c_have_arg, bool ignore_c_all_arg,
                             bool ignore_arg, bool ignore_sa_apostrophe_arg,
                             bool ignore_one_letter) {
  Button_SetCheck(m_h_ignore_numbers,
                  ignore_numbers_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_c_start,
                  ignore_c_start_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_c_have,
                  ignore_c_have_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_c_all,
                  ignore_c_all_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_one_letter,
                  ignore_one_letter ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_, ignore_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_se_apostrophe,
                  ignore_sa_apostrophe_arg ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedDlg::set_sugg_box_settings(LRESULT size, LRESULT trans) {
  SendMessage(m_h_slider_size, TBM_SETPOS, TRUE, size);
  SendMessage(m_h_slider_sugg_button_opacity, TBM_SETPOS, TRUE, trans);
}

void AdvancedDlg::set_buffer_size(int size) {
  wchar_t buf[DEFAULT_BUF_SIZE];
  _itow_s(size, buf, 10);
  Edit_SetText(m_h_buffer_size, buf);
}

void AdvancedDlg::update_controls(const Settings &settings) {
  Edit_SetText(m_h_edit_delimiters, settings.delimiters.c_str());
  Edit_SetText(m_delimiter_exclusions_le,
               m_settings.delimiter_exclusions.c_str());
  Button_SetCheck(m_split_camel_case_cb, m_settings.split_camel_case);
  set_recheck_delay(settings.recheck_delay);
  set_conversion_opts(settings.ignore_yo, settings.convert_single_quotes,
                      settings.remove_boundary_apostrophes);
  set_underline_settings(settings.underline_color, settings.underline_style);
  set_ignore(settings.ignore_containing_digit,
             settings.ignore_starting_with_capital,
             settings.ignore_having_a_capital, settings.ignore_all_capital,
             settings.ignore_having_underscore,
             settings.ignore_starting_or_ending_with_apostrophe,
             settings.ignore_one_letter);
  set_sugg_box_settings(settings.suggestion_button_size,
                        settings.suggestion_button_opacity);
  set_buffer_size(settings.find_next_buffer_size);
  m_tokenization_style_cmb.set_index(settings.tokenization_style);
  setup_delimiter_line_edit_visiblity();
}

const wchar_t *const indic_names[] = {
    L"Plain",        L"Squiggle", L"TT",   L"Diagonal",
    L"Strike",       L"Hidden",   L"Box",  L"Round Box",
    L"Straight Box", L"Dash",     L"Dots", L"Squiggle Low"};

void AdvancedDlg::setup_delimiter_line_edit_visiblity() {
  ShowWindow(m_delimiter_exclusions_le,
             m_tokenization_style_cmb.current_data() ==
                 TokenizationStyle::by_non_alphabetic ? TRUE : FALSE);
  ShowWindow(m_h_edit_delimiters, m_tokenization_style_cmb.current_data() ==
                                      TokenizationStyle::by_delimiters ? TRUE : FALSE);
}

INT_PTR AdvancedDlg::run_dlg_proc(UINT message, WPARAM w_param,
                                  LPARAM l_param) {
  wchar_t *end_ptr = nullptr;
  wchar_t buf[DEFAULT_BUF_SIZE];
  int x;
  switch (message) {
  case WM_INITDIALOG: {
    // Retrieving handles of dialog controls
    m_h_edit_delimiters = ::GetDlgItem(_hSelf, IDC_DELIMITERS);
    m_delimiter_exclusions_le =
        ::GetDlgItem(_hSelf, IDC_DELIMITER_EXCLUSIONS_LE);
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
    m_h_slider_sugg_button_opacity =
        ::GetDlgItem(_hSelf, IDC_SLIDER_TRANSPARENCY);
    m_h_buffer_size = ::GetDlgItem(_hSelf, IDC_BUFFER_SIZE);
    m_tokenization_style_cmb.init(
        ::GetDlgItem(_hSelf, IDC_TOKENIZATION_STYLE_CMB));
    SendMessage(m_h_slider_size, TBM_SETRANGE, TRUE, MAKELPARAM(5, 22));
    SendMessage(m_h_slider_sugg_button_opacity, TBM_SETRANGE, TRUE,
                MAKELPARAM(5, 100));
    m_split_camel_case_cb = ::GetDlgItem(_hSelf, IDC_CAMEL_CASE_SPLITTING_CB);

    m_brush = nullptr;

    SetWindowText(m_h_ignore_yo, L"Cyrillic: Count io (\u0451) as e");
    create_tool_tip(IDC_DELIMITERS, _hSelf,
                    L"Standard white-space symbols such as New Line ('\\n'), "
                    L"Carriage Return ('\\r'), Tab ('\\t'), Space (' ') are "
                    L"always counted as delimiters");
    create_tool_tip(IDC_RECHECK_DELAY, _hSelf,
                    L"Delay between the end of typing "
                    L"and rechecking the the text "
                    L"after it");
    create_tool_tip(IDC_IGNORE_CSTART, _hSelf,
                    L"Remember that words at the "
                    L"beginning of sentences would "
                    L"also be ignored that way.");
    create_tool_tip(
        IDC_IGNORE_SE_APOSTROPHE, _hSelf,
        L"Words like this sadly cannot be added to Aspell user dictionary");
    create_tool_tip(
        IDC_REMOVE_ENDING_APOSTROPHE, _hSelf,
        L"Words like this are mostly mean plural possessive form in "
        L"English, if you want to add such forms of words to "
        L"dictionary manually, please uncheck");

    ComboBox_ResetContent(m_h_underline_style);
    for (auto name : indic_names)
      ComboBox_AddString(m_h_underline_style, name);
    return TRUE;
  }
  case WM_CLOSE: {
    if (m_brush != nullptr)
      DeleteObject(m_brush);
    EndDialog(_hSelf, 0);
    return FALSE;
  }
  case WM_DRAWITEM:
    switch (w_param) {
    case IDC_UNDERLINE_COLOR:
      PAINTSTRUCT ps;
      auto ds = reinterpret_cast<LPDRAWITEMSTRUCT>(l_param);
      HDC dc = ds->hDC;
      /*
      Pen = CreatePen (PS_SOLID, 1, RGB (128, 128, 128));
      SelectPen (Dc, Pen);
      */
      if ((ds->itemState & ODS_SELECTED) != 0u) {
        DrawEdge(dc, &ds->rcItem, BDR_SUNKENINNER | BDR_SUNKENOUTER, BF_RECT);
      } else {
        DrawEdge(dc, &ds->rcItem, BDR_RAISEDINNER | BDR_RAISEDOUTER, BF_RECT);
      }
      // DeleteObject (Pen);
      EndPaint(m_h_underline_color, &ps);
      return 1;
    }
    break;
  case WM_CTLCOLORBTN:
    if (GetDlgItem(_hSelf, IDC_UNDERLINE_COLOR) == reinterpret_cast<HWND>(l_param)) {
      auto h_dc = reinterpret_cast<HDC>(w_param);
      if (m_brush != nullptr)
        DeleteObject(m_brush);

      SetBkColor(h_dc, m_underline_color_btn);
      SetBkMode(h_dc, TRANSPARENT);
      m_brush = CreateSolidBrush(m_underline_color_btn);
      return reinterpret_cast<INT_PTR>(m_brush);
    }
    break;
  case WM_COMMAND:
    switch (LOWORD(w_param)) {
    case IDC_TOKENIZATION_STYLE_CMB: {
      setup_delimiter_line_edit_visiblity();
    }
    case IDC_DEFAULT_DELIMITERS:
      if (HIWORD(w_param) == BN_CLICKED) {
        switch (m_tokenization_style_cmb.current_data()) {
        case TokenizationStyle::by_non_alphabetic:
          Edit_SetText(m_delimiter_exclusions_le,
                       default_delimiter_exclusions());
          break;
        case TokenizationStyle::by_delimiters:
          Edit_SetText(m_h_edit_delimiters, default_delimiters());
          break;
        case TokenizationStyle::COUNT:
          break;
        }
        return 1;
      }
    case IDC_RECHECK_DELAY:
      if (HIWORD(w_param) == EN_CHANGE) {
        Edit_GetText(m_h_recheck_delay, buf, DEFAULT_BUF_SIZE);
        if (*buf == 0u)
          return 1;

        x = wcstol(buf, &end_ptr, 10);
        if (*end_ptr != 0u)
          Edit_SetText(m_h_recheck_delay, L"0");
        else if (x > 30000)
          Edit_SetText(m_h_recheck_delay, L"30000");
        else if (x < 0)
          Edit_SetText(m_h_recheck_delay, L"0");

        return 1;
      }
      break;
    case IDC_BUFFER_SIZE:
      if (HIWORD(w_param) == EN_CHANGE) {
        Edit_GetText(m_h_buffer_size, buf, DEFAULT_BUF_SIZE);
        if (*buf == 0u)
          return 1;

        x = wcstol(buf, &end_ptr, 10);
        if (*end_ptr != 0u)
          Edit_SetText(m_h_buffer_size, L"1024");
        else if (x > 10 * 1024)
          Edit_SetText(m_h_buffer_size, L"10240");
        else if (x < 1)
          Edit_SetText(m_h_buffer_size, L"1");

        return 1;
      }
      break;
    case IDC_UNDERLINE_COLOR:
      if (HIWORD(w_param) == BN_CLICKED) {
        CHOOSECOLOR cc;
        static COLORREF acr_cust_clr[16];
        memset(&cc, 0, sizeof(cc));
        cc.hwndOwner = _hSelf;
        cc.lStructSize = sizeof(CHOOSECOLOR);
        cc.rgbResult = m_underline_color_btn;
        cc.lpCustColors = acr_cust_clr;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColor(&cc)) {
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
  return 0;
}

void AdvancedDlg::set_delimeters_edit(const wchar_t *delimiters) {
  Edit_SetText(m_h_edit_delimiters, delimiters);
}

void AdvancedDlg::set_recheck_delay(int delay) {
  wchar_t buf[DEFAULT_BUF_SIZE];
  _itow(delay, buf, 10);
  Edit_SetText(m_h_recheck_delay, buf);
}

int AdvancedDlg::get_recheck_delay() {
  wchar_t buf[DEFAULT_BUF_SIZE];
  Edit_GetText(m_h_recheck_delay, buf, DEFAULT_BUF_SIZE);
  wchar_t *end_ptr;
  int x = wcstol(buf, &end_ptr, 10);
  return x;
}

AdvancedDlg::AdvancedDlg(const Settings &settings) : m_settings(settings) {}

void AdvancedDlg::apply_settings(Settings &settings) {
  settings.delimiters = get_edit_text(m_h_edit_delimiters);
  settings.ignore_yo = Button_GetCheck(m_h_ignore_yo) == BST_CHECKED;
  settings.convert_single_quotes =
      Button_GetCheck(m_h_convert_single_quotes) == BST_CHECKED;
  settings.remove_boundary_apostrophes =
      Button_GetCheck(m_h_remove_boundary_apostrophes) == BST_CHECKED;
  settings.underline_color = m_underline_color_btn;
  settings.underline_style = ComboBox_GetCurSel(m_h_underline_style);
  settings.recheck_delay = get_recheck_delay();
  settings.ignore_containing_digit =
      Button_GetCheck(m_h_ignore_numbers) == BST_CHECKED;
  settings.ignore_starting_with_capital =
      Button_GetCheck(m_h_ignore_c_start) == BST_CHECKED;
  settings.ignore_having_a_capital =
      Button_GetCheck(m_h_ignore_c_have) == BST_CHECKED;
  settings.ignore_all_capital =
      Button_GetCheck(m_h_ignore_c_all) == BST_CHECKED;
  settings.ignore_having_underscore =
      Button_GetCheck(m_h_ignore_) == BST_CHECKED;
  settings.ignore_starting_or_ending_with_apostrophe =
      Button_GetCheck(m_h_ignore_se_apostrophe) == BST_CHECKED;
  settings.ignore_one_letter =
      Button_GetCheck(m_h_ignore_one_letter) == BST_CHECKED;
  settings.suggestion_button_size =
      static_cast<int>(SendMessage(m_h_slider_size, TBM_GETPOS, 0, 0));
  settings.suggestion_button_opacity = static_cast<int>(
      SendMessage(m_h_slider_sugg_button_opacity, TBM_GETPOS, 0, 0));
  settings.tokenization_style = m_tokenization_style_cmb.current_data();
  wchar_t *end_ptr = nullptr;
  settings.find_next_buffer_size =
      wcstol(get_edit_text(m_h_buffer_size).c_str(), &end_ptr, 10);
  settings.delimiter_exclusions = get_edit_text(m_delimiter_exclusions_le);
  settings.split_camel_case =
      Button_GetCheck(m_split_camel_case_cb) == BST_CHECKED;
}

SimpleDlg *SettingsDlg::get_simple_dlg() { return &m_simple_dlg; }

AdvancedDlg *SettingsDlg::get_advanced_dlg() { return &m_advanced_dlg; }

SettingsDlg::SettingsDlg(HINSTANCE h_inst, HWND parent, NppInterface &npp,
                         const Settings &settings)
    : m_npp(npp), m_simple_dlg(*this, settings, m_npp),
      m_advanced_dlg(settings), m_settings(settings) {
  Window::init(h_inst, parent);
  m_settings.settings_changed.connect([this] { update_controls(); });
  get_spell_checker()->speller_status_changed.connect([this] {
    m_simple_dlg.update_language_controls(m_settings);
    m_simple_dlg.update_lib_status(m_settings);
  });
}

void SettingsDlg::destroy() {
  m_simple_dlg.destroy();
  m_advanced_dlg.destroy();
};

// Send appropriate event and set some npp thread properties
void SettingsDlg::apply_settings() {
  auto mut_settings = m_settings.modify();
  m_simple_dlg.apply_settings(*mut_settings);
  m_advanced_dlg.apply_settings(*mut_settings);
}

void SettingsDlg::update_controls() {
  m_simple_dlg.update_controls(m_settings);
  m_advanced_dlg.update_controls(m_settings);
}

void SettingsDlg::apply_lib_change(SpellerId new_lib_id) {
  auto mut_settings = m_settings.modify();
  mut_settings->active_speller_lib_id = new_lib_id;
}

void SimpleDlg::init_settings(HINSTANCE h_inst, HWND parent) {
  return Window::init(h_inst, parent);
}

void SimpleDlg::update_lib_status(const Settings &settings) {
  fill_lib_info(get_spell_checker()->get_aspell_status(), settings);
}

void SimpleDlg::update_controls(const Settings &settings) {
  m_speller_cmb.set_index(settings.active_speller_lib_id);
  update_lib_status(settings);
  fill_sugestions_num(settings.suggestion_count);
  set_file_types(settings.check_those, settings.file_types.c_str());
  Button_SetCheck(m_h_check_comments,
                  settings.check_comments ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_check_strings,
                  settings.check_strings ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_check_varfunc, settings.check_variable_functions
                                         ? BST_CHECKED
                                         : BST_UNCHECKED);
  set_decode_names(settings.use_language_name_aliases);
  set_sugg_type(settings.suggestions_mode);
  set_one_user_dic(settings.use_unified_dictionary);
  Button_SetCheck(m_h_aspell_run_together_cb,
                  settings.aspell_allow_run_together_words);
}

INT_PTR SettingsDlg::run_dlg_proc(UINT message, WPARAM w_param,
                                  LPARAM l_param) {
  switch (message) {
  case WM_INITDIALOG: {
    m_controls_tab.initTabBar(_hInst, _hSelf, false, true, false);
    m_controls_tab.setFont(L"Tahoma", 13);

    m_simple_dlg.init_settings(_hInst, _hSelf);
    m_simple_dlg.create(IDD_SIMPLE, false, false);
    m_simple_dlg.display();
    m_advanced_dlg.init(_hInst, _hSelf);
    m_advanced_dlg.create(IDD_ADVANCED, false, false);

    m_window_vector.emplace_back(&m_simple_dlg, TEXT("Simple"), TEXT("Simple Options"));
    m_window_vector.emplace_back(&m_advanced_dlg, TEXT("Advanced"), TEXT("Advanced Options"));
    m_controls_tab.createTabs(m_window_vector);
    m_controls_tab.display();
    RECT rc;
    getClientRect(rc);
    m_controls_tab.reSizeTo(rc);
    rc.bottom -= 30;

    m_simple_dlg.reSizeTo(rc);
    m_advanced_dlg.reSizeTo(rc);

    // This stuff is copied from npp source to make tabbed window looked totally
    // nice and white
    auto enable_dlg_theme =
        reinterpret_cast<ETDTProc>(::SendMessage(_hParent, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0));
    if (enable_dlg_theme != nullptr)
      enable_dlg_theme(_hSelf, ETDT_ENABLETAB);

    update_controls();
    m_simple_dlg.update_language_controls(m_settings);

    return 1;
  }
  case WM_CONTEXTMENU: {
    HMENU menu = CreatePopupMenu();
    AppendMenu(menu, MF_STRING, 0,
               L"Copy All Misspelled Words in Current Document to Clipboard");
    TrackPopupMenuEx(menu, 0, GET_X_LPARAM(l_param), GET_Y_LPARAM(l_param),
                     _hSelf, nullptr);
  } break;
  case WM_NOTIFY: {
    auto nmhdr = reinterpret_cast<NMHDR *>(l_param);
    if (nmhdr->code == TCN_SELCHANGE) {
      if (nmhdr->hwndFrom == m_controls_tab.getHSelf()) {
        m_controls_tab.clickedUpdate();
        return 0;
      }
    }
    break;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case 0: // Menu
    {
      get_spell_checker()->copy_misspellings_to_clipboard();
    } break;
    case IDAPPLY:
      if (HIWORD(w_param) == BN_CLICKED) {
        apply_settings();

        return 0;
      }
      break;
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        display(false);
        apply_settings();
        return 0;
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        display(false);
        return 0;
      }
      break;
    }
  }
  }
  return 0;
}

UINT SettingsDlg::do_dialog() {
  if (!isCreated()) {
    create(IDD_SETTINGS);
    goToCenter();
  } else {
    goToCenter();
    update_controls();
  }

  display();
  return 1u;
}
