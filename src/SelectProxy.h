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

class SelectProxy : public StaticDialog
{
public:
  void init (HINSTANCE hInst, HWND Parent);
  void DoDialog ();
  void ApplyChoice (SpellChecker *SpellCheckerInstance);
  void SetOptions (BOOL UseProxy, TCHAR *HostName, TCHAR *UserName, TCHAR *Password, int Port, BOOL ProxyAnonymous, int ProxyType);
protected:
  __override virtual INT_PTR run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
  void DisableControls ();
protected:
  HWND HPort;
  HWND HUserName;
  HWND HHostName;
  HWND HPassword;
  HWND HUseProxy;
  HWND HProxyAnonymous;
  HWND HProxyType;
};
