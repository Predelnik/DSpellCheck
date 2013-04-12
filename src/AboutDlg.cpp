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
#include "CommonFunctions.h"
#include "MainDef.h"
#include "Plugin.h"
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

bool GetProductAndVersion (TCHAR *&strProductVersion)
{
  // get the filename of the executable containing the version resource
  TCHAR szFilename[MAX_PATH + 1] = {0};
  if (GetModuleFileName ((HMODULE) getHModule (), szFilename, MAX_PATH) == 0)
  {
    return false;
  }

  // allocate a block of memory for the version info
  DWORD dummy;
  DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
  if (dwSize == 0)
  {
    return false;
  }
  std::vector<BYTE> data(dwSize);

  // load the version info
  if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0]))
  {
    return false;
  }

  // get the name and version strings
  LPVOID pvProductName = NULL;
  unsigned int iProductNameLen = 0;
  LPVOID pvProductVersion = NULL;
  unsigned int iProductVersionLen = 0;

  UINT                uiVerLen = 0;
  VS_FIXEDFILEINFO*   pFixedInfo = 0;     // pointer to fixed file info structure
  // get the fixed file info (language-independent)
  if(VerQueryValue(&data[0], TEXT("\\"), (void**)&pFixedInfo, (UINT *)&uiVerLen) == 0)
  {
    return false;
  }

  CLEAN_AND_ZERO_ARR (strProductVersion);
  strProductVersion = new TCHAR[DEFAULT_BUF_SIZE];
  _stprintf (strProductVersion, _T ("Version: %u.%u.%u.%u"),     HIWORD (pFixedInfo->dwProductVersionMS),
    LOWORD (pFixedInfo->dwProductVersionMS),
    HIWORD (pFixedInfo->dwProductVersionLS),
    LOWORD (pFixedInfo->dwProductVersionLS));

  return true;
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
    {
      TCHAR *Ver = 0;
      GetProductAndVersion (Ver);
      Static_SetText (::GetDlgItem(_hSelf, IDC_VERSION), Ver);
      CLEAN_AND_ZERO_ARR (Ver);
    }
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

          ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);

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
      case IDCANCEL:
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