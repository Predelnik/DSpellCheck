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

#include "RemoveDicsDialog.h"

#include "Controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "HunspellController.h"
#include "CommonFunctions.h"
#include "Plugin.h"
#include "SpellChecker.h"

#include "Settings.h"
#include "resource.h"

void RemoveDicsDialog::DoDialog ()
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
  SetFocus (HLangList);
}

void RemoveDicsDialog::init (HINSTANCE hInst, HWND Parent)
{
  return Window::init (hInst, Parent);
}

HWND RemoveDicsDialog::GetListBox ()
{
  return HLangList;
}

void RemoveDicsDialog::RemoveSelected (SpellChecker *SpellCheckerInstance)
{
  int Count = 0;
  BOOL Success = FALSE;
  BOOL NeedSingleReset = FALSE;
  BOOL NeedMultiReset = FALSE;
  BOOL SingleTemp, MultiTemp;
  auto settings = SpellCheckerInstance->getSettings ();
  for (int i = 0; i < ListBox_GetCount (HLangList); i++)
  {
    if (CheckedListBox_GetCheckState (HLangList, i) == BST_CHECKED)
    {
      TCHAR FileName[MAX_PATH];
      for (int j = 0; j < 1 + SpellCheckerInstance->GetRemoveSystem () ? 1 : 0; j++)
      {
        *FileName = _T ('\0');

        _tcscat (FileName, j == 0 ? settings->spellerSettings[SpellerType::hunspell].path.data () : settings->spellerSettings[SpellerType::hunspell].additionalPath.data ());
        _tcscat (FileName, _T ("\\"));
        _tcscat (FileName, SpellCheckerInstance->GetLangByIndex (i));
        _tcscat (FileName, _T (".aff"));
        SetFileAttributes (FileName, FILE_ATTRIBUTE_NORMAL);
        Success = DeleteFile (FileName);
        _tcsncpy (FileName + _tcslen (FileName) - 4, _T (".dic"), 4);
        SetFileAttributes (FileName, FILE_ATTRIBUTE_NORMAL);
        Success = Success && DeleteFile (FileName);
        if (SpellCheckerInstance->GetRemoveUserDics ())
        {
          _tcsncpy (FileName + _tcslen (FileName) - 4, _T (".usr"), 4);
          SetFileAttributes (FileName, FILE_ATTRIBUTE_NORMAL);
          DeleteFile (FileName); // Success doesn't matter in that case, 'cause dictionary might not exist.
        }
        if (Success)
        {
          FileName[_tcslen (FileName) - 4] = _T ('\0');
          SpellCheckerInstance->GetHunspellSpeller ()->UpdateOnDicRemoval (FileName, SingleTemp, MultiTemp);
          NeedSingleReset |= SingleTemp;
          NeedMultiReset |= MultiTemp;
          Count++;
        }
      }
    }
  }
  for (int i = 0; i < ListBox_GetCount (HLangList); i++)
    CheckedListBox_SetCheckState (HLangList, i, BST_UNCHECKED);
  if (Count > 0)
  {
    SpellCheckerInstance->onHunspellDictionariesChange ();
    TCHAR Buf[DEFAULT_BUF_SIZE];
    _stprintf (Buf, _T ("%d dictionary(ies) has(ve) been successfully removed"), Count);
    MessageBoxInfo MsgBox (_hParent, Buf, _T ("Dictionaries were removed"), MB_OK | MB_ICONINFORMATION);
    SendMessage (_hParent, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
  }
}

void RemoveDicsDialog::UpdateOptions (SpellChecker *SpellCheckerInstance)
{
  SpellCheckerInstance->SetRemoveUserDics (Button_GetCheck (HRemoveUserDics) == BST_CHECKED);
  SpellCheckerInstance->SetRemoveSystem (Button_GetCheck (HRemoveSystem) == BST_CHECKED);
}

void RemoveDicsDialog::SetCheckBoxes (BOOL RemoveUserDics, BOOL RemoveSystem)
{
  Button_SetCheck (HRemoveUserDics, RemoveUserDics ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HRemoveSystem, RemoveSystem ? BST_CHECKED : BST_UNCHECKED);
}

void RemoveDicsDialog::update()
{
  if (!isCreated())
    return;

  auto status = getSpellChecker()->getStatus ();
  ListBox_ResetContent (HLangList);
  for (auto &lang : status->languageList)
    {
      std::wstring name = lang.AliasName;
      name += (lang.systemOnly ? L" [!For All Users]" : L"");
      ListBox_AddString (HLangList, name.data ());
    }
}

BOOL CALLBACK RemoveDicsDialog::run_dlgProc (UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      HLangList = ::GetDlgItem (_hSelf, IDC_REMOVE_LANGLIST);
      HRemoveUserDics = ::GetDlgItem (_hSelf, IDC_REMOVE_USER_DICS);
      HRemoveSystem = ::GetDlgItem (_hSelf, IDC_REMOVE_SYSTEM);
      SendEvent (EID_UPDATE_LANG_LISTS);
      SendEvent (EID_UPDATE_REMOVE_DICS_OPTIONS);
      update ();
      return TRUE;
    }
    break;
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDC_REMOVE_USER_DICS:
      case IDC_REMOVE_SYSTEM:
        {
          if (HIWORD (wParam) == BN_CLICKED)
          {
            SendEvent (EID_UPDATE_FROM_REMOVE_DICS_OPTIONS);
          }
        }
        break;
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