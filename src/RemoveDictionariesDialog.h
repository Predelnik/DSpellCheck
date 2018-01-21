// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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

#include "StaticDialog/StaticDialog.h"

class Settings;
class SpellChecker;
class SpellerContainer;

class RemoveDictionariesDialog : public StaticDialog {
public:
  RemoveDictionariesDialog(HINSTANCE h_inst, HWND parent,
                           const Settings &settings,
                           SpellerContainer &speller_container);
  void do_dialog();
  void update_list();
  void remove_selected();
  HWND get_list_box();
  void update_options();
  void update_controls();

protected:
  INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param,
                              LPARAM l_param) override;

protected:
  HWND m_lang_list = nullptr;
  HWND m_remove_user_dics = nullptr;
  HWND m_remove_system = nullptr;
  const Settings &m_settings;
  SpellerContainer &m_speller_container;
};