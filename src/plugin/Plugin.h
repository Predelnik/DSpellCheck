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

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"
#include "common/enum_array.h"

class MenuItem;
class AspellOptionsDialog;


enum class Action {
  toggle_auto_spell_check,
  quick_language_change,
  copy_all_misspellings,
  erase_all_misspellings,
  settings,
  show_spell_check_menu_at_cursor,
  reload_user_dictionaries,
  additional_actions,
  toggle_debug_logging,
  open_debug_log,
  replace_with_1st_suggestion,
  ignore_for_current_session,
  mark_lines_with_misspelling,
  online_manual,
  about,
  find_next_error,
  find_prev_error,

  COUNT,
};

const int nb_func = static_cast<int>(Action::COUNT);
constexpr int requested_menu_count = 350;

extern enum_array<Action, int> action_index;

enum class CustomGuiMessage;
class SelectMultipleLanguagesDialog;
class NppInterface;
class DownloadDictionariesDialog;
class ConnectionSettingsDialog;
class SpellChecker;
class ProgressDialog;
class RemoveDictionariesDialog;
class Settings;

void set_hmodule(HANDLE h_module_arg);
std::wstring rc_str(UINT string_id);
std::wstring_view rc_str_view(UINT string_id);
LRESULT show_calculated_menu(std::vector<MenuItem> &&menu_list);
HMENU get_this_plugin_menu();
HMENU get_langs_sub_menu();
HANDLE get_h_module();
SelectMultipleLanguagesDialog *get_lang_list();
RemoveDictionariesDialog *get_remove_dics();
AspellOptionsDialog *get_aspell_options_dlg();
ConnectionSettingsDialog *get_select_proxy();
ProgressDialog *get_progress_dlg();
FuncItem *get_func_item();
std::wstring get_default_hunspell_path();
int get_context_menu_id_start();
int get_langs_menu_id_start();
bool get_use_allocated_ids();
DWORD get_custom_gui_message_id(CustomGuiMessage message_id);
std::wstring get_debug_log_path();
NppInterface &npp_interface();
void copy_misspellings_to_clipboard();
void delete_log();
void print_to_log(std::wstring_view line, HWND parent_wnd);
