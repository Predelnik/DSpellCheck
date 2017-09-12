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

#include <shlwapi.h>

#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "LangList.h"
#include "Plugin.h"
#include "RemoveDics.h"
#include "MainDef.h"
#include "SpellChecker.h"

#ifdef VLD_BUILD
#include <vld.h>
#endif // VLD_BUILD

extern FuncItem funcItem[nbFunc];
extern NppData nppData;
extern bool doCloseTag;
HANDLE HModule;
int RecheckDelay;
std::vector<std::pair<long, long>> CheckQueue;
UINT_PTR uiTimer = 0u;
UINT_PTR recheckTimer = 0u;
bool recheckDone = true;
WNDPROC wndProcNotepad = NULL;
bool restylingCausedRecheckWasDone =
    false; // Hack to avoid eternal cycle in case of scintilla bug
bool firstRestyle = true; // hack to successfully avoid checking hyperlinks when they appear on program start

int GetRecheckDelay() { return RecheckDelay; }

void SetRecheckDelay(int Value, int WriteToIni) {
  if (Value < 0)
    Value = 0;

  if (Value > 20000)
    Value = 20000;

  if (WriteToIni) {
    std::pair<wchar_t *, DWORD> x;
    x.first = 0;
    SetString(x.first, L"Recheck_Delay");
    x.second = MAKELPARAM(Value, 500);
    getSpellChecker()->WriteSetting(x);
  }
  RecheckDelay = Value;
}

bool APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID) {
  HModule = hModule;

  switch (reasonForCall) {
  case DLL_PROCESS_ATTACH:
    pluginInit(hModule);
    break;

  case DLL_PROCESS_DETACH:
    //_CrtDumpMemoryLeaks();
    break;

  case DLL_THREAD_ATTACH:
    break;

  case DLL_THREAD_DETACH:
    break;
  }

  return true;
}
WPARAM LastHwnd = NULL;
LPARAM LastCoords = 0;
std::vector<SuggestionsMenuItem *> *MenuList = NULL;
// Ok, trying to use window subclassing to handle messages
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT Message, WPARAM wParam,
                                   LPARAM lParam) {
  LRESULT ret; // int->LRESULT, fix x64 issue, still compatible with x86
  switch (Message) {
  case WM_INITMENUPOPUP: {
    // Checking that it isn't system menu and nor any main menu except 0th
    if (MenuList && LOWORD(lParam) == 0 && HIWORD(lParam) == 0) {
      // Special check for 0th main menu item
      MENUBARINFO info;
      info.cbSize = sizeof(MENUBARINFO);
      GetMenuBarInfo(nppData._nppHandle, OBJID_MENU, 0, &info);
      HMENU MainMenu = info.hMenu;
      MENUITEMINFO FileMenuItem;
      FileMenuItem.cbSize = sizeof(MENUITEMINFO);
      FileMenuItem.fMask = MIIM_SUBMENU;
      GetMenuItemInfo(MainMenu, 0, true, &FileMenuItem);

      HMENU Menu = (HMENU)wParam;
      if (FileMenuItem.hSubMenu != Menu && GetLangsSubMenu() != Menu) {
        for (unsigned int i = 0; i < MenuList->size(); i++) {
          InsertSuggMenuItem(Menu, (*MenuList)[i]->Text, (*MenuList)[i]->Id, i,
                             (*MenuList)[i]->Separator);
        }
      }
    }
    CLEAN_AND_ZERO_POINTER_VECTOR(MenuList);
  } break;

  case WM_COMMAND:
    if (HIWORD(wParam) == 0) {
      if (!GetUseAllocatedIds())
        getSpellChecker()->ProcessMenuResult (wParam);

      if (LOWORD(wParam) == IDM_FILE_PRINTNOW ||
          LOWORD(wParam) == IDM_FILE_PRINT) {
        bool AutoCheckDisabledWhilePrinting = getSpellChecker()->getAutoCheckText ();

        if (AutoCheckDisabledWhilePrinting)
          getSpellChecker()->SwitchAutoCheck();

        ret = ::CallWindowProc(wndProcNotepad, hWnd, Message, wParam, lParam);

        if (AutoCheckDisabledWhilePrinting)
          getSpellChecker()->SwitchAutoCheck();

        return ret;
      }
    }
    break;
  case WM_NOTIFY:
    // Removing possibility of adding items to tab bar menu.
    if (((LPNMHDR)lParam)->code == NM_RCLICK) {
      CLEAN_AND_ZERO_POINTER_VECTOR(MenuList);
    }
    break;
  case WM_CONTEXTMENU:
    LastHwnd = wParam;
    LastCoords = lParam;
    getSpellChecker()->precalculateMenu();
    return true;
  case WM_DISPLAYCHANGE: {
    getSpellChecker()->HideSuggestionBox();
  } break;
  }

  if (Message != 0) {
    if (Message == GetCustomGUIMessageId (CustomGUIMessage::GENERIC_CALLBACK)) {
        const auto callbackPtr = std::unique_ptr<CallbackData> (reinterpret_cast<CallbackData *> (wParam));
        if (callbackPtr->aliveStatus.expired())
            return false;

       callbackPtr->callback ();
       return true;
    }
  }
  ret = ::CallWindowProc(wndProcNotepad, hWnd, Message, wParam, lParam);
  return ret;
}

