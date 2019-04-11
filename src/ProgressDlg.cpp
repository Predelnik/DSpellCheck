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

#include "ProgressDlg.h"

#include "DownloadDicsDlg.h"
#include "ProgressData.h"
#include "resource.h"

void ProgressDlg::do_dialog() {
  if (!isCreated()) {
    create(IDD_DIALOGPROGRESS);
  } else {
    goToCenter();
    display();
  }
}

void ProgressDlg::init(HINSTANCE h_inst, HWND parent) {
  return Window::init(h_inst, parent);
}

namespace {
  constexpr auto ui_update_timer_id = 0;
}

INT_PTR ProgressDlg::run_dlg_proc(UINT message, WPARAM w_param,
                                  LPARAM /*lParam*/) {
  switch (message) {
    case WM_SHOWWINDOW: {
       constexpr auto timer_resolution = 100;
       if (w_param == TRUE) {
         SetTimer (_hSelf, ui_update_timer_id, timer_resolution, nullptr);
       } else {
         KillTimer (_hSelf, ui_update_timer_id);
       }
    }
  case WM_TIMER: {
        switch (w_param)
        {
          case ui_update_timer_id:
            update ();
            return 0;
          default:
            break;
        }
      }
  case WM_INITDIALOG: {
    m_h_desc_top = GetDlgItem(_hSelf, IDC_DESCTOP);
    m_h_desc_bottom = GetDlgItem(_hSelf, IDC_DESCBOTTOM);
    m_h_progress_bar = GetDlgItem(_hSelf, IDC_PROGRESSBAR);
    SendMessage(m_h_progress_bar, PBM_SETRANGE, 0,
                static_cast<LPARAM>(MAKELONG(0, 100)));
    return TRUE;
  }
  case WM_COMMAND:
    switch (LOWORD(w_param)) {
    case IDC_STOP:
      if (HIWORD(w_param) == BN_CLICKED) {
        m_download_dics_dlg.set_cancel_pressed(true);
      }
      break;
    };
    break;
  };
  return FALSE;
}

void ProgressDlg::set_marquee(bool animated) {
  if (m_marquee == animated)
    return;
  m_marquee = animated;
  DWORD dw_style = ::GetWindowLong(m_h_progress_bar, GWL_STYLE);
  if (animated)
    dw_style = dw_style | PBS_MARQUEE;
  else
    dw_style = dw_style & (~PBS_MARQUEE);

  ::SetWindowLong(m_h_progress_bar, GWL_STYLE, dw_style);
  SendMessage(m_h_progress_bar, PBM_SETMARQUEE, static_cast<int>(animated), 0);
}

void ProgressDlg::update() {
  SendMessage(m_h_progress_bar, PBM_SETPOS, m_progress_data->get_progress(), 0);
  Static_SetText(m_h_desc_bottom, m_progress_data->get_status().c_str());
  set_marquee(m_progress_data->get_marquee());
  Static_SetText(m_h_desc_top, m_top_message.c_str());
}

void ProgressDlg::set_top_message(const wchar_t *message) {
  m_top_message = message;
}

ProgressDlg::ProgressDlg(DownloadDicsDlg &download_dics_dlg)
    : m_progress_data(std::make_shared<ProgressData>()),
      m_download_dics_dlg(download_dics_dlg) {}

ProgressDlg::~ProgressDlg()
{
  KillTimer (_hSelf, ui_update_timer_id);
}
