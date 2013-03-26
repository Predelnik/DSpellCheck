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

#include "LangList.h"
#include "Controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "CommonFunctions.h"
#include "SpellChecker.h"
#include "Plugin.h"

#include "resource.h"

void LangList::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_CHOOSE_MULTIPLE_LANGUAGES);
  }
  else
  {
    goToCenter ();
    display ();
  }
}

void LangList::init (HINSTANCE hInst, HWND Parent, HWND HLibArg)
{
  HLib = HLibArg;
  return Window::init (hInst, Parent);
}

HWND LangList::GetListBox ()
{
  return HLangList;
}

void LangList::ApplyChoice (SpellChecker *SpellCheckerInstance)
{
  int count = ListBox_GetCount (HLangList);
  TCHAR Buf[DEFAULT_BUF_SIZE];
  TCHAR *ItemBuf = 0;
  int ItemBufLen = 0;
  char *ConvertedBuf = 0;
  BOOL FirstOne = TRUE;
  Buf[0] = _T ('\0');
  for (int i = 0; i < count; i++)
  {
    if (CheckedListBox_GetCheckState (HLangList, i))
    {
      ItemBufLen = ListBox_GetTextLen (HLangList, i);
      ItemBuf = new TCHAR[ItemBufLen + 1];
      ListBox_GetText (HLangList, i, ItemBuf);
      if (!FirstOne)
        _tcscat (Buf, _T ("|"));

      _tcscat_s (Buf, ItemBuf);
      FirstOne = FALSE;
      CLEAN_AND_ZERO_ARR (ItemBuf);
    }
  }
  SetStringDUtf8 (ConvertedBuf, Buf);
  if (ComboBox_GetCurSel (HLib))
    SpellCheckerInstance->SetHunspellMultipleLanguages (ConvertedBuf);
  else
    SpellCheckerInstance->SetAspellMultipleLanguages (ConvertedBuf);
  CLEAN_AND_ZERO_ARR (ConvertedBuf);
}

BOOL CALLBACK LangList::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      HLangList = ::GetDlgItem (_hSelf, IDC_LANGLIST);
      return TRUE;
    }
    break;
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDOK:
        SendEvent (EID_APPLY_MULTI_LANG_SETTINGS);
      case IDCANCEL:
        if (HIWORD (wParam) == BN_CLICKED)
          display (false);

        break;
      }
    }
    break;
  };
  return FALSE;
}
