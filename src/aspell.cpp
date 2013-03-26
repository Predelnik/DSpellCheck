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
#include <fstream>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>

#include "CommonFunctions.h"
#include "MainDef.h"

HANDLE  g_hModule;

HINSTANCE   hInstLib = NULL;

PFUNC_aspell_mutable_container_add                  aspell_mutable_container_add                  = NULL;
PFUNC_aspell_mutable_container_remove               aspell_mutable_container_remove               = NULL;
PFUNC_aspell_mutable_container_clear                aspell_mutable_container_clear                = NULL;
PFUNC_aspell_mutable_container_to_mutable_container aspell_mutable_container_to_mutable_container = NULL;
PFUNC_aspell_key_info_enumeration_at_end            aspell_key_info_enumeration_at_end            = NULL;
PFUNC_aspell_key_info_enumeration_next              aspell_key_info_enumeration_next              = NULL;
PFUNC_delete_aspell_key_info_enumeration            delete_aspell_key_info_enumeration            = NULL;
PFUNC_aspell_key_info_enumeration_clone             aspell_key_info_enumeration_clone             = NULL;
PFUNC_aspell_key_info_enumeration_assign            aspell_key_info_enumeration_assign            = NULL;
PFUNC_new_aspell_config                             new_aspell_config                             = NULL;
PFUNC_delete_aspell_config                          delete_aspell_config                          = NULL;
PFUNC_aspell_config_clone                           aspell_config_clone                           = NULL;
PFUNC_aspell_config_assign                          aspell_config_assign                          = NULL;
PFUNC_aspell_config_error_number                    aspell_config_error_number                    = NULL;
PFUNC_aspell_config_error_message                   aspell_config_error_message                   = NULL;
PFUNC_aspell_config_error                           aspell_config_error                           = NULL;
PFUNC_aspell_config_set_extra                       aspell_config_set_extra                       = NULL;
PFUNC_aspell_config_keyinfo                         aspell_config_keyinfo                         = NULL;
PFUNC_aspell_config_possible_elements               aspell_config_possible_elements               = NULL;
PFUNC_aspell_config_get_default                     aspell_config_get_default                     = NULL;
PFUNC_aspell_config_elements                        aspell_config_elements                        = NULL;
PFUNC_aspell_config_replace                         aspell_config_replace                         = NULL;
PFUNC_aspell_config_remove                          aspell_config_remove                          = NULL;
PFUNC_aspell_config_have                            aspell_config_have                            = NULL;
PFUNC_aspell_config_retrieve                        aspell_config_retrieve                        = NULL;
PFUNC_aspell_config_retrieve_list                   aspell_config_retrieve_list                   = NULL;
PFUNC_aspell_config_retrieve_bool                   aspell_config_retrieve_bool                   = NULL;
PFUNC_aspell_config_retrieve_int                    aspell_config_retrieve_int                    = NULL;
PFUNC_aspell_error_number                           aspell_error_number                           = NULL;
PFUNC_aspell_error_message                          aspell_error_message                          = NULL;
PFUNC_aspell_error                                  aspell_error                                  = NULL;
PFUNC_delete_aspell_can_have_error                  delete_aspell_can_have_error                  = NULL;
PFUNC_new_aspell_speller                            new_aspell_speller                            = NULL;
PFUNC_to_aspell_speller                             to_aspell_speller                             = NULL;
PFUNC_delete_aspell_speller                         delete_aspell_speller                         = NULL;
PFUNC_aspell_speller_error_number                   aspell_speller_error_number                   = NULL;
PFUNC_aspell_speller_error_message                  aspell_speller_error_message                  = NULL;
PFUNC_aspell_speller_error                          aspell_speller_error                          = NULL;
PFUNC_aspell_speller_config                         aspell_speller_config                         = NULL;
PFUNC_aspell_speller_check                          aspell_speller_check                          = NULL;
PFUNC_aspell_speller_add_to_personal                aspell_speller_add_to_personal                = NULL;
PFUNC_aspell_speller_add_to_session                 aspell_speller_add_to_session                 = NULL;
PFUNC_aspell_speller_personal_word_list             aspell_speller_personal_word_list             = NULL;
PFUNC_aspell_speller_session_word_list              aspell_speller_session_word_list              = NULL;
PFUNC_aspell_speller_main_word_list                 aspell_speller_main_word_list                 = NULL;
PFUNC_aspell_speller_save_all_word_lists            aspell_speller_save_all_word_lists            = NULL;
PFUNC_aspell_speller_clear_session                  aspell_speller_clear_session                  = NULL;
PFUNC_aspell_speller_suggest                        aspell_speller_suggest                        = NULL;
PFUNC_aspell_speller_store_replacement              aspell_speller_store_replacement              = NULL;
PFUNC_delete_aspell_filter                          delete_aspell_filter                          = NULL;
PFUNC_aspell_filter_error_number                    aspell_filter_error_number                    = NULL;
PFUNC_aspell_filter_error_message                   aspell_filter_error_message                   = NULL;
PFUNC_aspell_filter_error                           aspell_filter_error                           = NULL;
PFUNC_to_aspell_filter                              to_aspell_filter                              = NULL;
PFUNC_delete_aspell_document_checker                delete_aspell_document_checker                = NULL;
PFUNC_aspell_document_checker_error_number          aspell_document_checker_error_number          = NULL;
PFUNC_aspell_document_checker_error_message         aspell_document_checker_error_message         = NULL;
PFUNC_aspell_document_checker_error                 aspell_document_checker_error                 = NULL;
PFUNC_new_aspell_document_checker                   new_aspell_document_checker                   = NULL;
PFUNC_to_aspell_document_checker                    to_aspell_document_checker                    = NULL;
PFUNC_aspell_document_checker_reset                 aspell_document_checker_reset                 = NULL;
PFUNC_aspell_document_checker_process               aspell_document_checker_process               = NULL;
PFUNC_aspell_document_checker_next_misspelling      aspell_document_checker_next_misspelling      = NULL;
PFUNC_aspell_document_checker_filter                aspell_document_checker_filter                = NULL;
PFUNC_aspell_word_list_empty                        aspell_word_list_empty                        = NULL;
PFUNC_aspell_word_list_size                         aspell_word_list_size                         = NULL;
PFUNC_aspell_word_list_elements                     aspell_word_list_elements                     = NULL;
PFUNC_delete_aspell_string_enumeration              delete_aspell_string_enumeration              = NULL;
PFUNC_aspell_string_enumeration_clone               aspell_string_enumeration_clone               = NULL;
PFUNC_aspell_string_enumeration_assign              aspell_string_enumeration_assign              = NULL;
PFUNC_aspell_string_enumeration_at_end              aspell_string_enumeration_at_end              = NULL;
PFUNC_aspell_string_enumeration_next                aspell_string_enumeration_next                = NULL;
PFUNC_get_aspell_module_info_list                   get_aspell_module_info_list                   = NULL;
PFUNC_aspell_module_info_list_empty                 aspell_module_info_list_empty                 = NULL;
PFUNC_aspell_module_info_list_size                  aspell_module_info_list_size                  = NULL;
PFUNC_aspell_module_info_list_elements              aspell_module_info_list_elements              = NULL;
PFUNC_get_aspell_dict_info_list                     get_aspell_dict_info_list                     = NULL;
PFUNC_aspell_dict_info_list_empty                   aspell_dict_info_list_empty                   = NULL;
PFUNC_aspell_dict_info_list_size                    aspell_dict_info_list_size                    = NULL;
PFUNC_aspell_dict_info_list_elements                aspell_dict_info_list_elements                = NULL;
PFUNC_aspell_module_info_enumeration_at_end         aspell_module_info_enumeration_at_end         = NULL;
PFUNC_aspell_module_info_enumeration_next           aspell_module_info_enumeration_next           = NULL;
PFUNC_delete_aspell_module_info_enumeration         delete_aspell_module_info_enumeration         = NULL;
PFUNC_aspell_module_info_enumeration_clone          aspell_module_info_enumeration_clone          = NULL;
PFUNC_aspell_module_info_enumeration_assign         aspell_module_info_enumeration_assign         = NULL;
PFUNC_aspell_dict_info_enumeration_at_end           aspell_dict_info_enumeration_at_end           = NULL;
PFUNC_aspell_dict_info_enumeration_next             aspell_dict_info_enumeration_next             = NULL;
PFUNC_delete_aspell_dict_info_enumeration           delete_aspell_dict_info_enumeration           = NULL;
PFUNC_aspell_dict_info_enumeration_clone            aspell_dict_info_enumeration_clone            = NULL;
PFUNC_aspell_dict_info_enumeration_assign           aspell_dict_info_enumeration_assign           = NULL;
PFUNC_new_aspell_string_list                        new_aspell_string_list                        = NULL;
PFUNC_aspell_string_list_empty                      aspell_string_list_empty                      = NULL;
PFUNC_aspell_string_list_size                       aspell_string_list_size                       = NULL;
PFUNC_aspell_string_list_elements                   aspell_string_list_elements                   = NULL;
PFUNC_aspell_string_list_add                        aspell_string_list_add                        = NULL;
PFUNC_aspell_string_list_remove                     aspell_string_list_remove                     = NULL;
PFUNC_aspell_string_list_clear                      aspell_string_list_clear                      = NULL;
PFUNC_aspell_string_list_to_mutable_container       aspell_string_list_to_mutable_container       = NULL;
PFUNC_delete_aspell_string_list                     delete_aspell_string_list                     = NULL;
PFUNC_aspell_string_list_clone                      aspell_string_list_clone                      = NULL;
PFUNC_aspell_string_list_assign                     aspell_string_list_assign                     = NULL;
PFUNC_new_aspell_string_map                         new_aspell_string_map                         = NULL;
PFUNC_aspell_string_map_add                         aspell_string_map_add                         = NULL;
PFUNC_aspell_string_map_remove                      aspell_string_map_remove                      = NULL;
PFUNC_aspell_string_map_clear                       aspell_string_map_clear                       = NULL;
PFUNC_aspell_string_map_to_mutable_container        aspell_string_map_to_mutable_container        = NULL;
PFUNC_delete_aspell_string_map                      delete_aspell_string_map                      = NULL;
PFUNC_aspell_string_map_clone                       aspell_string_map_clone                       = NULL;
PFUNC_aspell_string_map_assign                      aspell_string_map_assign                      = NULL;
PFUNC_aspell_string_map_empty                       aspell_string_map_empty                       = NULL;
PFUNC_aspell_string_map_size                        aspell_string_map_size                        = NULL;
PFUNC_aspell_string_map_elements                    aspell_string_map_elements                    = NULL;
PFUNC_aspell_string_map_insert                      aspell_string_map_insert                      = NULL;
PFUNC_aspell_string_map_replace                     aspell_string_map_replace                     = NULL;
PFUNC_aspell_string_map_lookup                      aspell_string_map_lookup                      = NULL;
PFUNC_aspell_string_pair_enumeration_at_end         aspell_string_pair_enumeration_at_end         = NULL;
PFUNC_aspell_string_pair_enumeration_next           aspell_string_pair_enumeration_next           = NULL;
PFUNC_delete_aspell_string_pair_enumeration         delete_aspell_string_pair_enumeration         = NULL;
PFUNC_aspell_string_pair_enumeration_clone          aspell_string_pair_enumeration_clone          = NULL;
PFUNC_aspell_string_pair_enumeration_assign         aspell_string_pair_enumeration_assign         = NULL;

