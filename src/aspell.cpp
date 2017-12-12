/*
This file is part of Notepad++ - interface defines

Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
2013 Sergey Semushin <Predelnik@gmail.com>

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

#include "PluginInterface.h"
#include "aspell.h"
#include <iostream>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>

#include "CommonFunctions.h"

HANDLE g_h_module;

HINSTANCE h_inst_lib = nullptr;

PFUNC_aspell_mutable_container_add aspell_mutable_container_add = nullptr;
PFUNC_aspell_mutable_container_remove aspell_mutable_container_remove = nullptr;
PFUNC_aspell_mutable_container_clear aspell_mutable_container_clear = nullptr;
PFUNC_aspell_mutable_container_to_mutable_container
    aspell_mutable_container_to_mutable_container = nullptr;
PFUNC_aspell_key_info_enumeration_at_end aspell_key_info_enumeration_at_end =
    nullptr;
PFUNC_aspell_key_info_enumeration_next aspell_key_info_enumeration_next = nullptr;
PFUNC_delete_aspell_key_info_enumeration delete_aspell_key_info_enumeration =
    nullptr;
PFUNC_aspell_key_info_enumeration_clone aspell_key_info_enumeration_clone =
    nullptr;
PFUNC_aspell_key_info_enumeration_assign aspell_key_info_enumeration_assign =
    nullptr;
PFUNC_new_aspell_config new_aspell_config = nullptr;
PFUNC_delete_aspell_config delete_aspell_config = nullptr;
PFUNC_aspell_config_clone aspell_config_clone = nullptr;
PFUNC_aspell_config_assign aspell_config_assign = nullptr;
PFUNC_aspell_config_error_number aspell_config_error_number = nullptr;
PFUNC_aspell_config_error_message aspell_config_error_message = nullptr;
PFUNC_aspell_config_error aspell_config_error = nullptr;
PFUNC_aspell_config_set_extra aspell_config_set_extra = nullptr;
PFUNC_aspell_config_keyinfo aspell_config_keyinfo = nullptr;
PFUNC_aspell_config_possible_elements aspell_config_possible_elements = nullptr;
PFUNC_aspell_config_get_default aspell_config_get_default = nullptr;
PFUNC_aspell_config_elements aspell_config_elements = nullptr;
PFUNC_aspell_config_replace aspell_config_replace = nullptr;
PFUNC_aspell_config_remove aspell_config_remove = nullptr;
PFUNC_aspell_config_have aspell_config_have = nullptr;
PFUNC_aspell_config_retrieve aspell_config_retrieve = nullptr;
PFUNC_aspell_config_retrieve_list aspell_config_retrieve_list = nullptr;
PFUNC_aspell_config_retrieve_bool aspell_config_retrieve_bool = nullptr;
PFUNC_aspell_config_retrieve_int aspell_config_retrieve_int = nullptr;
PFUNC_aspell_error_number aspell_error_number = nullptr;
PFUNC_aspell_error_message aspell_error_message = nullptr;
PFUNC_aspell_error aspell_error = nullptr;
PFUNC_delete_aspell_can_have_error delete_aspell_can_have_error = nullptr;
PFUNC_new_aspell_speller new_aspell_speller = nullptr;
PFUNC_to_aspell_speller to_aspell_speller = nullptr;
PFUNC_delete_aspell_speller delete_aspell_speller = nullptr;
PFUNC_aspell_speller_error_number aspell_speller_error_number = nullptr;
PFUNC_aspell_speller_error_message aspell_speller_error_message = nullptr;
PFUNC_aspell_speller_error aspell_speller_error = nullptr;
PFUNC_aspell_speller_config aspell_speller_config = nullptr;
PFUNC_aspell_speller_check aspell_speller_check = nullptr;
PFUNC_aspell_speller_add_to_personal aspell_speller_add_to_personal = nullptr;
PFUNC_aspell_speller_add_to_session aspell_speller_add_to_session = nullptr;
PFUNC_aspell_speller_personal_word_list aspell_speller_personal_word_list =
    nullptr;
PFUNC_aspell_speller_session_word_list aspell_speller_session_word_list = nullptr;
PFUNC_aspell_speller_main_word_list aspell_speller_main_word_list = nullptr;
PFUNC_aspell_speller_save_all_word_lists aspell_speller_save_all_word_lists =
    nullptr;
PFUNC_aspell_speller_clear_session aspell_speller_clear_session = nullptr;
PFUNC_aspell_speller_suggest aspell_speller_suggest = nullptr;
PFUNC_aspell_speller_store_replacement aspell_speller_store_replacement = nullptr;
PFUNC_delete_aspell_filter delete_aspell_filter = nullptr;
PFUNC_aspell_filter_error_number aspell_filter_error_number = nullptr;
PFUNC_aspell_filter_error_message aspell_filter_error_message = nullptr;
PFUNC_aspell_filter_error aspell_filter_error = nullptr;
PFUNC_to_aspell_filter to_aspell_filter = nullptr;
PFUNC_delete_aspell_document_checker delete_aspell_document_checker = nullptr;
PFUNC_aspell_document_checker_error_number
    aspell_document_checker_error_number = nullptr;
PFUNC_aspell_document_checker_error_message
    aspell_document_checker_error_message = nullptr;
PFUNC_aspell_document_checker_error aspell_document_checker_error = nullptr;
PFUNC_new_aspell_document_checker new_aspell_document_checker = nullptr;
PFUNC_to_aspell_document_checker to_aspell_document_checker = nullptr;
PFUNC_aspell_document_checker_reset aspell_document_checker_reset = nullptr;
PFUNC_aspell_document_checker_process aspell_document_checker_process = nullptr;
PFUNC_aspell_document_checker_next_misspelling
    aspell_document_checker_next_misspelling = nullptr;
PFUNC_aspell_document_checker_filter aspell_document_checker_filter = nullptr;
PFUNC_aspell_word_list_empty aspell_word_list_empty = nullptr;
PFUNC_aspell_word_list_size aspell_word_list_size = nullptr;
PFUNC_aspell_word_list_elements aspell_word_list_elements = nullptr;
PFUNC_delete_aspell_string_enumeration delete_aspell_string_enumeration = nullptr;
PFUNC_aspell_string_enumeration_clone aspell_string_enumeration_clone = nullptr;
PFUNC_aspell_string_enumeration_assign aspell_string_enumeration_assign = nullptr;
PFUNC_aspell_string_enumeration_at_end aspell_string_enumeration_at_end = nullptr;
PFUNC_aspell_string_enumeration_next aspell_string_enumeration_next = nullptr;
PFUNC_get_aspell_module_info_list get_aspell_module_info_list = nullptr;
PFUNC_aspell_module_info_list_empty aspell_module_info_list_empty = nullptr;
PFUNC_aspell_module_info_list_size aspell_module_info_list_size = nullptr;
PFUNC_aspell_module_info_list_elements aspell_module_info_list_elements = nullptr;
PFUNC_get_aspell_dict_info_list get_aspell_dict_info_list = nullptr;
PFUNC_aspell_dict_info_list_empty aspell_dict_info_list_empty = nullptr;
PFUNC_aspell_dict_info_list_size aspell_dict_info_list_size = nullptr;
PFUNC_aspell_dict_info_list_elements aspell_dict_info_list_elements = nullptr;
PFUNC_aspell_module_info_enumeration_at_end
    aspell_module_info_enumeration_at_end = nullptr;
PFUNC_aspell_module_info_enumeration_next aspell_module_info_enumeration_next =
    nullptr;
PFUNC_delete_aspell_module_info_enumeration
    delete_aspell_module_info_enumeration = nullptr;
PFUNC_aspell_module_info_enumeration_clone
    aspell_module_info_enumeration_clone = nullptr;
PFUNC_aspell_module_info_enumeration_assign
    aspell_module_info_enumeration_assign = nullptr;
PFUNC_aspell_dict_info_enumeration_at_end aspell_dict_info_enumeration_at_end =
    nullptr;
PFUNC_aspell_dict_info_enumeration_next aspell_dict_info_enumeration_next =
    nullptr;
PFUNC_delete_aspell_dict_info_enumeration delete_aspell_dict_info_enumeration =
    nullptr;
PFUNC_aspell_dict_info_enumeration_clone aspell_dict_info_enumeration_clone =
    nullptr;
PFUNC_aspell_dict_info_enumeration_assign aspell_dict_info_enumeration_assign =
    nullptr;
PFUNC_new_aspell_string_list new_aspell_string_list = nullptr;
PFUNC_aspell_string_list_empty aspell_string_list_empty = nullptr;
PFUNC_aspell_string_list_size aspell_string_list_size = nullptr;
PFUNC_aspell_string_list_elements aspell_string_list_elements = nullptr;
PFUNC_aspell_string_list_add aspell_string_list_add = nullptr;
PFUNC_aspell_string_list_remove aspell_string_list_remove = nullptr;
PFUNC_aspell_string_list_clear aspell_string_list_clear = nullptr;
PFUNC_aspell_string_list_to_mutable_container
    aspell_string_list_to_mutable_container = nullptr;
PFUNC_delete_aspell_string_list delete_aspell_string_list = nullptr;
PFUNC_aspell_string_list_clone aspell_string_list_clone = nullptr;
PFUNC_aspell_string_list_assign aspell_string_list_assign = nullptr;
PFUNC_new_aspell_string_map new_aspell_string_map = nullptr;
PFUNC_aspell_string_map_add aspell_string_map_add = nullptr;
PFUNC_aspell_string_map_remove aspell_string_map_remove = nullptr;
PFUNC_aspell_string_map_clear aspell_string_map_clear = nullptr;
PFUNC_aspell_string_map_to_mutable_container
    aspell_string_map_to_mutable_container = nullptr;
PFUNC_delete_aspell_string_map delete_aspell_string_map = nullptr;
PFUNC_aspell_string_map_clone aspell_string_map_clone = nullptr;
PFUNC_aspell_string_map_assign aspell_string_map_assign = nullptr;
PFUNC_aspell_string_map_empty aspell_string_map_empty = nullptr;
PFUNC_aspell_string_map_size aspell_string_map_size = nullptr;
PFUNC_aspell_string_map_elements aspell_string_map_elements = nullptr;
PFUNC_aspell_string_map_insert aspell_string_map_insert = nullptr;
PFUNC_aspell_string_map_replace aspell_string_map_replace = nullptr;
PFUNC_aspell_string_map_lookup aspell_string_map_lookup = nullptr;
PFUNC_aspell_string_pair_enumeration_at_end
    aspell_string_pair_enumeration_at_end = nullptr;
PFUNC_aspell_string_pair_enumeration_next aspell_string_pair_enumeration_next =
    nullptr;
PFUNC_delete_aspell_string_pair_enumeration
    delete_aspell_string_pair_enumeration = nullptr;
PFUNC_aspell_string_pair_enumeration_clone
    aspell_string_pair_enumeration_clone = nullptr;
PFUNC_aspell_string_pair_enumeration_assign
    aspell_string_pair_enumeration_assign = nullptr;

std::wstring get_default_aspell_path() {
  wchar_t psz_path[MAX_PATH];
  psz_path[0] = '\0';
  HKEY h_key = nullptr;
  DWORD size = MAX_PATH;

  if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Aspell",
                                      0, KEY_READ, &h_key)) {
    if (ERROR_SUCCESS ==
        ::RegQueryValueEx(h_key, L"Path", nullptr, nullptr, (LPBYTE)psz_path, &size))
      wcscat(psz_path, L"\\aspell-15.dll");
    ::RegCloseKey(h_key);
  } else {
    wchar_t pf[MAX_PATH];
    SHGetSpecialFolderPath(nullptr, pf, CSIDL_PROGRAM_FILES, FALSE);
    PathAppend(psz_path, pf);
    PathAppend(psz_path, L"\\Aspell\\bin\\aspell-15.dll");
  }
  return psz_path;
}

std::wstring get_actual_aspell_path(const std::wstring& supposed_path) {
  if (supposed_path.empty ()) {
    return get_default_aspell_path();
  } else {
    return supposed_path;
  }
}

bool load_aspell(const wchar_t* path_arg) {
  auto path = get_actual_aspell_path(path_arg);
  h_inst_lib = LoadLibrary(path.c_str ());
  bool b_ret = false;

  if (h_inst_lib != nullptr) {
    aspell_mutable_container_add =
        (PFUNC_aspell_mutable_container_add)GetProcAddress(
            h_inst_lib, "aspell_mutable_container_add");
    aspell_mutable_container_remove =
        (PFUNC_aspell_mutable_container_remove)GetProcAddress(
            h_inst_lib, "aspell_mutable_container_remove");
    aspell_mutable_container_clear =
        (PFUNC_aspell_mutable_container_clear)GetProcAddress(
            h_inst_lib, "aspell_mutable_container_clear");
    aspell_mutable_container_to_mutable_container =
        (PFUNC_aspell_mutable_container_to_mutable_container)GetProcAddress(
            h_inst_lib, "aspell_mutable_container_to_mutable_container");
    aspell_key_info_enumeration_at_end =
        (PFUNC_aspell_key_info_enumeration_at_end)GetProcAddress(
            h_inst_lib, "aspell_key_info_enumeration_at_end");
    aspell_key_info_enumeration_next =
        (PFUNC_aspell_key_info_enumeration_next)GetProcAddress(
            h_inst_lib, "aspell_key_info_enumeration_next");
    delete_aspell_key_info_enumeration =
        (PFUNC_delete_aspell_key_info_enumeration)GetProcAddress(
            h_inst_lib, "delete_aspell_key_info_enumeration");
    aspell_key_info_enumeration_clone =
        (PFUNC_aspell_key_info_enumeration_clone)GetProcAddress(
            h_inst_lib, "aspell_key_info_enumeration_clone");
    aspell_key_info_enumeration_assign =
        (PFUNC_aspell_key_info_enumeration_assign)GetProcAddress(
            h_inst_lib, "aspell_key_info_enumeration_assign");
    new_aspell_config =
        (PFUNC_new_aspell_config)GetProcAddress(h_inst_lib, "new_aspell_config");
    delete_aspell_config = (PFUNC_delete_aspell_config)GetProcAddress(
        h_inst_lib, "delete_aspell_config");
    aspell_config_clone = (PFUNC_aspell_config_clone)GetProcAddress(
        h_inst_lib, "aspell_config_clone");
    aspell_config_assign = (PFUNC_aspell_config_assign)GetProcAddress(
        h_inst_lib, "aspell_config_assign");
    aspell_config_error_number =
        (PFUNC_aspell_config_error_number)GetProcAddress(
            h_inst_lib, "aspell_config_error_number");
    aspell_config_error_message =
        (PFUNC_aspell_config_error_message)GetProcAddress(
            h_inst_lib, "aspell_config_error_message");
    aspell_config_error = (PFUNC_aspell_config_error)GetProcAddress(
        h_inst_lib, "aspell_config_error");
    aspell_config_set_extra = (PFUNC_aspell_config_set_extra)GetProcAddress(
        h_inst_lib, "aspell_config_set_extra");
    aspell_config_keyinfo = (PFUNC_aspell_config_keyinfo)GetProcAddress(
        h_inst_lib, "aspell_config_keyinfo");
    aspell_config_possible_elements =
        (PFUNC_aspell_config_possible_elements)GetProcAddress(
            h_inst_lib, "aspell_config_possible_elements");
    aspell_config_get_default = (PFUNC_aspell_config_get_default)GetProcAddress(
        h_inst_lib, "aspell_config_get_default");
    aspell_config_elements = (PFUNC_aspell_config_elements)GetProcAddress(
        h_inst_lib, "aspell_config_elements");
    aspell_config_replace = (PFUNC_aspell_config_replace)GetProcAddress(
        h_inst_lib, "aspell_config_replace");
    aspell_config_remove = (PFUNC_aspell_config_remove)GetProcAddress(
        h_inst_lib, "aspell_config_remove");
    aspell_config_have = (PFUNC_aspell_config_have)GetProcAddress(
        h_inst_lib, "aspell_config_have");
    aspell_config_retrieve = (PFUNC_aspell_config_retrieve)GetProcAddress(
        h_inst_lib, "aspell_config_retrieve");
    aspell_config_retrieve_list =
        (PFUNC_aspell_config_retrieve_list)GetProcAddress(
            h_inst_lib, "aspell_config_retrieve_list");
    aspell_config_retrieve_bool =
        (PFUNC_aspell_config_retrieve_bool)GetProcAddress(
            h_inst_lib, "aspell_config_retrieve_bool");
    aspell_config_retrieve_int =
        (PFUNC_aspell_config_retrieve_int)GetProcAddress(
            h_inst_lib, "aspell_config_retrieve_int");
    aspell_error_number = (PFUNC_aspell_error_number)GetProcAddress(
        h_inst_lib, "aspell_error_number");
    aspell_error_message = (PFUNC_aspell_error_message)GetProcAddress(
        h_inst_lib, "aspell_error_message");
    aspell_error = (PFUNC_aspell_error)GetProcAddress(h_inst_lib, "aspell_error");
    delete_aspell_can_have_error =
        (PFUNC_delete_aspell_can_have_error)GetProcAddress(
            h_inst_lib, "delete_aspell_can_have_error");
    new_aspell_speller = (PFUNC_new_aspell_speller)GetProcAddress(
        h_inst_lib, "new_aspell_speller");
    to_aspell_speller =
        (PFUNC_to_aspell_speller)GetProcAddress(h_inst_lib, "to_aspell_speller");
    delete_aspell_speller = (PFUNC_delete_aspell_speller)GetProcAddress(
        h_inst_lib, "delete_aspell_speller");
    aspell_speller_error_number =
        (PFUNC_aspell_speller_error_number)GetProcAddress(
            h_inst_lib, "aspell_speller_error_number");
    aspell_speller_error_message =
        (PFUNC_aspell_speller_error_message)GetProcAddress(
            h_inst_lib, "aspell_speller_error_message");
    aspell_speller_error = (PFUNC_aspell_speller_error)GetProcAddress(
        h_inst_lib, "aspell_speller_error");
    aspell_speller_config = (PFUNC_aspell_speller_config)GetProcAddress(
        h_inst_lib, "aspell_speller_config");
    aspell_speller_check = (PFUNC_aspell_speller_check)GetProcAddress(
        h_inst_lib, "aspell_speller_check");
    aspell_speller_add_to_personal =
        (PFUNC_aspell_speller_add_to_personal)GetProcAddress(
            h_inst_lib, "aspell_speller_add_to_personal");
    aspell_speller_add_to_session =
        (PFUNC_aspell_speller_add_to_session)GetProcAddress(
            h_inst_lib, "aspell_speller_add_to_session");
    aspell_speller_personal_word_list =
        (PFUNC_aspell_speller_personal_word_list)GetProcAddress(
            h_inst_lib, "aspell_speller_personal_word_list");
    aspell_speller_session_word_list =
        (PFUNC_aspell_speller_session_word_list)GetProcAddress(
            h_inst_lib, "aspell_speller_session_word_list");
    aspell_speller_main_word_list =
        (PFUNC_aspell_speller_main_word_list)GetProcAddress(
            h_inst_lib, "aspell_speller_main_word_list");
    aspell_speller_save_all_word_lists =
        (PFUNC_aspell_speller_save_all_word_lists)GetProcAddress(
            h_inst_lib, "aspell_speller_save_all_word_lists");
    aspell_speller_clear_session =
        (PFUNC_aspell_speller_clear_session)GetProcAddress(
            h_inst_lib, "aspell_speller_clear_session");
    aspell_speller_suggest = (PFUNC_aspell_speller_suggest)GetProcAddress(
        h_inst_lib, "aspell_speller_suggest");
    aspell_speller_store_replacement =
        (PFUNC_aspell_speller_store_replacement)GetProcAddress(
            h_inst_lib, "aspell_speller_store_replacement");
    delete_aspell_filter = (PFUNC_delete_aspell_filter)GetProcAddress(
        h_inst_lib, "delete_aspell_filter");
    aspell_filter_error_number =
        (PFUNC_aspell_filter_error_number)GetProcAddress(
            h_inst_lib, "aspell_filter_error_number");
    aspell_filter_error_message =
        (PFUNC_aspell_filter_error_message)GetProcAddress(
            h_inst_lib, "aspell_filter_error_message");
    aspell_filter_error = (PFUNC_aspell_filter_error)GetProcAddress(
        h_inst_lib, "aspell_filter_error");
    to_aspell_filter =
        (PFUNC_to_aspell_filter)GetProcAddress(h_inst_lib, "to_aspell_filter");
    delete_aspell_document_checker =
        (PFUNC_delete_aspell_document_checker)GetProcAddress(
            h_inst_lib, "delete_aspell_document_checker");
    aspell_document_checker_error_number =
        (PFUNC_aspell_document_checker_error_number)GetProcAddress(
            h_inst_lib, "aspell_document_checker_error_number");
    aspell_document_checker_error_message =
        (PFUNC_aspell_document_checker_error_message)GetProcAddress(
            h_inst_lib, "aspell_document_checker_error_message");
    aspell_document_checker_error =
        (PFUNC_aspell_document_checker_error)GetProcAddress(
            h_inst_lib, "aspell_document_checker_error");
    new_aspell_document_checker =
        (PFUNC_new_aspell_document_checker)GetProcAddress(
            h_inst_lib, "new_aspell_document_checker");
    to_aspell_document_checker =
        (PFUNC_to_aspell_document_checker)GetProcAddress(
            h_inst_lib, "to_aspell_document_checker");
    aspell_document_checker_reset =
        (PFUNC_aspell_document_checker_reset)GetProcAddress(
            h_inst_lib, "aspell_document_checker_reset");
    aspell_document_checker_process =
        (PFUNC_aspell_document_checker_process)GetProcAddress(
            h_inst_lib, "aspell_document_checker_process");
    aspell_document_checker_next_misspelling =
        (PFUNC_aspell_document_checker_next_misspelling)GetProcAddress(
            h_inst_lib, "aspell_document_checker_next_misspelling");
    aspell_document_checker_filter =
        (PFUNC_aspell_document_checker_filter)GetProcAddress(
            h_inst_lib, "aspell_document_checker_filter");
    aspell_word_list_empty = (PFUNC_aspell_word_list_empty)GetProcAddress(
        h_inst_lib, "aspell_word_list_empty");
    aspell_word_list_size = (PFUNC_aspell_word_list_size)GetProcAddress(
        h_inst_lib, "aspell_word_list_size");
    aspell_word_list_elements = (PFUNC_aspell_word_list_elements)GetProcAddress(
        h_inst_lib, "aspell_word_list_elements");
    delete_aspell_string_enumeration =
        (PFUNC_delete_aspell_string_enumeration)GetProcAddress(
            h_inst_lib, "delete_aspell_string_enumeration");
    aspell_string_enumeration_clone =
        (PFUNC_aspell_string_enumeration_clone)GetProcAddress(
            h_inst_lib, "aspell_string_enumeration_clone");
    aspell_string_enumeration_assign =
        (PFUNC_aspell_string_enumeration_assign)GetProcAddress(
            h_inst_lib, "aspell_string_enumeration_assign");
    aspell_string_enumeration_at_end =
        (PFUNC_aspell_string_enumeration_at_end)GetProcAddress(
            h_inst_lib, "aspell_string_enumeration_at_end");
    aspell_string_enumeration_next =
        (PFUNC_aspell_string_enumeration_next)GetProcAddress(
            h_inst_lib, "aspell_string_enumeration_next");
    get_aspell_module_info_list =
        (PFUNC_get_aspell_module_info_list)GetProcAddress(
            h_inst_lib, "get_aspell_module_info_list");
    aspell_module_info_list_empty =
        (PFUNC_aspell_module_info_list_empty)GetProcAddress(
            h_inst_lib, "aspell_module_info_list_empty");
    aspell_module_info_list_size =
        (PFUNC_aspell_module_info_list_size)GetProcAddress(
            h_inst_lib, "aspell_module_info_list_size");
    aspell_module_info_list_elements =
        (PFUNC_aspell_module_info_list_elements)GetProcAddress(
            h_inst_lib, "aspell_module_info_list_elements");
    get_aspell_dict_info_list = (PFUNC_get_aspell_dict_info_list)GetProcAddress(
        h_inst_lib, "get_aspell_dict_info_list");
    aspell_dict_info_list_empty =
        (PFUNC_aspell_dict_info_list_empty)GetProcAddress(
            h_inst_lib, "aspell_dict_info_list_empty");
    aspell_dict_info_list_size =
        (PFUNC_aspell_dict_info_list_size)GetProcAddress(
            h_inst_lib, "aspell_dict_info_list_size");
    aspell_dict_info_list_elements =
        (PFUNC_aspell_dict_info_list_elements)GetProcAddress(
            h_inst_lib, "aspell_dict_info_list_elements");
    aspell_module_info_enumeration_at_end =
        (PFUNC_aspell_module_info_enumeration_at_end)GetProcAddress(
            h_inst_lib, "aspell_module_info_enumeration_at_end");
    aspell_module_info_enumeration_next =
        (PFUNC_aspell_module_info_enumeration_next)GetProcAddress(
            h_inst_lib, "aspell_module_info_enumeration_next");
    delete_aspell_module_info_enumeration =
        (PFUNC_delete_aspell_module_info_enumeration)GetProcAddress(
            h_inst_lib, "delete_aspell_module_info_enumeration");
    aspell_module_info_enumeration_clone =
        (PFUNC_aspell_module_info_enumeration_clone)GetProcAddress(
            h_inst_lib, "aspell_module_info_enumeration_clone");
    aspell_module_info_enumeration_assign =
        (PFUNC_aspell_module_info_enumeration_assign)GetProcAddress(
            h_inst_lib, "aspell_module_info_enumeration_assign");
    aspell_dict_info_enumeration_at_end =
        (PFUNC_aspell_dict_info_enumeration_at_end)GetProcAddress(
            h_inst_lib, "aspell_dict_info_enumeration_at_end");
    aspell_dict_info_enumeration_next =
        (PFUNC_aspell_dict_info_enumeration_next)GetProcAddress(
            h_inst_lib, "aspell_dict_info_enumeration_next");
    delete_aspell_dict_info_enumeration =
        (PFUNC_delete_aspell_dict_info_enumeration)GetProcAddress(
            h_inst_lib, "delete_aspell_dict_info_enumeration");
    aspell_dict_info_enumeration_clone =
        (PFUNC_aspell_dict_info_enumeration_clone)GetProcAddress(
            h_inst_lib, "aspell_dict_info_enumeration_clone");
    aspell_dict_info_enumeration_assign =
        (PFUNC_aspell_dict_info_enumeration_assign)GetProcAddress(
            h_inst_lib, "aspell_dict_info_enumeration_assign");
    new_aspell_string_list = (PFUNC_new_aspell_string_list)GetProcAddress(
        h_inst_lib, "new_aspell_string_list");
    aspell_string_list_empty = (PFUNC_aspell_string_list_empty)GetProcAddress(
        h_inst_lib, "aspell_string_list_empty");
    aspell_string_list_size = (PFUNC_aspell_string_list_size)GetProcAddress(
        h_inst_lib, "aspell_string_list_size");
    aspell_string_list_elements =
        (PFUNC_aspell_string_list_elements)GetProcAddress(
            h_inst_lib, "aspell_string_list_elements");
    aspell_string_list_add = (PFUNC_aspell_string_list_add)GetProcAddress(
        h_inst_lib, "aspell_string_list_add");
    aspell_string_list_remove = (PFUNC_aspell_string_list_remove)GetProcAddress(
        h_inst_lib, "aspell_string_list_remove");
    aspell_string_list_clear = (PFUNC_aspell_string_list_clear)GetProcAddress(
        h_inst_lib, "aspell_string_list_clear");
    aspell_string_list_to_mutable_container =
        (PFUNC_aspell_string_list_to_mutable_container)GetProcAddress(
            h_inst_lib, "aspell_string_list_to_mutable_container");
    delete_aspell_string_list = (PFUNC_delete_aspell_string_list)GetProcAddress(
        h_inst_lib, "delete_aspell_string_list");
    aspell_string_list_clone = (PFUNC_aspell_string_list_clone)GetProcAddress(
        h_inst_lib, "aspell_string_list_clone");
    aspell_string_list_assign = (PFUNC_aspell_string_list_assign)GetProcAddress(
        h_inst_lib, "aspell_string_list_assign");
    new_aspell_string_map = (PFUNC_new_aspell_string_map)GetProcAddress(
        h_inst_lib, "new_aspell_string_map");
    aspell_string_map_add = (PFUNC_aspell_string_map_add)GetProcAddress(
        h_inst_lib, "aspell_string_map_add");
    aspell_string_map_remove = (PFUNC_aspell_string_map_remove)GetProcAddress(
        h_inst_lib, "aspell_string_map_remove");
    aspell_string_map_clear = (PFUNC_aspell_string_map_clear)GetProcAddress(
        h_inst_lib, "aspell_string_map_clear");
    aspell_string_map_to_mutable_container =
        (PFUNC_aspell_string_map_to_mutable_container)GetProcAddress(
            h_inst_lib, "aspell_string_map_to_mutable_container");
    delete_aspell_string_map = (PFUNC_delete_aspell_string_map)GetProcAddress(
        h_inst_lib, "delete_aspell_string_map");
    aspell_string_map_clone = (PFUNC_aspell_string_map_clone)GetProcAddress(
        h_inst_lib, "aspell_string_map_clone");
    aspell_string_map_assign = (PFUNC_aspell_string_map_assign)GetProcAddress(
        h_inst_lib, "aspell_string_map_assign");
    aspell_string_map_empty = (PFUNC_aspell_string_map_empty)GetProcAddress(
        h_inst_lib, "aspell_string_map_empty");
    aspell_string_map_size = (PFUNC_aspell_string_map_size)GetProcAddress(
        h_inst_lib, "aspell_string_map_size");
    aspell_string_map_elements =
        (PFUNC_aspell_string_map_elements)GetProcAddress(
            h_inst_lib, "aspell_string_map_elements");
    aspell_string_map_insert = (PFUNC_aspell_string_map_insert)GetProcAddress(
        h_inst_lib, "aspell_string_map_insert");
    aspell_string_map_replace = (PFUNC_aspell_string_map_replace)GetProcAddress(
        h_inst_lib, "aspell_string_map_replace");
    aspell_string_map_lookup = (PFUNC_aspell_string_map_lookup)GetProcAddress(
        h_inst_lib, "aspell_string_map_lookup");
    aspell_string_pair_enumeration_at_end =
        (PFUNC_aspell_string_pair_enumeration_at_end)GetProcAddress(
            h_inst_lib, "aspell_string_pair_enumeration_at_end");
    aspell_string_pair_enumeration_next =
        (PFUNC_aspell_string_pair_enumeration_next)GetProcAddress(
            h_inst_lib, "aspell_string_pair_enumeration_next");
    delete_aspell_string_pair_enumeration =
        (PFUNC_delete_aspell_string_pair_enumeration)GetProcAddress(
            h_inst_lib, "delete_aspell_string_pair_enumeration");
    aspell_string_pair_enumeration_clone =
        (PFUNC_aspell_string_pair_enumeration_clone)GetProcAddress(
            h_inst_lib, "aspell_string_pair_enumeration_clone");
    aspell_string_pair_enumeration_assign =
        (PFUNC_aspell_string_pair_enumeration_assign)GetProcAddress(
            h_inst_lib, "aspell_string_pair_enumeration_assign");

    if (!new_aspell_config) // TODO: Add check for all used functions
      return false;

    b_ret = true;
  }

  return b_ret;
}

void unload_aspell() {
  if (h_inst_lib != nullptr)
    FreeLibrary(h_inst_lib);
}

void aspell_error_msg_box(HWND h_wnd, LPCSTR sz_error_msg) {
  wchar_t sz_msg[MAX_PATH];
#ifdef UNICODE
  wchar_t szTemp[MAX_PATH];
  ::MultiByteToWideChar(CP_ACP, 0, sz_error_msg, -1, szTemp, MAX_PATH);
  swprintf(sz_msg, L"Error:\n%s", szTemp);
#else
  swprintf(szMsg, _T("Error:\n%s"), szErrorMsg);
#endif
  ::MessageBox(h_wnd, sz_msg, L"GNU Aspell", MB_OK);
}