LRESULT showCalculatedMenu(std::vector<SuggestionsMenuItem *>* menuListPtr) {
    MenuList = menuListPtr;
    return ::CallWindowProc(wndProcNotepad, nppData._nppHandle, WM_CONTEXTMENU, LastHwnd, LastCoords);
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) {
  nppData = notpadPlusData;
  commandMenuInit();
  wndProcNotepad = (WNDPROC)::SetWindowLongPtr(nppData._nppHandle, GWLP_WNDPROC,
                                               (LPARAM)SubWndProcNotepad);
}

extern "C" __declspec(dllexport) const wchar_t *getName() {
  return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) {
  *nbF = nbFunc;
  return funcItem;
}

// For now doesn't look like there is such a need in check modified, but code
// stays until thorough testing
void WINAPI doRecheck(HWND, UINT, UINT_PTR, DWORD) {
  if (recheckTimer)
    SetTimer (nppData._nppHandle,  recheckTimer, USER_TIMER_MAXIMUM, doRecheck);
  recheckDone = true;

  getSpellChecker()->RecheckVisible();
  if (!firstRestyle)
    restylingCausedRecheckWasDone = true;
  firstRestyle = false;
}

// (PVOID /*lpParameter*/, BOOLEAN /*TimerOrWaitFired*/)
void WINAPI uiUpdate (HWND, UINT, UINT_PTR, DWORD)  {
    GetDownloadDics()->uiUpdate ();
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) {
  /*
  // DEBUG_CODE:
  long CurPos = SendMsgToEditor(&nppData, SCI_GETCURRENTPOS);
  int Style = SendMsgToEditor(&nppData, SCI_GETSTYLEAT, CurPos);
  */
  switch (notifyCode->nmhdr.code) {
  case NPPN_SHUTDOWN: {
    MSG msg;
    while (PeekMessage(&msg, NULL, WM_USER, 0xFFFF,
                       PM_REMOVE)) // Clearing message queue to make sure that
                                   // we're not stuck in SendMesage somewhere
    {
    };

    if (recheckTimer) KillTimer (nullptr, recheckTimer);
    if (uiTimer) KillTimer (nullptr, uiTimer);
    commandMenuCleanUp();

    pluginCleanUp();
    if (wndProcNotepad != NULL)
      // LONG_PTR is more x64 friendly, yet not affecting x86
      ::SetWindowLongPtr(nppData._nppHandle, GWLP_WNDPROC,
                         (LONG_PTR)wndProcNotepad); // Removing subclassing
  } break;

  case NPPN_READY: {
    RegisterCustomMessages();
    InitClasses();
    CheckQueue.clear();
    LoadSettings();
    getSpellChecker ()->CheckFileName();
    CreateHooks();
    recheckTimer = SetTimer (nppData._nppHandle, 0, USER_TIMER_MAXIMUM , doRecheck);
    uiTimer = SetTimer (nppData._nppHandle,  0, 100, uiUpdate);
    getSpellChecker()->RecheckVisibleBothViews();
    restylingCausedRecheckWasDone = false;
    getSpellChecker()->SetSuggestionsBoxTransparency();
    getSpellChecker()->ReinitLanguageLists(false); // To update quick lang change menu
    UpdateLangsMenu();
  } break;

  case NPPN_BUFFERACTIVATED: {
    getSpellChecker ()->CheckFileName();
    // SendEvent (EID_HIDE_SUGGESTIONS_BOX);
    RecheckVisible();
    restylingCausedRecheckWasDone = false;
  } break;

  case SCN_UPDATEUI:
    if (notifyCode->updated & (SC_UPDATE_CONTENT) && (recheckDone || firstRestyle) &&
        !restylingCausedRecheckWasDone ) // If restyling wasn't caused by user input...
    {
      getSpellChecker()->RecheckVisible();
      if (!firstRestyle)
        restylingCausedRecheckWasDone = true;
    } else if (notifyCode->updated &
                   (SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL) &&
               recheckDone) // If scroll wasn't caused by user input...
    {
      getSpellChecker()->RecheckVisible(true);
      restylingCausedRecheckWasDone = false;
    }
    getSpellChecker()->HideSuggestionBox();
    break;

  case SCN_MODIFIED:
    if (notifyCode->modificationType &
        (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT)) {
      long Start = 0, End = 0;
      if (notifyCode->modificationType & SC_MOD_DELETETEXT) {
        Start = notifyCode->position;
        End = notifyCode->position;
      } else {
        Start = notifyCode->position;
        End = notifyCode->position + notifyCode->length - 1;
      }

      if (recheckTimer)
          {
              SetTimer (nppData._nppHandle,  recheckTimer, RecheckDelay, doRecheck);
              recheckDone = false;
          }
    }
    break;

  case NPPN_LANGCHANGED: {
    getSpellChecker()->langChange();
  } break;

  case NPPN_TBMODIFICATION: {
    AddIcons();
  }

  default:
    return;
  }
}

