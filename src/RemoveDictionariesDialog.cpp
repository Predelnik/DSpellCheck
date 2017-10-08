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

#include "RemoveDictionariesDialog.h"

#include "CheckedList/CheckedList.h"
#include "MainDef.h"
#include "HunspellInterface.h"
#include "CommonFunctions.h"
#include "Plugin.h"
#include "SpellChecker.h"

#include "resource.h"

void RemoveDictionariesDialog::DoDialog() {
  if (!isCreated()) {
    create(IDD_REMOVE_DICS);
  } else {
    goToCenter();
    display();
  }
  SetFocus(HLangList);
}

void RemoveDictionariesDialog::init(HINSTANCE hInst, HWND Parent) {
  return Window::init(hInst, Parent);
}

HWND RemoveDictionariesDialog::GetListBox() { return HLangList; }

void RemoveDictionariesDialog::RemoveSelected(SpellChecker *SpellCheckerInstance) {
  int Count = 0;
  bool Success = false;
  bool NeedSingleReset = false;
  bool NeedMultiReset = false;
  bool SingleTemp, MultiTemp;
  for (int i = 0; i < ListBox_GetCount(HLangList); i++) {
    if (CheckedListBox_GetCheckState(HLangList, i) == BST_CHECKED) {
      wchar_t FileName[MAX_PATH];
      for (int j = 0; j < 1 + SpellCheckerInstance->GetRemoveSystem() ? 1 : 0;
           j++) {
        *FileName = L'\0';
        wcscat(FileName,
               (j == 0) ? SpellCheckerInstance->GetHunspellPath()
                        : SpellCheckerInstance->GetHunspellAdditionalPath());
        wcscat(FileName, L"\\");
        wcscat(FileName, SpellCheckerInstance->GetLangByIndex(i));
        wcscat(FileName, L".aff");
        SetFileAttributes(FileName, FILE_ATTRIBUTE_NORMAL);
        Success = DeleteFile(FileName);
        wcsncpy(FileName + wcslen(FileName) - 4, L".dic", 4);
        SetFileAttributes(FileName, FILE_ATTRIBUTE_NORMAL);
        Success = Success && DeleteFile(FileName);
        if (SpellCheckerInstance->GetRemoveUserDics()) {
          wcsncpy(FileName + wcslen(FileName) - 4, L".usr", 4);
          SetFileAttributes(FileName, FILE_ATTRIBUTE_NORMAL);
          DeleteFile(FileName); // Success doesn't matter in that case, 'cause
                                // dictionary might not exist.
        }
        if (Success) {
          FileName[wcslen(FileName) - 4] = L'\0';
          SpellCheckerInstance->GetHunspellSpeller()->UpdateOnDicRemoval(
              FileName, SingleTemp, MultiTemp);
          NeedSingleReset |= SingleTemp;
          NeedMultiReset |= MultiTemp;
          Count++;
        }
      }
    }
  }
  for (int i = 0; i < ListBox_GetCount(HLangList); i++)
    CheckedListBox_SetCheckState(HLangList, i, BST_UNCHECKED);
  if (Count > 0) {
    SpellCheckerInstance->HunspellReinitSettings(true);
    SpellCheckerInstance->ReinitLanguageLists(true);
    SpellCheckerInstance->DoPluginMenuInclusion();
    SpellCheckerInstance->RecheckVisibleBothViews();
    /*
    if (NeedSingleReset)
    SpellCheckerInstance->
    */
    wchar_t Buf[DEFAULT_BUF_SIZE];
    swprintf(Buf, L"%d dictionary(ies) has(ve) been successfully removed",
             Count);
    MessageBox (_hParent, Buf, L"Dictionaries were removed",
                MB_OK | MB_ICONINFORMATION);
  }
}

void RemoveDictionariesDialog::UpdateOptions(SpellChecker *SpellCheckerInstance) {
  SpellCheckerInstance->SetRemoveUserDics(Button_GetCheck(HRemoveUserDics) ==
                                          BST_CHECKED);
  SpellCheckerInstance->SetRemoveSystem(Button_GetCheck(HRemoveSystem) ==
                                        BST_CHECKED);
}

void RemoveDictionariesDialog::SetCheckBoxes(bool RemoveUserDics, bool RemoveSystem) {
  Button_SetCheck(HRemoveUserDics,
                  RemoveUserDics ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(HRemoveSystem, RemoveSystem ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR RemoveDictionariesDialog::run_dlg_proc(UINT message, WPARAM wParam,
                                LPARAM /*lParam*/) {
  switch (message) {
  case WM_INITDIALOG: {
    HLangList = ::GetDlgItem(_hSelf, IDC_REMOVE_LANGLIST);
    HRemoveUserDics = ::GetDlgItem(_hSelf, IDC_REMOVE_USER_DICS);
    HRemoveSystem = ::GetDlgItem(_hSelf, IDC_REMOVE_SYSTEM);
    getSpellChecker ()->ReinitLanguageLists(true);
    getSpellChecker ()->updateRemoveDicsOptions();
    return true;
  } break;
  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case IDC_REMOVE_USER_DICS:
    case IDC_REMOVE_SYSTEM: {
      if (HIWORD(wParam) == BN_CLICKED) {
        getSpellChecker ()->updateFromRemoveDicsOptions();
      }
    } break;
    case IDOK:
      if (HIWORD(wParam) == BN_CLICKED) {
        RemoveSelected(getSpellChecker ());
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(wParam) == BN_CLICKED) {
        for (int i = 0; i < ListBox_GetCount(HLangList); i++)
          CheckedListBox_SetCheckState(HLangList, i, BST_UNCHECKED);
        display(false);
      }

      break;
    }
  } break;
  };
  return false;
}