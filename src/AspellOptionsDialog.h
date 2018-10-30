#pragma once
#include "StaticDialog/StaticDialog.h"

class SpellerContainer;

class AspellOptionsDialog : public StaticDialog {
public:
  explicit AspellOptionsDialog (HINSTANCE h_inst, HWND parent, const Settings &settings, const SpellerContainer &spellers);
  void do_dialog();
  void apply_choice();
  void update_controls();

protected:
  INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param,
    LPARAM l_param) override;
  HWND m_allow_run_together_cb = nullptr;
  HWND m_aspell_personal_path_le = nullptr;

protected:
  const Settings &m_settings;
  const SpellerContainer &m_spellers;
};
