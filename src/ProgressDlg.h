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
  ProgressDlg();
  ~ProgressDlg();

  INT_PTR run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
  void init(HINSTANCE hInst, HWND Parent) override;
  void DoDialog();
  void SetTopMessage(const wchar_t* Message);
  void SetMarquee(bool animated);
  std::shared_ptr<ProgressData> getProgressData () { return m_progressData; }
  void update();

private:
  std::shared_ptr<ProgressData> m_progressData;
  HWND HDescBottom;
  HWND HDescTop;
  HWND HProgressBar;
};
