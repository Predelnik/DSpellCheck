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

void RemoveDicsDialog::SetCheckBoxes (BOOL RemoveUserDics, BOOL RemoveSystem)
{
  Button_SetCheck (HRemoveUserDics, RemoveUserDics ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HRemoveSystem, RemoveSystem ? BST_CHECKED : BST_UNCHECKED);
}

void RemoveDicsDialog::updateOnNewConfiguration()
{
  if (!isCreated())
    return;

  auto status = getSpellChecker()->getStatus ();
  auto settings = getSpellChecker()->getSettings ();
  ListBox_ResetContent (HLangList);
  for (auto &lang : status->languageList)
    ListBox_AddString (HLangList, lang.getLanguageName (*settings, true).data ());

  Button_SetCheck (HRemoveUserDics, settings->removeCorrespondingUserDictionaries ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HRemoveSystem, settings->removeDictionariesInstalledForAllUsers ? BST_CHECKED : BST_UNCHECKED);
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
      updateOnNewConfiguration ();
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
          apply ();
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

void RemoveDicsDialog::apply ()
{
  auto status = getSpellChecker()->getStatus();
  auto settings = getSpellChecker ()->getSettings ();
  auto &langList = status->languageList;
  auto show_error = [this](){ ::MessageBox(_hParent, L"Some error happened during dictionaries removal, please try again.", _T("Error happened"), MB_OK); };
  auto info = std::make_unique<RemoveDictionariesInfo> ();
  if (ListBox_GetCount (HLangList) != langList.size ()) { show_error (); return; }
  for (int i = 0; i < ListBox_GetCount (HLangList); ++i)
    {
      if (getListboxText (HLangList, i) != langList[i].getLanguageName (*settings, true))
        {
          show_error ();
          return;
        }
      if (CheckedListBox_GetCheckState (HLangList, i) == BST_CHECKED)
        info->dictionaryNames.push_back(langList[i].originalName);
    }
  info->newSettings = std::make_unique<SettingsData> (*settings);
  info->newSettings->removeCorrespondingUserDictionaries = (Button_GetCheck (HRemoveUserDics) == BST_CHECKED);
  info->newSettings->removeDictionariesInstalledForAllUsers = (Button_GetCheck (HRemoveSystem) == BST_CHECKED);
  PostMessageToMainThread (TM_REMOVE_DICTIONARIES_APPLY,
    reinterpret_cast<WPARAM>(info.release ()));
}
