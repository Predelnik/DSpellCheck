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
#include "RemoveDictionariesDialog.h"
#include "MainDef.h"
#include "SpellChecker.h"
#include "menuCmdID.h"

#ifdef VLD_BUILD
#include <vld.h>
#endif // VLD_BUILD

extern FuncItem func_item[nb_func]; // NOLINT
extern NppData npp_data; // NOLINT
extern bool do_close_tag; // NOLINT
HANDLE HModule;
std::vector<std::pair<long, long>> CheckQueue;
UINT_PTR uiTimer = 0u;
UINT_PTR recheckTimer = 0u;
bool recheckDone = true;
WNDPROC wndProcNotepad = nullptr;
bool restylingCausedRecheckWasDone =
    false; // Hack to avoid eternal cycle in case of scintilla bug
bool firstRestyle = true; // hack to successfully avoid checking hyperlinks when they appear on program start

bool APIENTRY DllMain(HANDLE hModule, DWORD reasonForCall, LPVOID) {
  HModule = hModule;

  switch (reasonForCall) {
  case DLL_PROCESS_ATTACH:
    plugin_init(hModule);
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
std::vector<SuggestionsMenuItem> curMenuList;
// Ok, trying to use window subclassing to handle messages
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT Message, WPARAM wParam,
                                   LPARAM lParam) {
  LRESULT ret; // int->LRESULT, fix x64 issue, still compatible with x86
  switch (Message) {
  case WM_INITMENUPOPUP: {
    // Checking that it isn't system menu and nor any main menu except 0th
    if (!curMenuList.empty () && LOWORD(lParam) == 0 && HIWORD(lParam) == 0) {
      // Special check for 0th main menu item
      MENUBARINFO info;
      info.cbSize = sizeof(MENUBARINFO);
      GetMenuBarInfo(npp_data.npp_handle, OBJID_MENU, 0, &info);
      HMENU MainMenu = info.hMenu;
      MENUITEMINFO fileMenuItem;
      fileMenuItem.cbSize = sizeof(MENUITEMINFO);
      fileMenuItem.fMask = MIIM_SUBMENU;
      GetMenuItemInfo(MainMenu, 0, true, &fileMenuItem);

      const auto menu = reinterpret_cast<HMENU>(wParam);
      if (fileMenuItem.hSubMenu != menu && get_langs_sub_menu() != menu) {
        int i = 0;
        for (auto &item : curMenuList) {
              InsertSuggMenuItem(menu, item.Text.c_str (), item.Id, i, item.Separator);
            ++i;
        }
      }
    }
    curMenuList.clear ();
  } break;

  case WM_COMMAND:
    if (HIWORD(wParam) == 0) {
      if (!get_use_allocated_ids())
        get_spell_checker()->ProcessMenuResult (wParam);

      if (LOWORD(wParam) == IDM_FILE_PRINTNOW ||
          LOWORD(wParam) == IDM_FILE_PRINT) {
        bool AutoCheckDisabledWhilePrinting = get_spell_checker()->getAutoCheckText ();

        if (AutoCheckDisabledWhilePrinting)
          get_spell_checker()->SwitchAutoCheck();

        ret = ::CallWindowProc(wndProcNotepad, hWnd, Message, wParam, lParam);

        if (AutoCheckDisabledWhilePrinting)
          get_spell_checker()->SwitchAutoCheck();

        return ret;
      }
    }
    break;
  case WM_NOTIFY:
    // Removing possibility of adding items to tab bar menu.
    if (((LPNMHDR)lParam)->code == NM_RCLICK) {
      curMenuList.clear ();
    }
    break;
  case WM_CONTEXTMENU:
    LastHwnd = wParam;
    LastCoords = lParam;
    get_spell_checker()->precalculateMenu();
    return true;
  case WM_DISPLAYCHANGE: {
    get_spell_checker()->HideSuggestionBox();
  } break;
  }

  if (Message != 0) {
    if (Message == get_custom_gui_message_id (CustomGuiMessage::generic_callback)) {
        const auto callbackPtr = std::unique_ptr<CallbackData> (reinterpret_cast<CallbackData *> (wParam));
        if (callbackPtr->alive_status.expired())
            return false;

       callbackPtr->callback ();
       return true;
    }
  }
  ret = ::CallWindowProc(wndProcNotepad, hWnd, Message, wParam, lParam);
  return ret;
}

LRESULT show_calculated_menu(const std::vector<SuggestionsMenuItem>&& menuList) {
    curMenuList = std::move (menuList);
    return ::CallWindowProc(wndProcNotepad, npp_data.npp_handle, WM_CONTEXTMENU, LastHwnd, LastCoords);
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) { // NOLINT
  npp_data = notpadPlusData;
  command_menu_init();
  wndProcNotepad = (WNDPROC)::SetWindowLongPtr(npp_data.npp_handle, GWLP_WNDPROC,
                                               (LPARAM)SubWndProcNotepad);
}

extern "C" __declspec(dllexport) const wchar_t *getName() { // NOLINT
  return npp_plugin_name;
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) { // NOLINT
  *nbF = nb_func;
  return func_item;
}

// For now doesn't look like there is such a need in check modified, but code
// stays until thorough testing
void WINAPI doRecheck(HWND, UINT, UINT_PTR, DWORD) {
  if (recheckTimer)
    SetTimer (npp_data.npp_handle,  recheckTimer, USER_TIMER_MAXIMUM, doRecheck);
  recheckDone = true;

  get_spell_checker()->RecheckVisible();
  if (!firstRestyle)
    restylingCausedRecheckWasDone = true;
  firstRestyle = false;
}

// (PVOID /*lpParameter*/, BOOLEAN /*TimerOrWaitFired*/)
void WINAPI uiUpdate (HWND, UINT, UINT_PTR, DWORD)  {
    get_download_dics()->ui_update ();
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode) { // NOLINT
  /*
  // DEBUG_CODE:
  long CurPos = SendMsgToEditor(&nppData, SCI_GETCURRENTPOS);
  int Style = SendMsgToEditor(&nppData, SCI_GETSTYLEAT, CurPos);
  */
  switch (notifyCode->nmhdr.code) {
  case NPPN_SHUTDOWN: {
    MSG msg;
    while (PeekMessage(&msg, nullptr, WM_USER, 0xFFFF,
                       PM_REMOVE)) // Clearing message queue to make sure that
                                   // we're not stuck in SendMesage somewhere
    {
    };

    if (recheckTimer) KillTimer (nullptr, recheckTimer);
    if (uiTimer) KillTimer (nullptr, uiTimer);
    command_menu_clean_up();

    plugin_clean_up();
    if (wndProcNotepad != nullptr)
      // LONG_PTR is more x64 friendly, yet not affecting x86
      ::SetWindowLongPtr(npp_data.npp_handle, GWLP_WNDPROC,
                         (LONG_PTR)wndProcNotepad); // Removing subclassing
  } break;

  case NPPN_READY: {
    register_custom_messages();
    init_classes();
    CheckQueue.clear();
    load_settings();
    get_spell_checker ()->CheckFileName();
    create_hooks();
    recheckTimer = SetTimer (npp_data.npp_handle, 0, USER_TIMER_MAXIMUM , doRecheck);
    uiTimer = SetTimer (npp_data.npp_handle,  0, 100, uiUpdate);
    get_spell_checker()->RecheckVisibleBothViews();
    restylingCausedRecheckWasDone = false;
    get_spell_checker()->SetSuggestionsBoxTransparency();
    get_spell_checker()->ReinitLanguageLists(false); // To update quick lang change menu
    update_langs_menu();
  } break;

  case NPPN_BUFFERACTIVATED: {
    if (get_spell_checker ()) {
        get_spell_checker ()->CheckFileName();
        recheck_visible();
        restylingCausedRecheckWasDone = false;
    }
  } break;

  case SCN_UPDATEUI:
    if (notifyCode->updated & (SC_UPDATE_CONTENT) && (recheckDone || firstRestyle) &&
        !restylingCausedRecheckWasDone ) // If restyling wasn't caused by user input...
    {
      get_spell_checker()->RecheckVisible();
      if (!firstRestyle)
        restylingCausedRecheckWasDone = true;
    } else if (notifyCode->updated &
                   (SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL) &&
               recheckDone) // If scroll wasn't caused by user input...
    {
      get_spell_checker()->RecheckVisible(true);
      restylingCausedRecheckWasDone = false;
    }
    get_spell_checker()->HideSuggestionBox();
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
              SetTimer (npp_data.npp_handle,  recheckTimer, get_spell_checker ()->getRecheckDelay(), doRecheck);
              recheckDone = false;
          }
    }
    break;

  case NPPN_LANGCHANGED: {
    get_spell_checker()->langChange();
  } break;

  case NPPN_TBMODIFICATION: {
    add_icons();
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
  if ((!get_use_allocated_ids() && HIBYTE(MenuId) != DSPELLCHECK_MENU_ID &&
       HIBYTE(MenuId) != LANGUAGE_MENU_ID) ||
      (get_use_allocated_ids() && ((int)MenuId < get_context_menu_id_start() ||
                                (int)MenuId > get_context_menu_id_start() + 350)))
    return;
  int UsedMenuId = 0;
  if (get_use_allocated_ids()) {
    UsedMenuId = ((int)MenuId < get_langs_menu_id_start() ? DSPELLCHECK_MENU_ID
                                                      : LANGUAGE_MENU_ID);
  } else {
    UsedMenuId = HIBYTE(MenuId);
  }
  switch (UsedMenuId) {
  case LANGUAGE_MENU_ID:
    WPARAM Result = 0;
    if (!get_use_allocated_ids())
      Result = LOBYTE(MenuId);
    else
      Result = MenuId - get_langs_menu_id_start();
    if (Result == DOWNLOAD_DICS)
      get_download_dics()->do_dialog();
    else if (Result == CUSTOMIZE_MULTIPLE_DICS) {
      get_lang_list()->do_dialog();
    } else if (Result == REMOVE_DICS) {
      get_remove_dics()->do_dialog();
    }
    break;
  }
}

extern "C" __declspec(dllexport) LRESULT
    messageProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/) { // NOLINT
  switch (Message) {
  case WM_MOVE:
    if (get_spell_checker())
      get_spell_checker()->HideSuggestionBox();
    return false;
  case WM_COMMAND: {
    if (HIWORD(wParam) == 0 && get_use_allocated_ids()) {
      InitNeededDialogs(wParam);
      get_spell_checker()->ProcessMenuResult (wParam);
    }
  } break;
  }

  return false;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; } // NOLINT
#endif // UNICODE
