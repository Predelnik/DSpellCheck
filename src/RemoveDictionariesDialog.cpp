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

#include "RemoveDictionariesDialog.h"

#include "CheckedList/CheckedList.h"
#include "MainDef.h"
#include "HunspellInterface.h"
#include "Plugin.h"
#include "SpellChecker.h"

#include "resource.h"

void RemoveDictionariesDialog::do_dialog() {
  if (!isCreated()) {
    create(IDD_REMOVE_DICS);
  } else {
    goToCenter();
    display();
  }
  SetFocus(m_lang_list);
}

void RemoveDictionariesDialog::init(HINSTANCE h_inst, HWND parent) {
  return Window::init(h_inst, parent);
}

HWND RemoveDictionariesDialog::get_list_box() { return m_lang_list; }

void RemoveDictionariesDialog::remove_selected(SpellChecker *spell_checker_instance) {
  int count = 0;
    bool need_single_reset = false;
  bool need_multi_reset = false;
  bool single_temp, multi_temp;
  for (int i = 0; i < ListBox_GetCount(m_lang_list); i++) {
    if (CheckedListBox_GetCheckState(m_lang_list, i) == BST_CHECKED) {
      wchar_t file_name[MAX_PATH];
      for (int j = 0; j < 1 + spell_checker_instance->get_remove_system() ? 1 : 0;
           j++) {
        *file_name = L'\0';
        wcscat(file_name,
               (j == 0) ? spell_checker_instance->get_hunspell_path()
                        : spell_checker_instance->get_hunspell_additional_path());
        wcscat(file_name, L"\\");
        wcscat(file_name, spell_checker_instance->get_lang_by_index(i));
        wcscat(file_name, L".aff");
        SetFileAttributes(file_name, FILE_ATTRIBUTE_NORMAL);
        bool success = DeleteFile(file_name);
        wcsncpy(file_name + wcslen(file_name) - 4, L".dic", 4);
        SetFileAttributes(file_name, FILE_ATTRIBUTE_NORMAL);
        success = success && DeleteFile(file_name);
        if (spell_checker_instance->get_remove_user_dics()) {
          wcsncpy(file_name + wcslen(file_name) - 4, L".usr", 4);
          SetFileAttributes(file_name, FILE_ATTRIBUTE_NORMAL);
          DeleteFile(file_name); // Success doesn't matter in that case, 'cause
                                // dictionary might not exist.
        }
        if (success) {
          file_name[wcslen(file_name) - 4] = L'\0';
          spell_checker_instance->get_hunspell_speller()->update_on_dic_removal(
              file_name, single_temp, multi_temp);
          need_single_reset |= single_temp;
          need_multi_reset |= multi_temp;
          count++;
        }
      }
    }
  }
  for (int i = 0; i < ListBox_GetCount(m_lang_list); i++)
    CheckedListBox_SetCheckState(m_lang_list, i, BST_UNCHECKED);
  if (count > 0) {
    spell_checker_instance->hunspell_reinit_settings(true);
    spell_checker_instance->reinit_language_lists(true);
    spell_checker_instance->do_plugin_menu_inclusion();
    spell_checker_instance->recheck_visible_both_views();
    wchar_t buf[DEFAULT_BUF_SIZE];
    swprintf(buf, L"%d dictionary(ies) has(ve) been successfully removed",
             count);
    MessageBox (_hParent, buf, L"Dictionaries were removed",
                MB_OK | MB_ICONINFORMATION);
  }
}

void RemoveDictionariesDialog::update_options(SpellChecker *spell_checker_instance) {
  spell_checker_instance->set_remove_user_dics(Button_GetCheck(m_remove_user_dics) ==
                                          BST_CHECKED);
  spell_checker_instance->set_remove_system(Button_GetCheck(m_remove_system) ==
                                        BST_CHECKED);
}

void RemoveDictionariesDialog::set_check_boxes(bool remove_user_dics, bool remove_system) {
  Button_SetCheck(m_remove_user_dics,
                  remove_user_dics ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_remove_system, remove_system ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR RemoveDictionariesDialog::run_dlg_proc(UINT message, WPARAM w_param,
                                LPARAM /*lParam*/) {
  switch (message) {
  case WM_INITDIALOG: {
    m_lang_list = ::GetDlgItem(_hSelf, IDC_REMOVE_LANGLIST);
    m_remove_user_dics = ::GetDlgItem(_hSelf, IDC_REMOVE_USER_DICS);
    m_remove_system = ::GetDlgItem(_hSelf, IDC_REMOVE_SYSTEM);
    get_spell_checker ()->reinit_language_lists(true);
    get_spell_checker ()->update_remove_dics_options();
    return true;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDC_REMOVE_USER_DICS:
    case IDC_REMOVE_SYSTEM: {
      if (HIWORD(w_param) == BN_CLICKED) {
        get_spell_checker ()->update_from_remove_dics_options();
      }
    } break;
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        remove_selected(get_spell_checker ());
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        for (int i = 0; i < ListBox_GetCount(m_lang_list); i++)
          CheckedListBox_SetCheckState(m_lang_list, i, BST_UNCHECKED);
        display(false);
      }

      break;
    }
  } break;
  };
  return false;
}