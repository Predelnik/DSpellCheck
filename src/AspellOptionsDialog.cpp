#include "AspellOptionsDialog.h"
#include "Settings.h"
#include "utils/WinApi.h"

AspellOptionsDialog::AspellOptionsDialog(HINSTANCE h_inst, HWND parent, Settings& settings) : m_settings (settings) {
  Window::init (h_inst, parent);
}

void AspellOptionsDialog::do_dialog() {
  if (!isCreated()) {
    create(IDD_ASPELL_OPTIONS);
  }
  goToCenter();
  display();
}

void AspellOptionsDialog::apply_choice() {
  auto s = m_settings.modify();
  s->aspell_allow_run_together_words = Button_GetCheck (m_allow_run_together_cb) == BST_CHECKED;
  s->aspell_personal_dictionary_path = get_edit_text (m_aspell_personal_path_le);
}

void AspellOptionsDialog::update_controls() {
  Button_SetCheck(m_allow_run_together_cb, m_settings.aspell_allow_run_together_words ? BST_CHECKED : BST_UNCHECKED);
  Edit_SetText(m_aspell_personal_path_le, m_settings.aspell_personal_dictionary_path.c_str ());
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
        auto cur_path = get_edit_text(m_aspell_personal_path_le);
        auto path = WinApi::browse_for_directory (_hSelf, cur_path.data ());
        if (path)
          Edit_SetText(m_aspell_personal_path_le, path->data ());
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
