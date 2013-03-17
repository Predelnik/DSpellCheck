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

#include <CommCtrl.h>

#include "AboutDlg.h"
#include "resource.h"

void AboutDlg::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_ABOUT);
    goToCenter ();
  }
  display ();
}

void AboutDlg::init (HINSTANCE hInst, HWND Parent)
{
  return Window::init (hInst, Parent);
}

BOOL CALLBACK AboutDlg::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    return TRUE;
  case WM_NOTIFY:
    {
      switch (((LPNMHDR)lParam)->code)
      {
      case NM_CLICK:          // Fall through to the next case.
      case NM_RETURN:
        {
          PNMLINK pNMLink = (PNMLINK)lParam;
          LITEM   item    = pNMLink->item;

          if ((item.iLink == 0))
          {
            ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
          }

          return TRUE;
        }
      }
      return FALSE;
    }
	break;
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDOK:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          display (false);
          return TRUE;
        }
      }
    }
	break;
  }
  return FALSE;
}