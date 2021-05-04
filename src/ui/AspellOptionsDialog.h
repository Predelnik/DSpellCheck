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

#pragma once
#include "ui/StaticDialog.h"

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
