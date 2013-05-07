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

#include "RemoveDics.h"

#include "Controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "HunspellInterface.h"
#include "CommonFunctions.h"
#include "Plugin.h"
#include "SpellChecker.h"

#include "resource.h"

void RemoveDics::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_REMOVE_DICS);
  }
  else
  {
    goToCenter ();
    display ();
  }
}

void RemoveDics::init (HINSTANCE hInst, HWND Parent, HWND LibComboArg)
{
  LibCombo = LibComboArg;
  return Window::init (hInst, Parent);
}

HWND RemoveDics::GetListBox ()
{
  return HLangList;
}

void RemoveDics::RemoveSelected (SpellChecker *SpellCheckerInstance)
{
  int Count = 0;
  BOOL Success = FALSE;
  for (int i = 0; i < ListBox_GetCount (HLangList); i++)
  {
    if (CheckedListBox_GetCheckState (HLangList, i) == BST_CHECKED)
    {
      TCHAR FileName[MAX_PATH];
      *FileName = _T ('\0');
      _tcscat (FileName, SpellCheckerInstance->GetHunspellPath ());
      _tcscat (FileName, _T ("\\"));
      _tcscat (FileName, SpellCheckerInstance->GetLangByIndex (i));
      _tcscat (FileName, _T (".aff"));
      SetFileAttributes (FileName, FILE_ATTRIBUTE_NORMAL);
      Success = DeleteFile (FileName);
      _tcsncpy (FileName + _tcslen (FileName) - 4, _T (".dic"), 4);
      SetFileAttributes (FileName, FILE_ATTRIBUTE_NORMAL);
      Success = Success && DeleteFile (FileName);
      if (Success)
        Count++;
    }
  }
  if (Count > 0)
  {
    SpellCheckerInstance->GetHunspellSpeller ()->SetDirectory (SpellCheckerInstance->GetHunspellPath ()); // Calling the update for Hunspell dictionary list
    if (ComboBox_GetCurSel (LibCombo) == 1)
      SpellCheckerInstance->ReinitLanguageLists (1);
    TCHAR Buf[DEFAULT_BUF_SIZE];
    _stprintf (Buf, _T ("%d dictionary(ies) has(ve) been successfully removed"), Count);
    for (int i = 0; i < ListBox_GetCount (HLangList); i++)
            CheckedListBox_SetCheckState (HLangList, i, BST_UNCHECKED);
    MessageBox (0, Buf, _T ("Dictionaries were removed"), MB_OK | MB_ICONINFORMATION);
  }
}

BOOL CALLBACK RemoveDics::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      HLangList = ::GetDlgItem (_hSelf, IDC_REMOVE_LANGLIST);
      return TRUE;
    }
    break;
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDOK:
        if (HIWORD (wParam) == BN_CLICKED) 
        {
          SendEvent (EID_REMOVE_SELECTED_DICS);
          display (false);
        }
        break;
      case IDCANCEL:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          for (int i = 0; i < ListBox_GetCount (HLangList); i++)
            CheckedListBox_SetCheckState (HLangList, i, BST_UNCHECKED);
          display (false);
        }

        break;
      }
    }
    break;
  };
  return FALSE;
}