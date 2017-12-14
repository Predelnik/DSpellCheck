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

#pragma once

#include "PluginInterface.h"
#include "StaticDialog/StaticDialog.h"

class SpellChecker;
class NppInterface;
class Settings;

class SuggestionsButton : public StaticDialog {
public:
  void do_dialog();
  HMENU get_popup_menu();
  int get_result();
  SuggestionsButton(HINSTANCE h_inst, HWND parent, NppInterface &npp,
                    SpellChecker &spell_checker,
                    const Settings &settings);
  void show_suggestion_menu();
  void on_settings_changed();
  void set_transparency();

protected:
  INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param,
                              LPARAM l_param) override;

private:
  int m_menu_result;
  bool m_state_pressed;
  bool m_state_hovered;
  bool m_state_menu;
  HMENU m_popup_menu;
  NppData m_npp_data_instance;
  NppInterface &m_npp;
  SpellChecker &m_spell_checker;
  const Settings &m_settings;
};
