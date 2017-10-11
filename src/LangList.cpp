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

#include "LangList.h"
#include "CheckedList/CheckedList.h"
#include "CommonFunctions.h"
#include "SpellChecker.h"
#include "Plugin.h"

#include "resource.h"

void LangList::do_dialog() {
  if (!isCreated()) {
    create(IDD_CHOOSE_MULTIPLE_LANGUAGES);
  }
  goToCenter();
  display();
  SetFocus(m_h_lang_list);
}

void LangList::init(HINSTANCE h_inst, HWND parent) {
  return Window::init(h_inst, parent);
}

HWND LangList::get_list_box() { return m_h_lang_list; }

void LangList::apply_choice(SpellChecker *spell_checker_instance) {
  int count = ListBox_GetCount(m_h_lang_list);
  std::wstring buf;
  bool first = true;
  for (int i = 0; i < count; i++) {
    if (CheckedListBox_GetCheckState(m_h_lang_list, i)) {
      if (!first)
      {
        buf += L"|";
      }
      else
          first = false;

      buf += spell_checker_instance->get_lang_by_index(i);
    }
  }
  auto converted_buf = to_utf8_string(buf.c_str ());
  if (spell_checker_instance->get_lib_mode() == 1) {
    spell_checker_instance->set_hunspell_multiple_languages(converted_buf.c_str ());
    spell_checker_instance->hunspell_reinit_settings(false);
  } else {
    spell_checker_instance->set_aspell_multiple_languages(converted_buf.c_str ());
    spell_checker_instance->aspell_reinit_settings();
  }
  spell_checker_instance->recheck_visible();
}

INT_PTR LangList::run_dlg_proc(UINT message, WPARAM w_param, LPARAM) {
  switch (message) {
  case WM_INITDIALOG: {
    m_h_lang_list = ::GetDlgItem(_hSelf, IDC_LANGLIST);
    get_spell_checker ()->reinit_language_lists(true);
    return true;
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        get_spell_checker ()->apply_multi_lang_settings ();
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        get_spell_checker ()->reinit_language_lists(true); // Reset all settings
        display(false);
      }
      break;
    }
  } break;
  };
  return false;
}