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

#include "SettingsDialog.h"

#include "aspell.h"
#include "AspellOptionsDialog.h"
#include "DownloadDictionariesDialog.h"
#include "RemoveDictionariesDialog.h"
#include "SelectMultipleLanguagesDialog.h"
#include "common/IniWorker.h"
#include "common/winapi.h"
#include "common/WindowsDefs.h"
#include "npp/NppInterface.h"
#include "plugin/Plugin.h"
#include "plugin/resource.h"
#include "plugin/Settings.h"
#include "spellers/LanguageInfo.h"
#include "spellers/NativeSpellerInterface.h"
#include "spellers/SpellerContainer.h"

#include <cassert>
#include <Uxtheme.h>

SimpleSettingsTab::SimpleSettingsTab(SettingsDialog &parent, const Settings &settings, NppInterface &npp)
  : m_npp(npp), m_settings(settings), m_parent(parent) {
  m_h_ux_theme = ::LoadLibrary(TEXT("uxtheme.dll"));
  if (m_h_ux_theme != nullptr)
    m_open_theme_data = reinterpret_cast<OtdProc>(::GetProcAddress(m_h_ux_theme, "OpenThemeData"));
}

void SimpleSettingsTab::disable_language_combo(bool disable) {
  auto enable = disable ? FALSE : TRUE;
  m_language_cmb.clear();
  m_language_cmb.set_enabled(enable);
  ListBox_ResetContent(get_remove_dics()->get_list_box());
  EnableWindow(m_h_remove_dics, enable);
}

void SimpleSettingsTab::update_language_controls(const Settings &settings, const SpellerContainer &speller_container,
                                                 const std::wstring &selected_language_name) {

  if (!m_language_cmb)
    return;

  m_language_cmb.clear();
  int selected_index = 0;

  auto langs_available = speller_container.get_available_languages();

  int i = 0;
  const auto &target_language = selected_language_name.empty() ? settings.get_active_language() : selected_language_name;

  for (auto &lang : langs_available) {
    if (target_language == lang.orig_name) {
      selected_index = i;
    }

    m_language_cmb.add_item(lang.alias_name.c_str(), i);
    ++i;
  }
  if (target_language == multiple_language_alias)
    selected_index = i;

  if (!langs_available.empty())
    m_language_cmb.add_item(rc_str(IDS_MULTIPLE_LANGUAGES).c_str(), -1);

  m_language_cmb.set_current_index(selected_index);
  m_language_cmb.set_enabled(!langs_available.empty());
}

SimpleSettingsTab::~SimpleSettingsTab() {
  if (m_h_ux_theme != nullptr)
    FreeLibrary(m_h_ux_theme);
}

void SimpleSettingsTab::apply_settings(Settings &settings, const SpellerContainer &speller_container) {
  int lang_count = m_language_cmb.count();
  int cur_sel = m_language_cmb.current_index();

  if (m_language_cmb.is_enabled()) {
    std::wstring new_lang = (cur_sel == lang_count - 1 ? multiple_language_alias : speller_container.get_available_languages()[cur_sel].orig_name.c_str());
    settings.get_active_language() = new_lang;
    m_parent.m_selected_language_name = new_lang;
  }
  settings.data.suggestion_count = (_wtoi(WinApi::get_edit_text(m_h_suggestions_num).c_str()));
  switch (settings.data.active_speller_lib_id) {
  case SpellerId::aspell:
    settings.data.aspell_dll_path = WinApi::get_edit_text(m_h_lib_path);
    break;
  case SpellerId::hunspell:
    settings.data.hunspell_user_path = WinApi::get_edit_text(m_h_lib_path);
    settings.data.hunspell_system_path = WinApi::get_edit_text(m_h_system_path);
    break;
  case SpellerId::native:
    break;
  case SpellerId::COUNT:
    break;
  default: ;
  }

  settings.data.check_those = Button_GetCheck(m_h_check_only_those) == BST_CHECKED;
  settings.data.file_types = WinApi::get_edit_text(m_h_file_types);
  settings.data.suggestions_mode = m_suggestion_mode_cmb.current_data();
  settings.data.check_comments = Button_GetCheck(m_h_check_comments) == BST_CHECKED;
  settings.data.check_strings = Button_GetCheck(m_h_check_strings) == BST_CHECKED;
  settings.data.check_variable_functions = Button_GetCheck(m_h_check_varfunc) == BST_CHECKED;
  settings.data.use_unified_dictionary = Button_GetCheck(m_h_one_user_dic) == BST_CHECKED;
  settings.data.select_word_on_context_menu_click = Button_GetCheck(m_h_select_misspelled_on_menu_cb) == BST_CHECKED;
  settings.data.language_name_style = m_language_name_style_cmb.current_data();
}

