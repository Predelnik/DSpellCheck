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

#include "SelectMultipleLanguagesDialog.h"

#include "CheckedList/CheckedList.h"
#include "common/Utility.h"
#include "common/string_utils.h"
#include "plugin/Constants.h"
#include "plugin/Plugin.h"
#include "plugin/resource.h"
#include "plugin/Settings.h"
#include "spellers/LanguageInfo.h"
#include "spellers/SpellerContainer.h"

SelectMultipleLanguagesDialog::SelectMultipleLanguagesDialog(HINSTANCE h_inst, HWND parent, const Settings &settings, const SpellerContainer &speller_container)
  : m_settings(settings), m_speller_container(speller_container) {
  Window::init(h_inst, parent);
  m_settings.settings_changed.connect([this] { update_list(); });
  m_speller_container.speller_status_changed.connect([this] { update_list(); });
}

void SelectMultipleLanguagesDialog::do_dialog() {
  if (!isCreated()) {
    create(IDD_CHOOSE_MULTIPLE_LANGUAGES);
  }
  goToCenter();
  display();
  SetFocus(m_h_lang_list);
}

HWND SelectMultipleLanguagesDialog::get_list_box() { return m_h_lang_list; }

void SelectMultipleLanguagesDialog::update_list() {
  if (m_h_lang_list == nullptr)
    return;

  ListBox_ResetContent(m_h_lang_list);
  auto langs = m_speller_container.get_available_languages();
  for (auto &lang : langs) {
    ListBox_AddString(m_h_lang_list, lang.alias_name.c_str ());
  }

  CheckedListBox_EnableCheckAll(get_lang_list()->get_list_box(), BST_UNCHECKED);
  for (auto &token : make_delimiter_tokenizer(m_settings.get_active_multi_languages(), LR"(\|)").get_all_tokens()) {
    int index = -1;
    int i = 0;
    for (auto &lang : langs) {
      if (token == lang.orig_name) {
        index = i;
        break;
      }
      ++i;
    }
    if (index != -1)
      CheckedListBox_SetCheckState(m_h_lang_list, index, BST_CHECKED);
  }
}

void SelectMultipleLanguagesDialog::apply() {
  int count = ListBox_GetCount(m_h_lang_list);
  std::wstring buf;
  bool first = true;
  auto langs = m_speller_container.get_available_languages();
  for (int i = 0; i < count; i++) {
    if (CheckedListBox_GetCheckState(m_h_lang_list, i)) {
      if (!first) {
        buf += L"|";
      } else
        first = false;

      buf += langs[i].orig_name;
    }
  }
  {
    auto mut = m_settings.modify();
    mut->get_active_multi_languages() = buf;
    mut->get_active_language() = multiple_language_alias;
  }
}

INT_PTR SelectMultipleLanguagesDialog::run_dlg_proc(UINT message, WPARAM w_param, LPARAM /*l_param*/) {
  switch (message) {
  case WM_INITDIALOG: {
    m_h_lang_list = ::GetDlgItem(_hSelf, IDC_LANGLIST);
    update_list();
    return TRUE;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        apply();
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        update_list();
        display(false);
      }
      break;
    }
  }
  break;
  };
  return FALSE;
}
