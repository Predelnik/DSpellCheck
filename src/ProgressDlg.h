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

#include "StaticDialog\StaticDialog.h"

struct ProgressData;

class ProgressDlg : public StaticDialog {
public:
  ProgressDlg(DownloadDicsDlg &download_dics_dlg);
  ~ProgressDlg() override;

  INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) override; // NOLINT
  void init(HINSTANCE h_inst, HWND parent) override;
  void do_dialog();
  void set_top_message(const wchar_t* message);
  void set_marquee(bool animated);
  std::shared_ptr<ProgressData> get_progress_data () { return m_progress_data; }
  void update();

private:
  std::shared_ptr<ProgressData> m_progress_data;
  HWND m_h_desc_bottom = nullptr;
  HWND m_h_desc_top = nullptr;
  HWND m_h_progress_bar = nullptr;
  bool m_marquee = false;
  std::wstring m_top_message;
  DownloadDicsDlg &m_download_dics_dlg;
};