void GetDefaultAspellPath (TCHAR *&Path)
{
  TCHAR pszPath[MAX_PATH];
  pszPath[0] = '\0';
  HKEY    hKey = NULL;
  DWORD   size = MAX_PATH;

  if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T ("SOFTWARE\\Aspell"), 0, KEY_READ, &hKey))
  {
    if (ERROR_SUCCESS == ::RegQueryValueEx(hKey, _T ("Path"), NULL ,NULL, (LPBYTE)pszPath, &size))
      wcscat(pszPath, _T ("\\aspell-15.dll"));
    ::RegCloseKey(hKey);
  }
  else
  {
    TCHAR Pf[MAX_PATH];
    SHGetSpecialFolderPath(
      0,
      Pf,
      CSIDL_PROGRAM_FILES,
      FALSE );
    PathAppend(pszPath, Pf);
    PathAppend(pszPath, _T("\\Aspell\\bin\\aspell-15.dll"));
  }
  SetString (Path, pszPath);
}

void GetActualAspellPath (TCHAR *&Path, TCHAR *&PathArg)
{
  BOOL   bRet = FALSE;

  if (!PathArg || !*PathArg)
  {
    GetDefaultAspellPath (Path);
  }
  else
  {
    SetString (Path, PathArg);
  }
}

