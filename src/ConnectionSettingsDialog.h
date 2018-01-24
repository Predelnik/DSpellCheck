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
#include "CommonFunctions.h"
#include "utils/winapi.h"

class Settings;
class SpellChecker;
enum class ProxyType;

class ConnectionSettingsDialog : public StaticDialog {
public:
    explicit ConnectionSettingsDialog (Settings &settings, DownloadDicsDlg &download_dics_dlg);
  void do_dialog();
  void apply_choice();
  void update_controls(
  );

protected:
  INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param,
                              LPARAM l_param) override;
  void disable_controls();

protected:
  HWND m_port = nullptr;
  HWND m_user_name = nullptr;
  HWND m_host_name = nullptr;
  HWND m_passive_mode_cb = nullptr;
  HWND m_password = nullptr;
  HWND m_use_proxy = nullptr;
  HWND m_proxy_anonymous = nullptr;
  DownloadDicsDlg &m_download_dics_dlg;

  WinApi::EnumComboBox<ProxyType> m_proxy_type_cmb;
  Settings &m_settings;
};