// Here you can process the Npp Messages
// I will make the messages accessible little by little, according to the need
// of plugin development.
// Please let me know if you need to access to some messages :

void InitNeededDialogs(WPARAM wParam) {
  // A little bit of code duplication here :(
  auto MenuId = wParam;
  if ((!GetUseAllocatedIds() && HIBYTE(MenuId) != DSPELLCHECK_MENU_ID &&
       HIBYTE(MenuId) != LANGUAGE_MENU_ID) ||
      (GetUseAllocatedIds() && ((int)MenuId < GetContextMenuIdStart() ||
                                (int)MenuId > GetContextMenuIdStart() + 350)))
    return;
  int UsedMenuId = 0;
  if (GetUseAllocatedIds()) {
    UsedMenuId = ((int)MenuId < GetLangsMenuIdStart() ? DSPELLCHECK_MENU_ID
                                                      : LANGUAGE_MENU_ID);
  } else {
    UsedMenuId = HIBYTE(MenuId);
  }
  switch (UsedMenuId) {
  case LANGUAGE_MENU_ID:
    WPARAM Result = 0;
    if (!GetUseAllocatedIds())
      Result = LOBYTE(MenuId);
    else
      Result = MenuId - GetLangsMenuIdStart();
    if (Result == DOWNLOAD_DICS)
      GetDownloadDics()->DoDialog();
    else if (Result == CUSTOMIZE_MULTIPLE_DICS) {
      GetLangList()->DoDialog();
    } else if (Result == REMOVE_DICS) {
      GetRemoveDics()->DoDialog();
    }
    break;
  }
}

extern "C" __declspec(dllexport) LRESULT
    messageProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/) {
  switch (Message) {
  case WM_MOVE:
    getSpellChecker()->HideSuggestionBox();
    return false;
  case WM_COMMAND: {
    if (HIWORD(wParam) == 0 && GetUseAllocatedIds()) {
      InitNeededDialogs(wParam);
      getSpellChecker()->ProcessMenuResult (wParam);
    }
  } break;
  }

  return false;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; }
#endif // UNICODE