void SimpleSettingsTab::fill_lib_info(AspellStatus aspell_status, const Settings &settings) {

  auto is_aspell = settings.data.active_speller_lib_id == SpellerId::aspell ? TRUE : FALSE;
  ShowWindow(m_h_lib_link, is_aspell);
  ShowWindow(m_h_aspell_status, is_aspell);
  auto is_hunspell = settings.data.active_speller_lib_id == SpellerId::hunspell ? TRUE : FALSE;
  ShowWindow(m_h_download_dics, is_hunspell);
  ShowWindow(m_h_remove_dics, is_hunspell);
  ShowWindow(m_h_one_user_dic, is_hunspell);
  ShowWindow(m_h_hunspell_path_group_box, is_hunspell);
  ShowWindow(m_h_hunspell_path_type, is_hunspell);
  ShowWindow(m_h_system_path, 0);
  ShowWindow(m_h_configure_aspell_btn, settings.data.active_speller_lib_id == SpellerId::aspell);
  auto not_native = settings.data.active_speller_lib_id == SpellerId::native ? FALSE : TRUE;
  ShowWindow(m_h_lib_group_box, not_native);
  ShowWindow(m_h_lib_path, not_native);
  ShowWindow(m_h_reset_speller_path, not_native);
  ShowWindow(m_h_browse_btn, not_native);
  BOOL configure_enabled = FALSE;

  switch (settings.data.active_speller_lib_id) {
  case SpellerId::aspell:

    switch (aspell_status) {
    case AspellStatus::working:
      m_aspell_status_color = COLOR_OK;
      configure_enabled = TRUE;
      Static_SetText(m_h_aspell_status, rc_str(IDS_ASPELL_STATUS_OK).c_str());
      break;
    case AspellStatus::no_dictionaries:
      m_aspell_status_color = COLOR_FAIL;
      configure_enabled = TRUE;
      Static_SetText(m_h_aspell_status, rc_str(IDS_ASPELL_STATUS_NO_DICTS).c_str());
      break;
    case AspellStatus::not_working:
      m_aspell_status_color = COLOR_FAIL;
      Static_SetText(m_h_aspell_status, rc_str(IDS_ASPELL_STATUS_FAIL).c_str());
      break;
    case AspellStatus::incorrect_bitness:
      m_aspell_status_color = COLOR_FAIL;
      Static_SetText(m_h_aspell_status, rc_str(IDS_ASPELL_INCORRECT_BITNESS).c_str());
      break;
    }
    EnableWindow(m_h_configure_aspell_btn, configure_enabled);
    Edit_SetText(m_h_lib_path, get_actual_aspell_path(settings.data.aspell_dll_path).c_str());

    Static_SetText(m_h_lib_group_box, rc_str(IDS_ASPELL_LOCATION).c_str());
    break;
  case SpellerId::hunspell: {
    auto is_lib_path = ComboBox_GetCurSel(m_h_hunspell_path_type) == 0;
    ShowWindow(m_h_lib_path, is_lib_path);
    ShowWindow(m_h_system_path, !is_lib_path);
    Static_SetText(m_h_lib_group_box, rc_str(IDS_HUNSPELL_SETTINGS).c_str());
    Edit_SetText(m_h_lib_path, settings.data.hunspell_user_path.c_str());
  }
  break;
  case SpellerId::native:
    break;
  case SpellerId::COUNT:
    break;
  default: ;
  }
  Edit_SetText(m_h_system_path, settings.data.hunspell_system_path.c_str());
}

void SimpleSettingsTab::fill_sugestions_num(int suggestions_num) {
  wchar_t buf[10];
  _itow_s(suggestions_num, buf, 10);
  Edit_SetText(m_h_suggestions_num, buf);
}

