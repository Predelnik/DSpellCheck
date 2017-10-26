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

#pragma once

#include "StaticDialog/StaticDialog.h"
#include "PluginInterface.h"
#include "TabBar/ControlsTab.h"

class HunspellInterface;
class SpellChecker;
class LanguageInfo;
class Settings;
class SettingsDlg;
typedef HTHEME (WINAPI *OtdProc)(HWND, LPCWSTR);

class SimpleDlg : public StaticDialog {
public:
    SimpleDlg(SettingsDlg& parent);
    ~SimpleDlg();
    void apply_settings(Settings& settings);
    void update_language_controls(const Settings& settings
    );
    void fill_sugestions_num(int suggestions_num);
    void fill_lib_info(int status, const wchar_t* aspell_path, const wchar_t* hunspell_path,
                       const wchar_t* hunspell_additional_path);
    void disable_language_combo(bool disable);
    void set_file_types(bool check_those, const wchar_t* file_types);
    void set_sugg_type(int sugg_type);
    void set_check_comments(bool value);
    int get_selected_lib();
    void set_lib_mode(int lib_mode);
    void set_decode_names(bool value);
    void set_one_user_dic(bool value);
    void init_settings(HINSTANCE h_inst, HWND parent, NppData npp_data);
    void update_lib_status(const Settings& settings);
    void update_controls(const Settings& settings);

protected:

    INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) override;

private:
    /* NppData struct instance */
    NppData m_npp_data_instance;
    SettingsDlg& m_parent;

    /* handles of controls */
    COLORREF m_aspell_status_color = NULL;
    HBRUSH m_default_brush = nullptr;
    HWND m_h_aspell_status = nullptr;
    HWND m_h_lib_path = nullptr;
    HWND m_h_suggestions_num = nullptr;
    HWND m_h_combo_language = nullptr;
    HWND m_h_check_not_those = nullptr;
    HWND m_h_check_only_those = nullptr;
    HWND m_h_file_types = nullptr;
    HWND m_h_check_comments = nullptr;
    HWND m_h_lib_link = nullptr;
    HWND m_h_sugg_type = nullptr;
    HWND m_h_library = nullptr;
    HWND m_h_lib_group_box = nullptr;
    HWND m_h_download_dics = nullptr;
    HWND m_h_remove_dics = nullptr;
    HWND m_h_decode_names = nullptr;
    HWND m_h_one_user_dic = nullptr;
    HWND m_h_hunspell_path_group_box = nullptr;
    HWND m_h_reset_speller_path = nullptr;
    HWND m_h_hunspell_path_type = nullptr;
    HWND m_h_system_path = nullptr;
    HWND m_h_aspell_run_together_cb = nullptr;

    HMODULE m_h_ux_theme;
    OtdProc m_open_theme_data;
};

class AdvancedDlg : public StaticDialog {
public:
    void apply_settings(Settings& settings);
    void fill_delimiters(const char* delimiters);
    void set_delimeters_edit(const wchar_t* delimiters);
    void set_conversion_opts(bool convert_yo, bool convert_single_quotes_arg,
                             bool remove_single_apostrophe);
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
    INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) override;

private:
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
    HWND m_h_buffer_size = nullptr;
    HWND m_h_remove_boundary_apostrophes = nullptr;

    HBRUSH m_brush = nullptr;
    COLORREF m_underline_color_btn = 0;
};

class SettingsDlg : public StaticDialog {
public:
    UINT do_dialog();
    SimpleDlg* get_simple_dlg();
    AdvancedDlg* get_advanced_dlg();
    SettingsDlg(HINSTANCE h_inst, HWND parent, NppData npp_data, const Settings& settings);

private:
    INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) override;
    void destroy() override;
    void apply_settings();
    void update_controls();
    void apply_lib_change (int new_lib_id);

private:
    NppData m_npp_data;
    SimpleDlg m_simple_dlg;
    AdvancedDlg m_advanced_dlg;
    WindowVector m_window_vector;
    ControlsTab m_controls_tab;
    const Settings& m_settings;

    friend class SimpleDlg;
};
