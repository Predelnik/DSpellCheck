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

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"
#include "SettingsDlg.h"

struct SuggestionsMenuItem;

const int nb_func = 10;

extern int quick_lang_change_item_index;
enum class CustomGuiMessage;
class LangList;
class NppInterface;
class DownloadDicsDlg;
class SelectProxyDialog;
class SpellChecker;
class ProgressDlg;
class RemoveDictionariesDialog;

//
// Initialization of your plugin data
// It will be called while plugin loading
//
void set_hmodule(HANDLE h_module_arg);

//
// Cleaning of your plugin
// It will be called while plugin unloading
//
void plugin_clean_up();

//
//Initialization of your plugin commands
//
void command_menu_init();

//
//Clean up your plugin commands allocation (if any)
//
void command_menu_clean_up();

//
// Function which sets your command
//
int set_next_command(const wchar_t* cmd_name, Pfuncplugincmd p_func,
                     std::unique_ptr<ShortcutKey> sk = nullptr, bool check0_n_init = false);

void set_delimiters (const char *str);
std::wstring rc_str (UINT string_id);
std::wstring_view rc_str_view (UINT string_id);
const char *get_delimiters ();
void set_encoding_by_id (int enc_id);
void recheck_visible ();
void init_classes ();
void create_hooks ();
LRESULT show_calculated_menu(std::vector<SuggestionsMenuItem>&& menu_list);
void add_icons ();
bool get_auto_check_state ();
void auto_check_state_received (bool state);
HMENU get_this_plugin_menu ();
HMENU get_langs_sub_menu ();
HANDLE get_h_module ();
LangList *get_lang_list ();
RemoveDictionariesDialog *get_remove_dics ();
SelectProxyDialog *get_select_proxy ();
ProgressDlg *get_progress ();
DownloadDicsDlg *get_download_dics ();
FuncItem *get_func_item ();
std::wstring get_default_hunspell_path ();
void set_context_menu_id_start (int id);
void set_langs_menu_id_start (int id);
void set_use_allocated_ids (bool value);
int get_context_menu_id_start ();
int get_langs_menu_id_start ();
bool get_use_allocated_ids ();
const Settings& get_settings();
DWORD get_custom_gui_message_id (CustomGuiMessage message_id);
void register_custom_messages ();
void init_npp_interface ();
void notify (SCNotification* notify_code);
NppInterface &npp_interface ();
void copy_misspellings_to_clipboard ();
