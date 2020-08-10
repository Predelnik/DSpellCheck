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

#include "AspellOptionsDialog.h"
#include "Settings.h"
#include "SpellerContainer.h"
#include "utils/WinApi.h"

AspellOptionsDialog::AspellOptionsDialog(HINSTANCE h_inst, HWND parent, const Settings &settings, const SpellerContainer &spellers)
    : m_settings(settings), m_spellers(spellers) {
  Window::init(h_inst, parent);
}

void AspellOptionsDialog::do_dialog() {
  if (!isCreated()) {
    create(IDD_ASPELL_OPTIONS);
  }
  update_controls();
  goToCenter();
  display();
}

void AspellOptionsDialog::apply_choice() {
  auto s = m_settings.modify();
  s->data.aspell_allow_run_together_words = Button_GetCheck(m_allow_run_together_cb) == BST_CHECKED;
  s->data.aspell_personal_dictionary_path = WinApi::get_edit_text(m_aspell_personal_path_le);
  if (s->data.aspell_personal_dictionary_path == m_spellers.get_aspell_default_personal_dictionary_path())
    s->data.aspell_personal_dictionary_path = L"";
}

void AspellOptionsDialog::update_controls() {
  Button_SetCheck(m_allow_run_together_cb, m_settings.data.aspell_allow_run_together_words ? BST_CHECKED : BST_UNCHECKED);
  Edit_SetText(m_aspell_personal_path_le, m_settings.data.aspell_personal_dictionary_path.empty() ? m_spellers.get_aspell_default_personal_dictionary_path().c_str()
                                                                                             : m_settings.data.aspell_personal_dictionary_path.c_str());
}

INT_PTR __stdcall AspellOptionsDialog::run_dlg_proc(UINT message, WPARAM w_param, LPARAM /*l_param*/) {
  switch (message) {
  case WM_INITDIALOG: {
    m_allow_run_together_cb = GetDlgItem(_hSelf, IDC_ASPELL_RUNTOGETHER_CB);
    m_aspell_personal_path_le = GetDlgItem(_hSelf, IDC_ASPELL_PERSONAL_PATH_LE);
  }
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDC_BROWSEPDICTIONARYPATH:
      if (HIWORD(w_param) == BN_CLICKED) {
        auto cur_path = WinApi::get_edit_text(m_aspell_personal_path_le);
        auto path = WinApi::browse_for_directory(_hSelf, cur_path.data());
        if (path)
          Edit_SetText(m_aspell_personal_path_le, path->data());
      }
      break;
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        apply_choice();
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        update_controls(); // Reset all settings
        display(false);
      }
    default:
      break;
    }
  }
  default:
    break;
  }
  return FALSE;
}
