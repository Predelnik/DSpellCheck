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
#include "StaticDialog/StaticDialog.h"

class SpellChecker;

class SelectProxyDialog : public StaticDialog {
public:
  void DoDialog();
  void ApplyChoice(SpellChecker *SpellCheckerInstance);
  void SetOptions(bool UseProxy, wchar_t *HostName, wchar_t *UserName,
                  wchar_t *Password, int Port, bool ProxyAnonymous,
                  int ProxyType);

protected:
  virtual INT_PTR WINAPI run_dlgProc(UINT message, WPARAM wParam,
                              LPARAM lParam) override;
  void DisableControls();

protected:
  HWND HPort = nullptr;
  HWND HUserName = nullptr;
  HWND HHostName = nullptr;
  HWND HPassword = nullptr;
  HWND HUseProxy = nullptr;
  HWND HProxyAnonymous = nullptr;
  HWND HProxyType = nullptr;
};
