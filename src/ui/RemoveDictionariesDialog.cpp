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

#include "RemoveDictionariesDialog.h"

#include "CheckedList/CheckedList.h"
#include "common/winapi.h"
#include "core/SpellChecker.h"
#include "plugin/Plugin.h"
#include "plugin/resource.h"
#include "plugin/Settings.h"
#include "spellers/HunspellInterface.h"
#include "spellers/LanguageInfo.h"
#include "spellers/SpellerContainer.h"

RemoveDictionariesDialog::RemoveDictionariesDialog(HINSTANCE h_inst, HWND parent, const Settings &settings, const SpellerContainer &speller_container)
  : m_settings(settings), m_speller_container(speller_container) {
  m_settings.settings_changed.connect([this] { update_list(); });
  m_speller_container.speller_status_changed.connect([this] { update_list(); });
  Window::init(h_inst, parent);
  m_settings.settings_changed.connect([this] { update_controls(); });
}

void RemoveDictionariesDialog::do_dialog() {
  if (!isCreated()) {
    create(IDD_REMOVE_DICS);
  } else {
    goToCenter();
    display();
  }
  SetFocus(m_lang_list);
}

void RemoveDictionariesDialog::update_list() {
  if (m_lang_list == nullptr)
    return;

  std::set<std::wstring> currently_checked;
  for (int i = 0; i < ListBox_GetCount(m_lang_list); ++i)
    if (CheckedListBox_GetCheckState(m_lang_list, i) == BST_CHECKED)
      currently_checked.insert(m_cur_lang_list[i].orig_name);

  m_cur_lang_list = m_speller_container.get_available_languages();
  ListBox_ResetContent(m_lang_list);
  for (auto &lang : m_cur_lang_list)
    ListBox_AddString(m_lang_list,
                    (std::wstring(lang.alias_name) + (lang.for_all_users ? L" [!For All Users]" : L"")).c_str());

  for (int i = 0; i < ListBox_GetCount(m_lang_list); ++i)
    if (currently_checked.count(m_cur_lang_list[i].orig_name) > 0)
      CheckedListBox_SetCheckState(m_lang_list, i, BST_CHECKED);
}

HWND RemoveDictionariesDialog::get_list_box() { return m_lang_list; }

void RemoveDictionariesDialog::remove_selected() {
  int count = 0;
  bool need_single_reset = false;
  bool need_multi_reset = false;
  bool single_temp, multi_temp;
  for (int i = 0; i < ListBox_GetCount(m_lang_list); i++) {
    if (CheckedListBox_GetCheckState(m_lang_list, i) == BST_CHECKED) {
      wchar_t file_name[MAX_PATH];
      for (int j = 0; j < 1 + (m_settings.data.remove_system_dictionaries ? 1 : 0); j++) {
        *file_name = L'\0';
        wcscat(file_name, ((j == 0) ? m_settings.data.hunspell_user_path : m_settings.data.hunspell_system_path).c_str());
        wcscat(file_name, L"\\");
        wcscat(file_name, m_speller_container.get_available_languages()[i].orig_name.c_str());
        wcscat(file_name, L".aff");
        bool success = WinApi::delete_file(file_name);
        wcsncpy(file_name + wcslen(file_name) - 4, L".dic", 4);
        success = success && WinApi::delete_file(file_name);
        if (m_settings.data.remove_user_dictionaries) {
          wcsncpy(file_name + wcslen(file_name) - 4, L".usr", 4);
          WinApi::delete_file(file_name);
        }
        if (success) {
          file_name[wcslen(file_name) - 4] = L'\0';
          m_speller_container.get_hunspell_speller().update_on_dic_removal(file_name, single_temp, multi_temp);
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
    update_list();
    m_settings.settings_changed();
    auto text = wstring_printf(rc_str(IDS_PD_DICTIONARIES_REMOVED).c_str(), count);
    MessageBox(_hParent, text.c_str(), L"Dictionaries were removed", MB_OK | MB_ICONINFORMATION);
  }
  if (need_single_reset) {
    auto s = m_settings.modify();
    s->reset_hunspell_lang_to_default();
  }
  if (need_multi_reset) {
    auto s = m_settings.modify();
    s->data.speller_multi_languages[SpellerId::hunspell] = L"";
  }
}

void RemoveDictionariesDialog::update_options() {
  auto mut_settings = m_settings.modify();
  mut_settings->data.remove_user_dictionaries = Button_GetCheck(m_remove_user_dics) == BST_CHECKED;
  mut_settings->data.remove_system_dictionaries = Button_GetCheck(m_remove_system) == BST_CHECKED;
}

void RemoveDictionariesDialog::update_controls() {
  Button_SetCheck(m_remove_user_dics, m_settings.data.remove_user_dictionaries ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_remove_system, m_settings.data.remove_system_dictionaries ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR RemoveDictionariesDialog::run_dlg_proc(UINT message, WPARAM w_param, LPARAM /*lParam*/) {
  switch (message) {
  case WM_INITDIALOG: {
    m_lang_list = ::GetDlgItem(_hSelf, IDC_REMOVE_LANGLIST);
    m_remove_user_dics = ::GetDlgItem(_hSelf, IDC_REMOVE_USER_DICS);
    m_remove_system = ::GetDlgItem(_hSelf, IDC_REMOVE_SYSTEM);
    update_list();
    update_controls();
    return TRUE;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDC_REMOVE_USER_DICS:
    case IDC_REMOVE_SYSTEM: {
      if (HIWORD(w_param) == BN_CLICKED) {
        update_options();
      }
    }
    break;
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        display(false);
        remove_selected();
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        display(false);
        update_list(); // reset checkboxes
      }

      break;
    }
  }
  break;
  default:
    break;
  };
  return FALSE;
}