BOOL LoadAspell (TCHAR *PathArg)
{
  TCHAR *Path = 0;
  GetActualAspellPath (Path, PathArg);
  hInstLib = LoadLibrary(Path);
  CLEAN_AND_ZERO_ARR (Path);
  BOOL bRet = FALSE;

  if (hInstLib != NULL)
  {
    aspell_mutable_container_add                  = (PFUNC_aspell_mutable_container_add                 )GetProcAddress(hInstLib, "aspell_mutable_container_add");
    aspell_mutable_container_remove               = (PFUNC_aspell_mutable_container_remove              )GetProcAddress(hInstLib, "aspell_mutable_container_remove");
    aspell_mutable_container_clear                = (PFUNC_aspell_mutable_container_clear               )GetProcAddress(hInstLib, "aspell_mutable_container_clear");
    aspell_mutable_container_to_mutable_container = (PFUNC_aspell_mutable_container_to_mutable_container)GetProcAddress(hInstLib, "aspell_mutable_container_to_mutable_container");
    aspell_key_info_enumeration_at_end            = (PFUNC_aspell_key_info_enumeration_at_end           )GetProcAddress(hInstLib, "aspell_key_info_enumeration_at_end");
    aspell_key_info_enumeration_next              = (PFUNC_aspell_key_info_enumeration_next             )GetProcAddress(hInstLib, "aspell_key_info_enumeration_next");
    delete_aspell_key_info_enumeration            = (PFUNC_delete_aspell_key_info_enumeration           )GetProcAddress(hInstLib, "delete_aspell_key_info_enumeration");
    aspell_key_info_enumeration_clone             = (PFUNC_aspell_key_info_enumeration_clone            )GetProcAddress(hInstLib, "aspell_key_info_enumeration_clone");
    aspell_key_info_enumeration_assign            = (PFUNC_aspell_key_info_enumeration_assign           )GetProcAddress(hInstLib, "aspell_key_info_enumeration_assign");
    new_aspell_config                             = (PFUNC_new_aspell_config                            )GetProcAddress(hInstLib, "new_aspell_config");
    delete_aspell_config                          = (PFUNC_delete_aspell_config                         )GetProcAddress(hInstLib, "delete_aspell_config");
    aspell_config_clone                           = (PFUNC_aspell_config_clone                          )GetProcAddress(hInstLib, "aspell_config_clone");
    aspell_config_assign                          = (PFUNC_aspell_config_assign                         )GetProcAddress(hInstLib, "aspell_config_assign");
    aspell_config_error_number                    = (PFUNC_aspell_config_error_number                   )GetProcAddress(hInstLib, "aspell_config_error_number");
    aspell_config_error_message                   = (PFUNC_aspell_config_error_message                  )GetProcAddress(hInstLib, "aspell_config_error_message");
    aspell_config_error                           = (PFUNC_aspell_config_error                          )GetProcAddress(hInstLib, "aspell_config_error");
    aspell_config_set_extra                       = (PFUNC_aspell_config_set_extra                      )GetProcAddress(hInstLib, "aspell_config_set_extra");
    aspell_config_keyinfo                         = (PFUNC_aspell_config_keyinfo                        )GetProcAddress(hInstLib, "aspell_config_keyinfo");
    aspell_config_possible_elements               = (PFUNC_aspell_config_possible_elements              )GetProcAddress(hInstLib, "aspell_config_possible_elements");
    aspell_config_get_default                     = (PFUNC_aspell_config_get_default                    )GetProcAddress(hInstLib, "aspell_config_get_default");
    aspell_config_elements                        = (PFUNC_aspell_config_elements                       )GetProcAddress(hInstLib, "aspell_config_elements");
    aspell_config_replace                         = (PFUNC_aspell_config_replace                        )GetProcAddress(hInstLib, "aspell_config_replace");
    aspell_config_remove                          = (PFUNC_aspell_config_remove                         )GetProcAddress(hInstLib, "aspell_config_remove");
    aspell_config_have                            = (PFUNC_aspell_config_have                           )GetProcAddress(hInstLib, "aspell_config_have");
    aspell_config_retrieve                        = (PFUNC_aspell_config_retrieve                       )GetProcAddress(hInstLib, "aspell_config_retrieve");
    aspell_config_retrieve_list                   = (PFUNC_aspell_config_retrieve_list                  )GetProcAddress(hInstLib, "aspell_config_retrieve_list");
    aspell_config_retrieve_bool                   = (PFUNC_aspell_config_retrieve_bool                  )GetProcAddress(hInstLib, "aspell_config_retrieve_bool");
    aspell_config_retrieve_int                    = (PFUNC_aspell_config_retrieve_int                   )GetProcAddress(hInstLib, "aspell_config_retrieve_int");
    aspell_error_number                           = (PFUNC_aspell_error_number                          )GetProcAddress(hInstLib, "aspell_error_number");
    aspell_error_message                          = (PFUNC_aspell_error_message                         )GetProcAddress(hInstLib, "aspell_error_message");
    aspell_error                                  = (PFUNC_aspell_error                                 )GetProcAddress(hInstLib, "aspell_error");
    delete_aspell_can_have_error                  = (PFUNC_delete_aspell_can_have_error                 )GetProcAddress(hInstLib, "delete_aspell_can_have_error");
    new_aspell_speller                            = (PFUNC_new_aspell_speller                           )GetProcAddress(hInstLib, "new_aspell_speller");
    to_aspell_speller                             = (PFUNC_to_aspell_speller                            )GetProcAddress(hInstLib, "to_aspell_speller");
    delete_aspell_speller                         = (PFUNC_delete_aspell_speller                        )GetProcAddress(hInstLib, "delete_aspell_speller");
    aspell_speller_error_number                   = (PFUNC_aspell_speller_error_number                  )GetProcAddress(hInstLib, "aspell_speller_error_number");
    aspell_speller_error_message                  = (PFUNC_aspell_speller_error_message                 )GetProcAddress(hInstLib, "aspell_speller_error_message");
    aspell_speller_error                          = (PFUNC_aspell_speller_error                         )GetProcAddress(hInstLib, "aspell_speller_error");
    aspell_speller_config                         = (PFUNC_aspell_speller_config                        )GetProcAddress(hInstLib, "aspell_speller_config");
    aspell_speller_check                          = (PFUNC_aspell_speller_check                         )GetProcAddress(hInstLib, "aspell_speller_check");
    aspell_speller_add_to_personal                = (PFUNC_aspell_speller_add_to_personal               )GetProcAddress(hInstLib, "aspell_speller_add_to_personal");
    aspell_speller_add_to_session                 = (PFUNC_aspell_speller_add_to_session                )GetProcAddress(hInstLib, "aspell_speller_add_to_session");
    aspell_speller_personal_word_list             = (PFUNC_aspell_speller_personal_word_list            )GetProcAddress(hInstLib, "aspell_speller_personal_word_list");
    aspell_speller_session_word_list              = (PFUNC_aspell_speller_session_word_list             )GetProcAddress(hInstLib, "aspell_speller_session_word_list");
    aspell_speller_main_word_list                 = (PFUNC_aspell_speller_main_word_list                )GetProcAddress(hInstLib, "aspell_speller_main_word_list");
    aspell_speller_save_all_word_lists            = (PFUNC_aspell_speller_save_all_word_lists           )GetProcAddress(hInstLib, "aspell_speller_save_all_word_lists");
    aspell_speller_clear_session                  = (PFUNC_aspell_speller_clear_session                 )GetProcAddress(hInstLib, "aspell_speller_clear_session");
    aspell_speller_suggest                        = (PFUNC_aspell_speller_suggest                       )GetProcAddress(hInstLib, "aspell_speller_suggest");
    aspell_speller_store_replacement              = (PFUNC_aspell_speller_store_replacement             )GetProcAddress(hInstLib, "aspell_speller_store_replacement");
    delete_aspell_filter                          = (PFUNC_delete_aspell_filter                         )GetProcAddress(hInstLib, "delete_aspell_filter");
    aspell_filter_error_number                    = (PFUNC_aspell_filter_error_number                   )GetProcAddress(hInstLib, "aspell_filter_error_number");
    aspell_filter_error_message                   = (PFUNC_aspell_filter_error_message                  )GetProcAddress(hInstLib, "aspell_filter_error_message");
    aspell_filter_error                           = (PFUNC_aspell_filter_error                          )GetProcAddress(hInstLib, "aspell_filter_error");
    to_aspell_filter                              = (PFUNC_to_aspell_filter                             )GetProcAddress(hInstLib, "to_aspell_filter");
    delete_aspell_document_checker                = (PFUNC_delete_aspell_document_checker               )GetProcAddress(hInstLib, "delete_aspell_document_checker");
    aspell_document_checker_error_number          = (PFUNC_aspell_document_checker_error_number         )GetProcAddress(hInstLib, "aspell_document_checker_error_number");
    aspell_document_checker_error_message         = (PFUNC_aspell_document_checker_error_message        )GetProcAddress(hInstLib, "aspell_document_checker_error_message");
    aspell_document_checker_error                 = (PFUNC_aspell_document_checker_error                )GetProcAddress(hInstLib, "aspell_document_checker_error");
    new_aspell_document_checker                   = (PFUNC_new_aspell_document_checker                  )GetProcAddress(hInstLib, "new_aspell_document_checker");
    to_aspell_document_checker                    = (PFUNC_to_aspell_document_checker                   )GetProcAddress(hInstLib, "to_aspell_document_checker");
    aspell_document_checker_reset                 = (PFUNC_aspell_document_checker_reset                )GetProcAddress(hInstLib, "aspell_document_checker_reset");
    aspell_document_checker_process               = (PFUNC_aspell_document_checker_process              )GetProcAddress(hInstLib, "aspell_document_checker_process");
    aspell_document_checker_next_misspelling      = (PFUNC_aspell_document_checker_next_misspelling     )GetProcAddress(hInstLib, "aspell_document_checker_next_misspelling");
    aspell_document_checker_filter                = (PFUNC_aspell_document_checker_filter               )GetProcAddress(hInstLib, "aspell_document_checker_filter");
    aspell_word_list_empty                        = (PFUNC_aspell_word_list_empty                       )GetProcAddress(hInstLib, "aspell_word_list_empty");
    aspell_word_list_size                         = (PFUNC_aspell_word_list_size                        )GetProcAddress(hInstLib, "aspell_word_list_size");
    aspell_word_list_elements                     = (PFUNC_aspell_word_list_elements                    )GetProcAddress(hInstLib, "aspell_word_list_elements");
    delete_aspell_string_enumeration              = (PFUNC_delete_aspell_string_enumeration             )GetProcAddress(hInstLib, "delete_aspell_string_enumeration");
    aspell_string_enumeration_clone               = (PFUNC_aspell_string_enumeration_clone              )GetProcAddress(hInstLib, "aspell_string_enumeration_clone");
    aspell_string_enumeration_assign              = (PFUNC_aspell_string_enumeration_assign             )GetProcAddress(hInstLib, "aspell_string_enumeration_assign");
    aspell_string_enumeration_at_end              = (PFUNC_aspell_string_enumeration_at_end             )GetProcAddress(hInstLib, "aspell_string_enumeration_at_end");
    aspell_string_enumeration_next                = (PFUNC_aspell_string_enumeration_next               )GetProcAddress(hInstLib, "aspell_string_enumeration_next");
    get_aspell_module_info_list                   = (PFUNC_get_aspell_module_info_list                  )GetProcAddress(hInstLib, "get_aspell_module_info_list");
    aspell_module_info_list_empty                 = (PFUNC_aspell_module_info_list_empty                )GetProcAddress(hInstLib, "aspell_module_info_list_empty");
    aspell_module_info_list_size                  = (PFUNC_aspell_module_info_list_size                 )GetProcAddress(hInstLib, "aspell_module_info_list_size");
    aspell_module_info_list_elements              = (PFUNC_aspell_module_info_list_elements             )GetProcAddress(hInstLib, "aspell_module_info_list_elements");
    get_aspell_dict_info_list                     = (PFUNC_get_aspell_dict_info_list                    )GetProcAddress(hInstLib, "get_aspell_dict_info_list");
    aspell_dict_info_list_empty                   = (PFUNC_aspell_dict_info_list_empty                  )GetProcAddress(hInstLib, "aspell_dict_info_list_empty");
    aspell_dict_info_list_size                    = (PFUNC_aspell_dict_info_list_size                   )GetProcAddress(hInstLib, "aspell_dict_info_list_size");
    aspell_dict_info_list_elements                = (PFUNC_aspell_dict_info_list_elements               )GetProcAddress(hInstLib, "aspell_dict_info_list_elements");
    aspell_module_info_enumeration_at_end         = (PFUNC_aspell_module_info_enumeration_at_end        )GetProcAddress(hInstLib, "aspell_module_info_enumeration_at_end");
    aspell_module_info_enumeration_next           = (PFUNC_aspell_module_info_enumeration_next          )GetProcAddress(hInstLib, "aspell_module_info_enumeration_next");
    delete_aspell_module_info_enumeration         = (PFUNC_delete_aspell_module_info_enumeration        )GetProcAddress(hInstLib, "delete_aspell_module_info_enumeration");
    aspell_module_info_enumeration_clone          = (PFUNC_aspell_module_info_enumeration_clone         )GetProcAddress(hInstLib, "aspell_module_info_enumeration_clone");
    aspell_module_info_enumeration_assign         = (PFUNC_aspell_module_info_enumeration_assign        )GetProcAddress(hInstLib, "aspell_module_info_enumeration_assign");
    aspell_dict_info_enumeration_at_end           = (PFUNC_aspell_dict_info_enumeration_at_end          )GetProcAddress(hInstLib, "aspell_dict_info_enumeration_at_end");
    aspell_dict_info_enumeration_next             = (PFUNC_aspell_dict_info_enumeration_next            )GetProcAddress(hInstLib, "aspell_dict_info_enumeration_next");
    delete_aspell_dict_info_enumeration           = (PFUNC_delete_aspell_dict_info_enumeration          )GetProcAddress(hInstLib, "delete_aspell_dict_info_enumeration");
    aspell_dict_info_enumeration_clone            = (PFUNC_aspell_dict_info_enumeration_clone           )GetProcAddress(hInstLib, "aspell_dict_info_enumeration_clone");
    aspell_dict_info_enumeration_assign           = (PFUNC_aspell_dict_info_enumeration_assign          )GetProcAddress(hInstLib, "aspell_dict_info_enumeration_assign");
    new_aspell_string_list                        = (PFUNC_new_aspell_string_list                       )GetProcAddress(hInstLib, "new_aspell_string_list");
    aspell_string_list_empty                      = (PFUNC_aspell_string_list_empty                     )GetProcAddress(hInstLib, "aspell_string_list_empty");
    aspell_string_list_size                       = (PFUNC_aspell_string_list_size                      )GetProcAddress(hInstLib, "aspell_string_list_size");
    aspell_string_list_elements                   = (PFUNC_aspell_string_list_elements                  )GetProcAddress(hInstLib, "aspell_string_list_elements");
    aspell_string_list_add                        = (PFUNC_aspell_string_list_add                       )GetProcAddress(hInstLib, "aspell_string_list_add");
    aspell_string_list_remove                     = (PFUNC_aspell_string_list_remove                    )GetProcAddress(hInstLib, "aspell_string_list_remove");
    aspell_string_list_clear                      = (PFUNC_aspell_string_list_clear                     )GetProcAddress(hInstLib, "aspell_string_list_clear");
    aspell_string_list_to_mutable_container       = (PFUNC_aspell_string_list_to_mutable_container      )GetProcAddress(hInstLib, "aspell_string_list_to_mutable_container");
    delete_aspell_string_list                     = (PFUNC_delete_aspell_string_list                    )GetProcAddress(hInstLib, "delete_aspell_string_list");
    aspell_string_list_clone                      = (PFUNC_aspell_string_list_clone                     )GetProcAddress(hInstLib, "aspell_string_list_clone");
    aspell_string_list_assign                     = (PFUNC_aspell_string_list_assign                    )GetProcAddress(hInstLib, "aspell_string_list_assign");
    new_aspell_string_map                         = (PFUNC_new_aspell_string_map                        )GetProcAddress(hInstLib, "new_aspell_string_map");
    aspell_string_map_add                         = (PFUNC_aspell_string_map_add                        )GetProcAddress(hInstLib, "aspell_string_map_add");
    aspell_string_map_remove                      = (PFUNC_aspell_string_map_remove                     )GetProcAddress(hInstLib, "aspell_string_map_remove");
    aspell_string_map_clear                       = (PFUNC_aspell_string_map_clear                      )GetProcAddress(hInstLib, "aspell_string_map_clear");
    aspell_string_map_to_mutable_container        = (PFUNC_aspell_string_map_to_mutable_container       )GetProcAddress(hInstLib, "aspell_string_map_to_mutable_container");
    delete_aspell_string_map                      = (PFUNC_delete_aspell_string_map                     )GetProcAddress(hInstLib, "delete_aspell_string_map");
    aspell_string_map_clone                       = (PFUNC_aspell_string_map_clone                      )GetProcAddress(hInstLib, "aspell_string_map_clone");
    aspell_string_map_assign                      = (PFUNC_aspell_string_map_assign                     )GetProcAddress(hInstLib, "aspell_string_map_assign");
    aspell_string_map_empty                       = (PFUNC_aspell_string_map_empty                      )GetProcAddress(hInstLib, "aspell_string_map_empty");
    aspell_string_map_size                        = (PFUNC_aspell_string_map_size                       )GetProcAddress(hInstLib, "aspell_string_map_size");
    aspell_string_map_elements                    = (PFUNC_aspell_string_map_elements                   )GetProcAddress(hInstLib, "aspell_string_map_elements");
    aspell_string_map_insert                      = (PFUNC_aspell_string_map_insert                     )GetProcAddress(hInstLib, "aspell_string_map_insert");
    aspell_string_map_replace                     = (PFUNC_aspell_string_map_replace                    )GetProcAddress(hInstLib, "aspell_string_map_replace");
    aspell_string_map_lookup                      = (PFUNC_aspell_string_map_lookup                     )GetProcAddress(hInstLib, "aspell_string_map_lookup");
    aspell_string_pair_enumeration_at_end         = (PFUNC_aspell_string_pair_enumeration_at_end        )GetProcAddress(hInstLib, "aspell_string_pair_enumeration_at_end");
    aspell_string_pair_enumeration_next           = (PFUNC_aspell_string_pair_enumeration_next          )GetProcAddress(hInstLib, "aspell_string_pair_enumeration_next");
    delete_aspell_string_pair_enumeration         = (PFUNC_delete_aspell_string_pair_enumeration        )GetProcAddress(hInstLib, "delete_aspell_string_pair_enumeration");
    aspell_string_pair_enumeration_clone          = (PFUNC_aspell_string_pair_enumeration_clone         )GetProcAddress(hInstLib, "aspell_string_pair_enumeration_clone");
    aspell_string_pair_enumeration_assign         = (PFUNC_aspell_string_pair_enumeration_assign        )GetProcAddress(hInstLib, "aspell_string_pair_enumeration_assign");

    if (!new_aspell_config) // TODO: Add check for all used functions
      return FALSE;

    bRet = TRUE;
  }

  return bRet;
}

void UnloadAspell(void)
{
  if (hInstLib != NULL)
    FreeLibrary(hInstLib);
}

void AspellErrorMsgBox(HWND hWnd, LPCSTR szErrorMsg)
{
  TCHAR	szMsg[MAX_PATH];
#ifdef UNICODE
  TCHAR	szTemp[MAX_PATH];
  ::MultiByteToWideChar(CP_ACP, 0, szErrorMsg, -1, szTemp, MAX_PATH);
  _stprintf(szMsg, _T("Error:\n%s"), szTemp);
#else
  _stprintf(szMsg, _T("Error:\n%s"), szErrorMsg);
#endif
  ::MessageBox(hWnd, szMsg, _T("GNU Aspell"), MB_OK);
}