void SimpleSettingsTab::set_file_types(bool check_those, const wchar_t *file_types) {
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

void SimpleSettingsTab::set_sugg_type(SuggestionMode mode) { m_suggestion_mode_cmb.set_current(mode); }

void SimpleSettingsTab::set_one_user_dic(bool value) {
  Button_SetCheck(m_h_one_user_dic, value ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedSettingsTab::reset_settings() {
  if (MessageBox(_hParent, rc_str(IDS_RESET_SETTINGS_TEXT).c_str(), rc_str(IDS_RESET_SETTINGS_CAPTION).c_str(), MB_YESNO) == IDNO)
    return;
  auto mut = m_settings.modify();
  IniWorker worker{L"", L"", IniWorker::Action::load};
  mut->process(worker);
}

INT_PTR SimpleSettingsTab::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) {
  wchar_t *end_ptr;

  switch (message) {
  case WM_INITDIALOG: {
    // Retrieving handles of dialog controls
    m_language_cmb.init(GetDlgItem(_hSelf, IDC_COMBO_LANGUAGE));
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
    m_language_name_style_cmb.init(GetDlgItem(_hSelf, IDC_LANGUAGE_NAME_STYLE));
    m_h_select_misspelled_on_menu_cb = ::GetDlgItem(_hSelf, IDC_SELECT_MISSPELLED_ON_MENU);
    m_language_name_style_cmb.clear();
    for (auto val : enum_range<LanguageNameStyle>()) {
      if ([&]() {
        switch (val) {
        case LanguageNameStyle::original:
        case LanguageNameStyle::english:
          return true;
        case LanguageNameStyle::native:
        case LanguageNameStyle::localized:
          return WinApi::is_locale_info_available();
        case LanguageNameStyle::COUNT:
          break;
        }
        return false;
      }())
        m_language_name_style_cmb.add_item(val);
    }

    m_h_lib_group_box = ::GetDlgItem(_hSelf, IDC_LIB_GROUPBOX);
    m_h_download_dics = ::GetDlgItem(_hSelf, IDC_DOWNLOADDICS);
    m_h_remove_dics = ::GetDlgItem(_hSelf, IDC_REMOVE_DICS);
    m_h_one_user_dic = ::GetDlgItem(_hSelf, IDC_ONE_USER_DIC);
    m_h_hunspell_path_group_box = ::GetDlgItem(_hSelf, IDC_HUNSPELL_PATH_GROUPBOX);
    m_h_hunspell_path_type = ::GetDlgItem(_hSelf, IDC_HUNSPELL_PATH_TYPE);
    m_h_reset_speller_path = ::GetDlgItem(_hSelf, IDC_RESETSPELLERPATH);
    m_h_system_path = ::GetDlgItem(_hSelf, IDC_SYSTEMPATH);
    m_h_configure_aspell_btn = GetDlgItem(_hSelf, IDC_CONFIGURE_ASPELL);
    m_h_browse_btn = ::GetDlgItem(_hSelf, IDC_BROWSEASPELLPATH);

    ComboBox_AddString(m_h_hunspell_path_type, rc_str(IDS_FOR_CURRENT_USER).c_str());
    ComboBox_AddString(m_h_hunspell_path_type, rc_str(IDS_FOR_ALL_USERS).c_str());
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
        m_parent.store_selected_language_name(m_language_cmb.current_data());
        if (m_language_cmb.current_data() == -1) {
          get_lang_list()->do_dialog();
        }
      }
      break;
    case IDC_LANGUAGE_NAME_STYLE:
      if (HIWORD(w_param) == CBN_SELCHANGE) {
        auto mut = m_settings.modify();
        mut->data.language_name_style = m_language_name_style_cmb.current_data();
      }
      break;
    case IDC_LIBRARY:
      if (HIWORD(w_param) == CBN_SELCHANGE) {
        m_parent.apply_lib_change(m_speller_cmb.current_data());
      }
      break;
    case IDC_HUNSPELL_PATH_TYPE:
      if (HIWORD(w_param) == CBN_SELCHANGE) {
        if (ComboBox_GetCurSel(m_h_hunspell_path_type) == 0 || m_settings.data.active_speller_lib_id == SpellerId::aspell) {
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
        auto text = WinApi::get_edit_text(m_h_suggestions_num);
        if (text.empty())
          return TRUE;

        int x = wcstol(text.c_str(), &end_ptr, 10);
        if (*end_ptr != L'\0')
          Edit_SetText(m_h_suggestions_num, L"5");
        else if (x > 20)
          Edit_SetText(m_h_suggestions_num, L"20");
        else if (x < 1)
          Edit_SetText(m_h_suggestions_num, L"1");

        return TRUE;
      }
    }
    break;
    case IDC_CONFIGURE_ASPELL: {
      if (HIWORD(w_param) == BN_CLICKED) {
        get_aspell_options_dlg()->do_dialog();
      }
    }
    break;
    case IDC_REMOVE_DICS:
      if (HIWORD(w_param) == BN_CLICKED) {
        get_remove_dics()->do_dialog();
      }
      break;
    case IDC_RESETSPELLERPATH: {
      if (HIWORD(w_param) == BN_CLICKED) {
        std::wstring path;
        switch (m_settings.data.active_speller_lib_id) {
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

        if (m_settings.data.active_speller_lib_id == SpellerId::aspell || ComboBox_GetCurSel(m_h_hunspell_path_type) == 0)
          Edit_SetText(m_h_lib_path, path.c_str());
        else
          Edit_SetText(m_h_system_path, L".\\plugins\\config\\Hunspell");
        return TRUE;
      }
    }
    break;
    case IDC_DOWNLOADDICS: {
      if (HIWORD(w_param) == BN_CLICKED) {
        m_parent.download_dics_dlg_requested();
      }
    }
    break;
    case IDC_BROWSEASPELLPATH:
      switch (m_settings.data.active_speller_lib_id) {
      case SpellerId::aspell: {
        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = _hSelf;
        std::vector<wchar_t> filename(MAX_PATH);
        auto lib_path = WinApi::get_edit_text(m_h_lib_path);
        std::copy(lib_path.begin(), lib_path.end(), filename.begin());
        auto aspell_library_desc = rc_str(IDS_ASPELL_LIBRARY);
        std::vector<wchar_t> buf;
        buf.insert(buf.end(), aspell_library_desc.begin(), aspell_library_desc.end());
        wchar_t rest[] = L" (*.dll)\0*.dll\0\0";
        buf.insert(buf.end(), std::begin(rest), std::end(rest));
        // TODO: add possibility to use modern browse dialog
        ofn.lpstrFile = filename.data();
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = buf.data();
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = nullptr;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = nullptr;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (GetOpenFileName(&ofn))
          Edit_SetText(m_h_lib_path, filename.data());
      }
      break;
      case SpellerId::hunspell: {
        auto lib_path = WinApi::get_edit_text(m_h_lib_path);
        std::vector<wchar_t> final_path(MAX_PATH);

        auto npp_path = m_npp.get_npp_directory();
        PathCombine(final_path.data(), npp_path.data(), lib_path.c_str());

        auto path = WinApi::browse_for_directory(_hSelf, final_path.data());
        if (path)
          Edit_SetText(m_h_lib_path, path->data());
      }
      break;
      case SpellerId::COUNT:
        break;
      case SpellerId::native:
        break;
      default: ;
      }
      break;
    }
  }
  break;
  case WM_NOTIFY:

    switch (reinterpret_cast<LPNMHDR>(l_param)->code) {
    case NM_CLICK: // Fall through to the next case.
    case NM_RETURN: {
      auto p_nm_link = reinterpret_cast<PNMLINK>(l_param);
      LITEM item = p_nm_link->item;

      if ((reinterpret_cast<LPNMHDR>(l_param)->hwndFrom == m_h_lib_link) && (item.iLink == 0)) {
        ShellExecute(nullptr, L"open", item.szUrl, nullptr, nullptr, SW_SHOW);
      }

      break;
    }
    default:
      break;
    }
    break;
  case WM_CTLCOLORSTATIC:
    if (GetDlgItem(_hSelf, IDC_ASPELL_STATUS) == reinterpret_cast<HWND>(l_param)) {
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

void AdvancedSettingsTab::set_underline_settings(int color, int style) {
  m_underline_color_btn = color;
  ComboBox_SetCurSel(m_h_underline_style, style);
}

void AdvancedSettingsTab::set_conversion_opts(bool convert_yo, bool convert_single_quotes_arg, bool remove_boundary_apostrophes) {
  Button_SetCheck(m_h_ignore_yo, convert_yo ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_convert_single_quotes, convert_single_quotes_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_remove_boundary_apostrophes, remove_boundary_apostrophes ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedSettingsTab::set_ignore(bool ignore_numbers_arg, bool ignore_c_start_arg, bool ignore_c_have_arg, bool ignore_c_all_arg, bool ignore_arg,
                                     bool ignore_sa_apostrophe_arg, bool ignore_one_letter) {
  Button_SetCheck(m_h_ignore_numbers, ignore_numbers_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_c_start, ignore_c_start_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_c_have, ignore_c_have_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_c_all, ignore_c_all_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_one_letter, ignore_one_letter ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_, ignore_arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_ignore_se_apostrophe, ignore_sa_apostrophe_arg ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedSettingsTab::set_sugg_box_settings(LRESULT size, LRESULT trans) {
  SendMessage(m_h_slider_size, TBM_SETPOS, TRUE, size);
  SendMessage(m_h_slider_sugg_button_opacity, TBM_SETPOS, TRUE, trans);
}

void AdvancedSettingsTab::update_controls(const Settings &settings) {
  Edit_SetText(m_h_edit_delimiters, settings.data.delimiters.c_str());
  Edit_SetText(m_delimiter_exclusions_le, m_settings.data.delimiter_exclusions.c_str());
  Edit_SetText(m_ignore_regexp_edit, m_settings.data.ignore_regexp_str.c_str());
  Button_SetCheck(m_split_camel_case_cb, m_settings.data.split_camel_case);
  set_recheck_delay(settings.data.recheck_delay);
  set_conversion_opts(settings.data.ignore_yo, settings.data.convert_single_quotes, settings.data.remove_boundary_apostrophes);
  set_underline_settings(settings.data.underline_color, settings.data.underline_style);
  Button_SetCheck(m_h_check_default_udl_style, settings.data.check_default_udl_style);
  set_ignore(settings.data.ignore_containing_digit, settings.data.ignore_starting_with_capital, settings.data.ignore_having_a_capital,
             settings.data.ignore_all_capital,
             settings.data.ignore_having_underscore, settings.data.ignore_starting_or_ending_with_apostrophe, settings.data.ignore_one_letter);
  set_sugg_box_settings(settings.data.suggestion_button_size, settings.data.suggestion_button_opacity);
  m_tokenization_style_cmb.set_current(settings.data.tokenization_style);
  setup_delimiter_line_edit_visiblity();
  RedrawWindow(m_ignore_regexp_edit, nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE | RDW_ERASE);
}

const std::vector<std::wstring> &get_indic_names() {
  static std::vector<std::wstring> indic_names = {rc_str(IDS_PLAIN), rc_str(IDS_SQUIGGLE), rc_str(IDS_TT), rc_str(IDS_DIAGONAL),
                                                  rc_str(IDS_STRIKE), rc_str(IDS_HIDDEN), rc_str(IDS_BOX), rc_str(IDS_ROUND_BOX),
                                                  rc_str(IDS_STRAIGHT_BOX), rc_str(IDS_DASH), rc_str(IDS_DOTS), rc_str(IDS_SQUIGGLE_LOW)};
  return indic_names;
}

void AdvancedSettingsTab::setup_delimiter_line_edit_visiblity() {
  ShowWindow(m_delimiter_exclusions_le, m_tokenization_style_cmb.current_data() != TokenizationStyle::by_delimiters ? TRUE : FALSE);
  ShowWindow(m_h_edit_delimiters, m_tokenization_style_cmb.current_data() == TokenizationStyle::by_delimiters ? TRUE : FALSE);
}

void AdvancedSettingsTab::on_recheck_delay_changed() {
  auto text = WinApi::get_edit_text(m_h_recheck_delay);
  if (text.empty())
    return;

  wchar_t *end_ptr = nullptr;
  auto x = wcstol(text.c_str(), &end_ptr, 10);
  if (*end_ptr != 0u)
    Edit_SetText(m_h_recheck_delay, L"0");
  else if (x > 30000)
    Edit_SetText(m_h_recheck_delay, L"30000");
  else if (x < 0)
    Edit_SetText(m_h_recheck_delay, L"0");
}

void AdvancedSettingsTab::on_default_delimiters_clicked() {
  switch (m_tokenization_style_cmb.current_data()) {
  case TokenizationStyle::by_non_alphabetic:
  case TokenizationStyle::by_non_ansi:
    Edit_SetText(m_delimiter_exclusions_le, default_delimiter_exclusions());
    break;
  case TokenizationStyle::by_delimiters:
    Edit_SetText(m_h_edit_delimiters, default_delimiters());
    break;
  case TokenizationStyle::COUNT:
    break;
  }
}

void AdvancedSettingsTab::on_underline_color_button_clicked() {
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
    RedrawWindow(m_h_underline_color, nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE | RDW_ERASE);
  }
}

INT_PTR AdvancedSettingsTab::setup_underline_button_colors(HDC h_dc) {

  SetBkColor(h_dc, m_underline_color_btn);
  SetBkMode(h_dc, TRANSPARENT);
  m_underline_color_brush = {CreateSolidBrush(m_underline_color_btn)};
  return reinterpret_cast<INT_PTR>(m_underline_color_brush.get ());
}

constexpr COLORREF regex_error_color = RGB (255, 200, 200);

INT_PTR AdvancedSettingsTab::setup_regexp_error_color(HDC h_dc) const {
  SetBkColor(h_dc, regex_error_color);
  SetBkMode(h_dc, TRANSPARENT);
  return reinterpret_cast<INT_PTR> (m_regexp_error_color_brush.get ());
}

void AdvancedSettingsTab::draw_underline_color_button(LPDRAWITEMSTRUCT ds) {
  PAINTSTRUCT ps;
  HDC dc = ds->hDC;
  if ((ds->itemState & ODS_SELECTED) != 0u) {
    DrawEdge(dc, &ds->rcItem, BDR_SUNKENINNER | BDR_SUNKENOUTER, BF_RECT);
  } else {
    DrawEdge(dc, &ds->rcItem, BDR_RAISEDINNER | BDR_RAISEDOUTER, BF_RECT);
  }
  EndPaint(m_h_underline_color, &ps);
}

INT_PTR AdvancedSettingsTab::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) {
  switch (message) {
  case WM_INITDIALOG: {
    // Retrieving handles of dialog controls
    m_h_edit_delimiters = ::GetDlgItem(_hSelf, IDC_DELIMITERS);
    m_delimiter_exclusions_le = ::GetDlgItem(_hSelf, IDC_DELIMITER_EXCLUSIONS_LE);
    m_h_default_delimiters = ::GetDlgItem(_hSelf, IDC_DEFAULT_DELIMITERS);
    m_h_ignore_yo = ::GetDlgItem(_hSelf, IDC_COUNT_YO_AS_YE);
    m_h_convert_single_quotes = ::GetDlgItem(_hSelf, IDC_COUNT_SINGLE_QUOTES_AS_APOSTROPHE);
    m_h_remove_boundary_apostrophes = ::GetDlgItem(_hSelf, IDC_REMOVE_ENDING_APOSTROPHE);
    m_h_recheck_delay = ::GetDlgItem(_hSelf, IDC_RECHECK_DELAY);
    m_h_underline_color = ::GetDlgItem(_hSelf, IDC_UNDERLINE_COLOR);
    m_h_underline_style = ::GetDlgItem(_hSelf, IDC_UNDERLINE_STYLE);
    m_h_ignore_numbers = ::GetDlgItem(_hSelf, IDC_IGNORE_NUMBERS);
    m_h_ignore_c_start = ::GetDlgItem(_hSelf, IDC_IGNORE_CSTART);
    m_h_ignore_c_have = ::GetDlgItem(_hSelf, IDC_IGNORE_CHAVE);
    m_h_ignore_c_all = ::GetDlgItem(_hSelf, IDC_IGNORE_CALL);
    m_h_check_default_udl_style = ::GetDlgItem(_hSelf, IDC_CHECK_DEFAULT_UDL_STYLE);
    m_h_ignore_one_letter = ::GetDlgItem(_hSelf, IDC_IGNORE_ONE_LETTER);
    m_h_ignore_ = ::GetDlgItem(_hSelf, IDC_IGNORE_);
    m_h_ignore_se_apostrophe = ::GetDlgItem(_hSelf, IDC_IGNORE_SE_APOSTROPHE);
    m_h_slider_size = ::GetDlgItem(_hSelf, IDC_SLIDER_SIZE);
    m_h_slider_sugg_button_opacity = ::GetDlgItem(_hSelf, IDC_SLIDER_TRANSPARENCY);
    m_ignore_regexp_edit = ::GetDlgItem(_hSelf, IDC_IGNORE_REGEXP_EDIT);
    m_tokenization_style_cmb.init(::GetDlgItem(_hSelf, IDC_TOKENIZATION_STYLE_CMB));
    SendMessage(m_h_slider_size, TBM_SETRANGE, TRUE, MAKELPARAM(5, 22));
    SendMessage(m_h_slider_sugg_button_opacity, TBM_SETRANGE, TRUE, MAKELPARAM(5, 100));
    m_split_camel_case_cb = ::GetDlgItem(_hSelf, IDC_CAMEL_CASE_SPLITTING_CB);
    m_reset_btn = this->get_control<WinApi::Button>(IDC_RESET_SETTINGS);
    m_reset_btn->button_pressed.connect([this]() { reset_settings(); });

    SetWindowText(m_h_ignore_yo, rc_str(IDS_CYRILLIC_YO_AS_YE).c_str());
    WinApi::create_tooltip(IDC_DELIMITERS, _hSelf, rc_str(IDS_DELIMITER_TOOLTIP).c_str());
    WinApi::create_tooltip(IDC_RECHECK_DELAY, _hSelf, rc_str(IDS_RECHECK_DELAY_TOOLTIP).c_str());
    WinApi::create_tooltip(IDC_IGNORE_CSTART, _hSelf, rc_str(IDS_FIRST_CAPITAL_IGNORE_TOOLTIP).c_str());
    WinApi::create_tooltip(IDC_IGNORE_SE_APOSTROPHE, _hSelf, rc_str(IDS_IGNORE_BOUND_APOSTROPHE_TOOLTIP).c_str());
    WinApi::create_tooltip(IDC_REMOVE_ENDING_APOSTROPHE, _hSelf, rc_str(IDS_REMOVE_ENDING_APOSTROPHE_TOOLTIP).c_str());
    m_h_regexp_tooltip = WinApi::create_tooltip(IDC_IGNORE_REGEXP_EDIT, _hSelf, LPSTR_TEXTCALLBACKW);

    ComboBox_ResetContent(m_h_underline_style);
    for (const auto &name : get_indic_names())
      ComboBox_AddString(m_h_underline_style, name.c_str());
    return TRUE;
  }
  case WM_CLOSE: {
    EndDialog(_hSelf, 0);
    return FALSE;
  }
  case WM_DRAWITEM:
    switch (w_param) {
    case IDC_UNDERLINE_COLOR:
      auto ds = reinterpret_cast<LPDRAWITEMSTRUCT>(l_param);
      draw_underline_color_button(ds);
      return 1;
    }
    break;
  case WM_CTLCOLORBTN:
    if (m_h_underline_color == reinterpret_cast<HWND>(l_param)) {
      auto h_dc = reinterpret_cast<HDC>(w_param);
      return setup_underline_button_colors(h_dc);
    }
    break;
  case WM_CTLCOLOREDIT:
    if (m_ignore_regexp_edit == reinterpret_cast<HWND>(l_param)) {
      if (m_settings.get_regexp_error())
        {
          auto h_dc = reinterpret_cast<HDC>(w_param);
          return setup_regexp_error_color(h_dc);
        }

      break;
    }
    break;
  case WM_NOTIFY:
    {
      NMHDR *nm = reinterpret_cast<NMHDR *> (l_param);
      if (nm->hwndFrom == m_h_regexp_tooltip && nm->code == TTN_GETDISPINFOW) {
          auto mb_err = m_settings.get_regexp_error();
          if (!mb_err)
            return FALSE;
          static std::wstring regex_error;
          regex_error = to_wstring (mb_err->what());
          NMTTDISPINFOW *nmtdi = reinterpret_cast<NMTTDISPINFOW *> (l_param);
          nmtdi->lpszText = const_cast<LPWSTR>(regex_error.c_str());
      }
    } break;
  case WM_COMMAND:
    switch (LOWORD(w_param)) {
    case IDC_TOKENIZATION_STYLE_CMB: {
      setup_delimiter_line_edit_visiblity();
    } break;
    case IDC_DEFAULT_DELIMITERS:
      if (HIWORD(w_param) == BN_CLICKED) {
        on_default_delimiters_clicked();
        return TRUE;
      }
      break;
    case IDC_RECHECK_DELAY: {
      if (HIWORD(w_param) == EN_CHANGE) {
        on_recheck_delay_changed();
        return TRUE;
      }
    } break;
    case IDC_UNDERLINE_COLOR:
      if (HIWORD(w_param) == BN_CLICKED) {
        on_underline_color_button_clicked();
      }
      break;
    default:
      break;
    }
  default:
    break;
  }
  return FALSE;
}

void AdvancedSettingsTab::set_delimeters_edit(const wchar_t *delimiters) {
  Edit_SetText(m_h_edit_delimiters, delimiters);
}

void AdvancedSettingsTab::set_recheck_delay(int delay) {
  Edit_SetText(m_h_recheck_delay, std::to_wstring (delay).c_str ());
}

int AdvancedSettingsTab::get_recheck_delay() {
  auto text = WinApi::get_edit_text(m_h_recheck_delay);
  wchar_t *end_ptr;
  int x = wcstol(text.c_str(), &end_ptr, 10);
  return x;
}

AdvancedSettingsTab::AdvancedSettingsTab(const Settings &settings)
  : m_settings(settings) {
  m_regexp_error_color_brush = {CreateSolidBrush(regex_error_color)};
}

void AdvancedSettingsTab::apply_settings(Settings &settings) {
  settings.data.delimiters = WinApi::get_edit_text(m_h_edit_delimiters);
  settings.data.ignore_yo = Button_GetCheck(m_h_ignore_yo) == BST_CHECKED;
  settings.data.convert_single_quotes = Button_GetCheck(m_h_convert_single_quotes) == BST_CHECKED;
  settings.data.remove_boundary_apostrophes = Button_GetCheck(m_h_remove_boundary_apostrophes) == BST_CHECKED;
  settings.data.underline_color = m_underline_color_btn;
  settings.data.underline_style = ComboBox_GetCurSel(m_h_underline_style);
  settings.data.recheck_delay = get_recheck_delay();
  settings.data.check_default_udl_style = Button_GetCheck(m_h_check_default_udl_style) == BST_CHECKED;
  settings.data.ignore_containing_digit = Button_GetCheck(m_h_ignore_numbers) == BST_CHECKED;
  settings.data.ignore_starting_with_capital = Button_GetCheck(m_h_ignore_c_start) == BST_CHECKED;
  settings.data.ignore_having_a_capital = Button_GetCheck(m_h_ignore_c_have) == BST_CHECKED;
  settings.data.ignore_all_capital = Button_GetCheck(m_h_ignore_c_all) == BST_CHECKED;
  settings.data.ignore_having_underscore = Button_GetCheck(m_h_ignore_) == BST_CHECKED;
  settings.data.ignore_starting_or_ending_with_apostrophe = Button_GetCheck(m_h_ignore_se_apostrophe) == BST_CHECKED;
  settings.data.ignore_one_letter = Button_GetCheck(m_h_ignore_one_letter) == BST_CHECKED;
  settings.data.suggestion_button_size = static_cast<int>(SendMessage(m_h_slider_size, TBM_GETPOS, 0, 0));
  settings.data.suggestion_button_opacity = static_cast<int>(SendMessage(m_h_slider_sugg_button_opacity, TBM_GETPOS, 0, 0));
  settings.data.tokenization_style = m_tokenization_style_cmb.current_data();
  settings.data.delimiter_exclusions = WinApi::get_edit_text(m_delimiter_exclusions_le);
  settings.data.split_camel_case = Button_GetCheck(m_split_camel_case_cb) == BST_CHECKED;
  settings.data.ignore_regexp_str = WinApi::get_edit_text(m_ignore_regexp_edit);
}

SimpleSettingsTab *SettingsDialog::get_simple_dlg() { return &m_simple_dlg; }

AdvancedSettingsTab *SettingsDialog::get_advanced_dlg() { return &m_advanced_dlg; }

SettingsDialog::SettingsDialog(HINSTANCE h_inst, HWND parent, NppInterface &npp, const Settings &settings, const SpellerContainer &speller_container)
  : m_npp(npp), m_simple_dlg(*this, settings, m_npp), m_advanced_dlg(settings), m_settings(settings), m_speller_container(speller_container) {
  Window::init(h_inst, parent);
  m_settings.settings_changed.connect([this] { update_controls(); });
  m_speller_container.speller_status_changed.connect([this] {
    m_simple_dlg.update_language_controls(m_settings, m_speller_container, m_selected_language_name);
    m_simple_dlg.fill_lib_info(m_speller_container.get_aspell_status(), m_settings);
  });
}

void SettingsDialog::destroy() {
  m_simple_dlg.destroy();
  m_advanced_dlg.destroy();
};

// Send appropriate event and set some npp thread properties
void SettingsDialog::apply_settings() {
  auto mut_settings = m_settings.modify();
  m_simple_dlg.apply_settings(*mut_settings, m_speller_container);
  m_advanced_dlg.apply_settings(*mut_settings);
}

void SettingsDialog::update_controls() {
  m_simple_dlg.update_controls(m_settings, m_speller_container);
  m_advanced_dlg.update_controls(m_settings);
}

void SettingsDialog::apply_lib_change(SpellerId new_lib_id) {
  auto mut_settings = m_settings.modify();
  mut_settings->data.active_speller_lib_id = new_lib_id;
}

void SettingsDialog::store_selected_language_name(int language_index) {
  m_selected_language_name = language_index != -1
                               ? m_speller_container.active_speller().get_language_list()[language_index].orig_name
                               : multiple_language_alias;
}

void SimpleSettingsTab::init_settings(HINSTANCE h_inst, HWND parent) { return Window::init(h_inst, parent); }

void SimpleSettingsTab::update_controls(const Settings &settings, const SpellerContainer &speller_container) {
  m_speller_cmb.set_current(settings.data.active_speller_lib_id);
  fill_lib_info(speller_container.get_aspell_status(), settings);
  fill_sugestions_num(settings.data.suggestion_count);
  set_file_types(settings.data.check_those, settings.data.file_types.c_str());
  Button_SetCheck(m_h_check_comments, settings.data.check_comments ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_check_strings, settings.data.check_strings ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_check_varfunc, settings.data.check_variable_functions ? BST_CHECKED : BST_UNCHECKED);
  set_sugg_type(settings.data.suggestions_mode);
  set_one_user_dic(settings.data.use_unified_dictionary);
  m_language_name_style_cmb.set_current(settings.data.language_name_style);
  Button_SetCheck(m_h_select_misspelled_on_menu_cb, settings.data.select_word_on_context_menu_click);
}

void SimpleSettingsTab::init_speller_id_combobox(const SpellerContainer &speller_container) {
  m_speller_cmb.clear();
  for (auto id : enum_range<SpellerId>()) {
    if (id == SpellerId::native && !speller_container.native_speller().is_working())
      continue;
    m_speller_cmb.add_item(id);
  }
}

void SettingsDialog::update_rect_offset_due_to_buttons(RECT &rc) {
  auto ok_hwnd = GetDlgItem(_hSelf, IDOK);
  RECT ok_rect;
  RECT window_rect;
  GetWindowRect(ok_hwnd, &ok_rect);
  GetWindowRect(_hSelf, &window_rect);
  auto offset = window_rect.bottom - ok_rect.bottom;
  auto button_height = ok_rect.bottom - ok_rect.top;
  rc.bottom -= (2 * offset + button_height);
}

INT_PTR SettingsDialog::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) {
  switch (message) {
  case WM_INITDIALOG: {
    m_controls_tab.initTabBar(_hInst, _hSelf, false, true, false);
    m_controls_tab.setFont(reinterpret_cast<HFONT>(SendMessage(_hSelf, WM_GETFONT, 0, 0)));

    m_simple_dlg.init_settings(_hInst, _hSelf);
    m_simple_dlg.create(IDD_SIMPLE, false, false);
    m_simple_dlg.init_speller_id_combobox(m_speller_container);
    m_simple_dlg.display();
    m_advanced_dlg.init(_hInst, _hSelf);
    m_advanced_dlg.create(IDD_ADVANCED, false, false);

    m_window_vector.emplace_back(&m_simple_dlg, rc_str(IDS_SIMPLE_TAB_TEXT).c_str(), L"Simple Options");
    m_window_vector.emplace_back(&m_advanced_dlg, rc_str(IDS_ADVANCED_TAB_TEXT).c_str(), L"Advanced Options");
    m_controls_tab.createTabs(m_window_vector);
    m_controls_tab.display();
    RECT rc;
    getClientRect(rc);
    m_controls_tab.reSizeTo(rc);
    rc.bottom = rc.top + m_simple_dlg.getHeight();
    SetWindowPos(m_simple_dlg.getHSelf(), nullptr, rc.left, rc.top, 0, 0, SWP_NOSIZE);
    SetWindowPos(m_advanced_dlg.getHSelf(), nullptr, rc.left, rc.top, 0, 0, SWP_NOSIZE);

    // This stuff is copied from npp source to make tabbed window looked totally
    // nice and white
    auto enable_dlg_theme = reinterpret_cast<ETDTProc>(::SendMessage(_hParent, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0));
    if (enable_dlg_theme != nullptr)
      enable_dlg_theme(_hSelf, ETDT_ENABLETAB);

    update_controls();
    m_simple_dlg.update_language_controls(m_settings, m_speller_container, m_selected_language_name);
    return 1;
  }
  case WM_NOTIFY: {
    auto nmhdr = reinterpret_cast<NMHDR *>(l_param);
    switch (nmhdr->code) {
    case TCN_SELCHANGE:
      if (nmhdr->hwndFrom == m_controls_tab.getHSelf()) {
        m_controls_tab.clickedUpdate();
        return 0;
      }
      break;
    }
    break;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case 0: // Menu
    {
      copy_misspellings_to_clipboard();
    }
    break;
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

UINT SettingsDialog::do_dialog() {
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
