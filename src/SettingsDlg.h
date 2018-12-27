// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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

#include "StaticDialog/StaticDialog.h"
#include "TabBar/ControlsTab.h"
#include "Settings.h"
#include "utils/WinApiControls.h"

enum class AspellStatus;
class HunspellInterface;
class SpellChecker;
class NppInterface;
class LanguageInfo;
class Settings;
class SettingsDlg;
class NppInteface;
class SpellerContainer;
using OtdProc = HTHEME(WINAPI *)(HWND, LPCWSTR);

class SimpleDlg : public StaticDialog {
public:
    SimpleDlg(SettingsDlg& parent, const Settings &settings, NppInterface &npp);
    ~SimpleDlg() override;
    void apply_settings(Settings& settings, const SpellerContainer& speller_container);
    void update_language_controls(const Settings& settings, const SpellerContainer& speller_container, const std::wstring& selected_language_name
    );
    void fill_sugestions_num(int suggestions_num);
    void fill_lib_info(AspellStatus aspell_status, const Settings& settings);
    void disable_language_combo(bool disable);
    void set_file_types(bool check_those, const wchar_t* file_types);
    void set_sugg_type(SuggestionMode mode);
    void set_one_user_dic(bool value);
    void init_settings(HINSTANCE h_inst, HWND parent);
    void update_controls(const Settings& settings, const SpellerContainer& speller_container);
    void init_speller_id_combobox(const SpellerContainer& speller_container);

protected:

    INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) override;

private:
    /* NppData struct instance */
    NppInterface &m_npp;
    const Settings &m_settings;
    SettingsDlg& m_parent;

    /* handles of controls */
    COLORREF m_aspell_status_color = NULL;
    HBRUSH m_default_brush = nullptr;
    HWND m_h_aspell_status = nullptr;
    HWND m_h_lib_path = nullptr;
    HWND m_h_suggestions_num = nullptr;
    HWND m_h_check_not_those = nullptr;
    HWND m_h_check_only_those = nullptr;
    HWND m_h_file_types = nullptr;
    HWND m_h_check_comments = nullptr;
    HWND m_h_check_strings  = nullptr;
    HWND m_h_check_varfunc = nullptr;
    HWND m_h_lib_link = nullptr;
    HWND m_h_lib_group_box = nullptr;
    HWND m_h_download_dics = nullptr;
    HWND m_h_remove_dics = nullptr;
    HWND m_h_one_user_dic = nullptr;
    HWND m_h_hunspell_path_group_box = nullptr;
    HWND m_h_reset_speller_path = nullptr;
    HWND m_h_hunspell_path_type = nullptr;
    HWND m_h_system_path = nullptr;
    HWND m_configure_aspell_btn = nullptr;
    HWND m_browse_btn = nullptr;
    WinApi::EnumComboBox<SpellerId> m_speller_cmb;
    WinApi::ComboBox m_language_cmb;
    WinApi::EnumComboBox<SuggestionMode> m_suggestion_mode_cmb;
    WinApi::EnumComboBox<LanguageNameStyle> m_language_name_style_cmb;

    HMODULE m_h_ux_theme;
    OtdProc m_open_theme_data;
};

class AdvancedDlg : public StaticDialog {
public:
    explicit AdvancedDlg (const Settings &settings);
    void apply_settings(Settings& settings);
    void set_delimeters_edit(const wchar_t* delimiters);
    void set_conversion_opts(bool convert_yo, bool convert_single_quotes_arg,
                             bool remove_boundary_apostrophes);
    int get_recheck_delay();
    void set_recheck_delay(int delay);
    void set_sugg_box_settings(LRESULT size, LRESULT trans);
    void set_underline_settings(int color, int style);
    void set_ignore(bool ignore_numbers_arg, bool ignore_c_start_arg,
                    bool ignore_c_have_arg, bool ignore_c_all_arg, bool ignore_arg,
                    bool ignore_sa_apostrophe_arg, bool ignore_one_letter);
    void set_buffer_size(int size);
    void update_controls(const Settings& settings);

protected:
    void setup_delimiter_line_edit_visiblity();
    INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) override;

private:
    const Settings &m_settings;
    HWND m_h_edit_delimiters = nullptr;
    HWND m_h_default_delimiters = nullptr;
    HWND m_h_ignore_yo = nullptr;
    HWND m_h_convert_single_quotes = nullptr;
    HWND m_h_recheck_delay = nullptr;
    HWND m_h_underline_color = nullptr;
    HWND m_h_underline_style = nullptr;
    HWND m_h_ignore_numbers = nullptr;
    HWND m_h_ignore_c_start = nullptr;
    HWND m_h_ignore_c_have = nullptr;
    HWND m_h_ignore_c_all = nullptr;
    HWND m_h_ignore_one_letter = nullptr;
    HWND m_h_ignore_ = nullptr;
    HWND m_h_ignore_se_apostrophe = nullptr;
    HWND m_h_slider_size = nullptr;
    HWND m_h_slider_sugg_button_opacity = nullptr;
    HWND m_h_remove_boundary_apostrophes = nullptr;
    HWND m_delimiter_exclusions_le = nullptr;
    HWND m_split_camel_case_cb = nullptr;
    HWND m_h_check_default_udl_style = nullptr;
    WinApi::EnumComboBox<TokenizationStyle> m_tokenization_style_cmb;

    HBRUSH m_brush = nullptr;
    COLORREF m_underline_color_btn = 0;
};

class SettingsDlg : public StaticDialog {
public:
    UINT do_dialog();
    SimpleDlg* get_simple_dlg();
    AdvancedDlg* get_advanced_dlg();
    SettingsDlg(HINSTANCE h_inst, HWND parent, NppInterface &npp, const Settings& settings, const SpellerContainer &speller_container);

    lsignal::signal<void ()> download_dics_dlg_requested;

private:
    INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) override;
    void destroy() override;
    void apply_settings();
    void update_controls();
    void apply_lib_change (SpellerId new_lib_id);
    void store_selected_language_name (int language_index);

private:
    NppInterface &m_npp;
    SimpleDlg m_simple_dlg;
    AdvancedDlg m_advanced_dlg;
    WindowVector m_window_vector;
    ControlsTab m_controls_tab;
    const Settings& m_settings;
    const SpellerContainer &m_speller_container;
    std::wstring m_selected_language_name;

    friend class SimpleDlg;
};
