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

#include "Progress.h"

#include "Plugin.h"
#include "resource.h"

void Progress::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_DIALOGPROGRESS);
  }
  else
  {
    goToCenter ();
    display ();
  }
}

void Progress::init (HINSTANCE hInst, HWND Parent)
{
  return Window::init (hInst, Parent);
}

BOOL CALLBACK Progress::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      HDescTop = GetDlgItem (_hSelf, IDC_DESCTOP);
      HDescBottom = GetDlgItem (_hSelf, IDC_DESCBOTTOM);
      HProgressBar = GetDlgItem (_hSelf, IDC_PROGRESSBAR);
      SendMessage (HProgressBar, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0,100));
      return TRUE;
    }
    break;
  case WM_COMMAND:
    switch (LOWORD (wParam))
    {
    case IDC_STOP:
      if (HIWORD (wParam) == BN_CLICKED)
      {
        SendNetworkEvent (EID_CANCEL_DOWNLOAD);
      }
      break;
    };
    break;
  };
  return FALSE;
}

void Progress::SetProgress (int value)
{
  SendMessage (HProgressBar, PBM_SETPOS, value, 0);
}

void Progress::SetBottomMessage (TCHAR *Message)
{
  Static_SetText (HDescBottom, Message);
}

void Progress::SetTopMessage (TCHAR *Message)
{
  Static_SetText (HDescTop, Message);
}

Progress::Progress(void)
{
}

Progress::~Progress(void)
{
}