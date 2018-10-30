#pragma once
#include "StaticDialog/StaticDialog.h"

class AspellOptionsDialog : public StaticDialog {
public:
  explicit AspellOptionsDialog (HINSTANCE h_inst, HWND parent, Settings &settings);
  void do_dialog();
  void apply_choice();
  void update_controls();

protected:
  INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param,
    LPARAM l_param) override;
  HWND m_allow_run_together_cb;
  HWND m_aspell_personal_path_le;

protected:
  Settings &m_settings;
};
