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

#include "ProgressDlg.h"

#include "Plugin.h"
#include "resource.h"
#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "ProgressData.h"

void ProgressDlg::DoDialog() {
  if (!isCreated()) {
    create(IDD_DIALOGPROGRESS);
  } else {
    goToCenter();
    display();
  }
}

void ProgressDlg::init(HINSTANCE hInst, HWND Parent) {
  return Window::init(hInst, Parent);
}

INT_PTR ProgressDlg::run_dlg_proc(UINT message, WPARAM wParam, LPARAM /*lParam*/) {
  switch (message) {
  case WM_INITDIALOG: {
    HDescTop = GetDlgItem(_hSelf, IDC_DESCTOP);
    HDescBottom = GetDlgItem(_hSelf, IDC_DESCBOTTOM);
    HProgressBar = GetDlgItem(_hSelf, IDC_PROGRESSBAR);
    SendMessage(HProgressBar, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0, 100));
    return true;
  } break;
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case IDC_STOP:
      if (HIWORD(wParam) == BN_CLICKED) {
        GetDownloadDics()->SetCancelPressed(true);
      }
      break;
    };
    break;
  };
  return false;
}


void ProgressDlg::SetMarquee(bool animated) {
  if (m_marquee == animated)
     return;
  m_marquee = animated;
  DWORD dwStyle = ::GetWindowLong(HProgressBar, GWL_STYLE);
  if (animated)
    dwStyle = dwStyle | PBS_MARQUEE;
  else
    dwStyle = dwStyle & (~PBS_MARQUEE);

  ::SetWindowLong(HProgressBar, GWL_STYLE, dwStyle);
  SendMessage(HProgressBar, PBM_SETMARQUEE, (int)animated, 0);
}

void ProgressDlg::update() {
    SendMessage(HProgressBar, PBM_SETPOS, m_progressData->getProgress(), 0);
    Static_SetText(HDescBottom, m_progressData->getStatus().c_str ());
    SetMarquee (m_progressData->getMarquee());
    Static_SetText (HDescTop, m_topMessage.c_str ());
}

void ProgressDlg::SetTopMessage(const wchar_t* Message) {
  m_topMessage = Message;
}

ProgressDlg::ProgressDlg() : m_progressData(std::make_shared<ProgressData> ()) {}

ProgressDlg::~ProgressDlg() {}