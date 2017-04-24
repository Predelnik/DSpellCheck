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

#include "LangListDialog.h"
#include "Controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "CommonFunctions.h"
#include "SpellChecker.h"
#include "Plugin.h"

#include "resource.h"
#include "Settings.h"

void LangListDialog::DoDialog() {
  if (!isCreated()) {
    create(IDD_CHOOSE_MULTIPLE_LANGUAGES);
  }
  goToCenter();
  display();
  SetFocus(HLangList);
}

void LangListDialog::init(HINSTANCE hInst, HWND Parent) {
  return Window::init(hInst, Parent);
}

HWND LangListDialog::GetListBox() { return HLangList; }

void LangListDialog::ApplyChoice(SpellChecker *spellCheckerInstance) {
  int count = ListBox_GetCount(HLangList);
  TCHAR Buf[DEFAULT_BUF_SIZE];
  TCHAR *ItemBuf = 0;
  BOOL FirstOne = TRUE;
  Buf[0] = _T('\0');
  for (int i = 0; i < count; i++) {
    if (CheckedListBox_GetCheckState(HLangList, i)) {
      setString(ItemBuf, spellCheckerInstance->GetLangByIndex(i));
      if (!FirstOne) _tcscat(Buf, _T ("|"));

      _tcscat_s(Buf, ItemBuf);
      FirstOne = FALSE;
    }
  }
  auto settingsCopy =
      std::make_unique<SettingsData>(*spellCheckerInstance->getSettings());
  settingsCopy->spellerSettings[settingsCopy->activeSpellerType]
      .activeMultiLanguage = Buf;
  PostMessageToMainThread(TM_SETTINGS_CHANGED,
                          reinterpret_cast<WPARAM>(settingsCopy.release()));

  CLEAN_AND_ZERO_ARR(ItemBuf);
}

BOOL CALLBACK LangListDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM) {
  switch (message) {
    case WM_INITDIALOG: {
      HLangList = ::GetDlgItem(_hSelf, IDC_LANGLIST);
      update ();
      return TRUE;
    } break;
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDOK:
          if (HIWORD(wParam) == BN_CLICKED) {
            SendEvent(EID_APPLY_MULTI_LANG_SETTINGS);
            display(false);
          }
          break;
        case IDCANCEL:
          if (HIWORD(wParam) == BN_CLICKED) {
            update ();
            display(false);
          }
          break;
      }
    } break;
  };
  return FALSE;
}

void LangListDialog::update()
{
  if (!isCreated ())
    return;

  auto status = getSpellChecker()->getStatus();
  ListBox_ResetContent(HLangList);
  for (auto &lang : status->languageList) ListBox_AddString(HLangList, lang.aliasName.c_str ());

  auto settingsCopy = *getSpellChecker()->getSettings();
  wchar_t *multiLangCopy = nullptr;
  wchar_t *context = nullptr;
  setString (multiLangCopy, settingsCopy.spellerSettings[SpellerType::hunspell].activeMultiLanguage.data ());
  int index = 0;
  auto token = _tcstok_s(multiLangCopy, _T ("|"), &context);
  while (token) {
    index = -1;
    for (int i = 0; i < static_cast<int> (status->languageList.size ()); ++i) {
      if (status->languageList[i].originalName == token) {
        index = i;
        break;
      }
    }
    if (index != -1)
      CheckedListBox_SetCheckState(HLangList, index, BST_CHECKED);
    token = _tcstok_s(NULL, _T ("|"), &context);
  }
  CLEAN_AND_ZERO_ARR (multiLangCopy);
}
