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
#include "CheckedList/CheckedList.h"
#include "MainDef.h"
#include "CommonFunctions.h"
#include "SpellChecker.h"
#include "Plugin.h"

#include "resource.h"

void LangList::DoDialog() {
  if (!isCreated()) {
    create(IDD_CHOOSE_MULTIPLE_LANGUAGES);
  }
  goToCenter();
  display();
  SetFocus(HLangList);
}

void LangList::init(HINSTANCE hInst, HWND Parent) {
  return Window::init(hInst, Parent);
}

HWND LangList::GetListBox() { return HLangList; }

void LangList::ApplyChoice(SpellChecker *SpellCheckerInstance) {
  int count = ListBox_GetCount(HLangList);
  std::wstring buf;
  bool first = true;
  for (int i = 0; i < count; i++) {
    if (CheckedListBox_GetCheckState(HLangList, i)) {
      if (!first)
      {
        buf += L"|";
      }
      else
          first = false;

      buf += SpellCheckerInstance->GetLangByIndex(i);
    }
  }
  auto convertedBuf = toUtf8String(buf.c_str ());
  if (SpellCheckerInstance->GetLibMode() == 1) {
    SpellCheckerInstance->SetHunspellMultipleLanguages(convertedBuf.c_str ());
    SpellCheckerInstance->HunspellReinitSettings(false);
  } else {
    SpellCheckerInstance->SetAspellMultipleLanguages(convertedBuf.c_str ());
    SpellCheckerInstance->AspellReinitSettings();
  }
  SpellCheckerInstance->RecheckVisible();
}

INT_PTR LangList::run_dlgProc(UINT message, WPARAM wParam, LPARAM) {
  switch (message) {
  case WM_INITDIALOG: {
    HLangList = ::GetDlgItem(_hSelf, IDC_LANGLIST);
    getSpellChecker ()->ReinitLanguageLists(true);
    return true;
  }
  case WM_COMMAND: {
    switch (LOWORD(wParam)) {
    case IDOK:
      if (HIWORD(wParam) == BN_CLICKED) {
        getSpellChecker ()->applyMultiLangSettings ();
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(wParam) == BN_CLICKED) {
        getSpellChecker ()->ReinitLanguageLists(true); // Reset all settings
        display(false);
      }
      break;
    }
  } break;
  };
  return false;
}