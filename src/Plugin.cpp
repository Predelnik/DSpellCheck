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

#include "Plugin.h"
#include "menuCmdID.h"

//
// put the headers you need here
//

#include "aspell.h"
#include "CommonFunctions.h"
#include "controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "AboutDlg.h"
#include "DownloadDicsDlg.h"
#include "LangList.h"
#include "Progress.h"
#include "RemoveDics.h"
#include "SelectProxy.h"
#include "SpellChecker.h"
#include "Suggestions.h"

#include "StackWalker/StackWalker.h"

const wchar_t configFileName[] = L"DSpellCheck.ini";

#ifdef UNICODE
#define generic_itoa _itow
#else
#define generic_itoa itoa
#endif

FuncItem funcItem[nbFunc];
BOOL ResourcesInited = FALSE;

FuncItem *get_funcItem() {
  // Well maybe we should lock mutex here
  return funcItem;
}

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;
wchar_t IniFilePath[MAX_PATH];
DWORD CustomGUIMessageIds[CustomGUIMessage::MAX] = {0};
bool doCloseTag = false;
SpellChecker *SpellCheckerInstance = 0;
SettingsDlg *SettingsDlgInstance = 0;
Suggestions *SuggestionsInstance = 0;
LangList *LangListInstance = 0;
RemoveDics *RemoveDicsInstance = 0;
SelectProxy *SelectProxyInstance = 0;
Progress *ProgressInstance = 0;
DownloadDicsDlg *DownloadDicsDlgInstance = 0;
AboutDlg *AboutDlgInstance = 0;
HANDLE hNetworkThread = NULL;
HMENU LangsMenu;
DWORD NetworkThreadId = 0;
int ContextMenuIdStart;
int LangsMenuIdStart = FALSE;
BOOL UseAllocatedIds;
toolbarIcons *AutoCheckIcon = 0;
BOOL AutoCheckState = FALSE;

HANDLE hNetworkEvent[EID_NETWORK_MAX] = {NULL};
HANDLE hModule = NULL;
HHOOK HMouseHook = NULL;
// HHOOK HCmHook = NULL;

//
// Initialize your plug-in data here
// It will be called while plug-in loading
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
  switch (wParam) {
  case WM_MOUSEMOVE:
    SendEvent(EID_INIT_SUGGESTIONS_BOX);
    break;
  }
  return CallNextHookEx(HMouseHook, nCode, wParam, lParam);
  ;
}

void SetContextMenuIdStart(int Id) { ContextMenuIdStart = Id; }

void SetLangsMenuIdStart(int Id) { LangsMenuIdStart = Id; }

void SetUseAllocatedIds(BOOL Value) { UseAllocatedIds = Value; }

int GetContextMenuIdStart() { return ContextMenuIdStart; }

int GetLangsMenuIdStart() { return LangsMenuIdStart; }

BOOL GetUseAllocatedIds() { return UseAllocatedIds; }

SpellChecker *GetSpellChecker() { return SpellCheckerInstance; }


void GetDefaultHunspellPath_(wchar_t *&Path) {
  Path = new wchar_t[MAX_PATH];
  wcscpy(Path, IniFilePath);
  wchar_t *Pointer = wcsrchr(Path, L'\\');
  *Pointer = 0;
  wcscat(Path, L"\\Hunspell");
}

void pluginInit(HANDLE hModuleArg) {
  hModule = hModuleArg;
  // Init it all dialog classes:
}

LangList *GetLangList() { return LangListInstance; }

RemoveDics *GetRemoveDics() { return RemoveDicsInstance; }

SelectProxy *GetSelectProxy() { return SelectProxyInstance; }

Progress *GetProgress() { return ProgressInstance; }

DownloadDicsDlg *GetDownloadDics() { return DownloadDicsDlgInstance; }

HANDLE getHModule() { return hModule; }


class MyStackWalker : public StackWalker {
public:
  MyStackWalker() : StackWalker() {}

protected:
  virtual void OnOutput(LPCSTR szText) {
    FILE *fp = _wfopen(L"DSpellCheck_Debug.log", L"a");
    fprintf(fp, szText);
    fclose(fp);
    StackWalker::OnOutput(szText);
  }
};

int filter(unsigned int, struct _EXCEPTION_POINTERS *ep) {
  MyStackWalker sw;
  sw.ShowCallstack(GetCurrentThread(), ep->ContextRecord);
  return EXCEPTION_CONTINUE_SEARCH;
}

DWORD WINAPI ThreadNetwork(LPVOID lpParam) {
  DWORD dwWaitResult = EID_MAX;
  SpellChecker *spellChecker = (SpellChecker *)lpParam;

  BOOL bRun = spellChecker->NotifyNetworkEvent(EID_NETWORK_MAX);

  MSG Msg;

  // Creating thread message queue
  PeekMessage(&Msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

  while (bRun) {
    dwWaitResult =
        MsgWaitForMultipleObjectsEx(EID_NETWORK_MAX, hNetworkEvent, INFINITE,
                                    QS_ALLEVENTS, MWMO_INPUTAVAILABLE);
    if (dwWaitResult == (unsigned int)-1) {
      spellChecker->ErrorMsgBox(L"Thread has died");
      break;
    }

    if (dwWaitResult == EID_NETWORK_MAX) {
      while (PeekMessage(&Msg, 0, 0, 0, PM_REMOVE) && bRun) {
        bRun = spellChecker->NotifyMessage(Msg.message, Msg.wParam, Msg.lParam);
      }
    } else
      bRun = spellChecker->NotifyNetworkEvent(dwWaitResult);
  }

  return 0;
}

void CreateThreadResources() {

  for (int i = 0; i < EID_NETWORK_MAX; i++)
    hNetworkEvent[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);


  hNetworkThread = CreateThread(NULL, 0, ThreadNetwork, SpellCheckerInstance, 0,
                                &NetworkThreadId);

  ResourcesInited = TRUE;
}

void CreateHooks() {
  HMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, 0, GetCurrentThreadId());
  // HCmHook = SetWindowsHookExW(WH_CALLWNDPROC, ContextMenuProc, 0,
  // GetCurrentThreadId());
}

void KillThreadResources() {
  CloseHandle(hNetworkThread);
  /* kill events */

  for (int i = 0; i < EID_NETWORK_MAX; i++)
    CloseHandle(hNetworkEvent[i]);
}

void pluginCleanUp() {
  UnhookWindowsHookEx(HMouseHook);
  KillThreadResources();

  CLEAN_AND_ZERO(SpellCheckerInstance);
  CLEAN_AND_ZERO(SettingsDlgInstance);
  CLEAN_AND_ZERO(AboutDlgInstance);
  CLEAN_AND_ZERO(SuggestionsInstance);
  CLEAN_AND_ZERO(LangListInstance);
  CLEAN_AND_ZERO(SelectProxyInstance);
  CLEAN_AND_ZERO(RemoveDicsInstance);
  CLEAN_AND_ZERO(DownloadDicsDlgInstance);
  CLEAN_AND_ZERO(ProgressInstance);
}

void SendEvent(EventId Event) {
  if (ResourcesInited)
      SpellCheckerInstance->NotifyEvent(Event);
}

void SendNetworkEvent(NetworkEventId Event) {
  if (ResourcesInited)
    SetEvent(hNetworkEvent[Event]);
}

void PostMessageToMainThread(UINT Msg, WPARAM WParam, LPARAM LParam) {
  if (ResourcesInited) {
    SpellCheckerInstance->NotifyMessage (Msg, WParam, LParam);
  }
}

DWORD WaitForNetworkEvent(NetworkEventId Event, DWORD WaitTime) {
  if (ResourcesInited)
    return WaitForSingleObject(hNetworkEvent[Event], WaitTime);
  else
    return WAIT_FAILED;
}

void RegisterCustomMessages() {
  for (int i = 0; i < static_cast<int>(CustomGUIMessage::MAX); i++) {
    CustomGUIMessageIds[i] = RegisterWindowMessage(CustomGUIMesssagesNames[i]);
  }
}

DWORD GetCustomGUIMessageId(CustomGUIMessage MessageId) {
  return CustomGUIMessageIds[static_cast<int>(MessageId)];
}

void SwitchAutoCheckText() { SendEvent(EID_SWITCH_AUTOCHECK); }

void GetSuggestions() {
  // SendEvent (EID_INITSUGGESTIONS);
}

void StartSettings() { SettingsDlgInstance->DoDialog(); }

void StartManual() {
  ShellExecute(NULL, L"open",
               L"https://github.com/Predelnik/DSpellCheck/wiki/Manual", NULL,
               NULL, SW_SHOW);
}

void StartAboutDlg() { AboutDlgInstance->DoDialog(); }

void StartLanguageList() { LangListInstance->DoDialog(); }

void LoadSettings() { SendEvent(EID_LOAD_SETTINGS); }

void RecheckVisible() { SendEvent(EID_RECHECK_VISIBLE); }

void FindNextMistake() { SendEvent(EID_FIND_NEXT_MISTAKE); }

void FindPrevMistake() { SendEvent(EID_FIND_PREV_MISTAKE); }

void QuickLangChangeContext() {
  POINT Pos;
  GetCursorPos(&Pos);
  TrackPopupMenu(GetLangsSubMenu(), 0, Pos.x, Pos.y, 0, nppData._nppHandle, 0);
}

//
// Initialization of your plug-in commands
// You should fill your plug-ins commands here
void commandMenuInit() {
  //
  // Firstly we get the parameters from your plugin config file (if any)
  //

  // get path of plugin configuration
  ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH,
                (LPARAM)IniFilePath);

  // if config path doesn't exist, we create it
  if (PathFileExists(IniFilePath) == FALSE) {
    ::CreateDirectory(IniFilePath, NULL);
  }

  // make your plugin config file full file path name
  PathAppend(IniFilePath, configFileName);

  wchar_t Buf[DEFAULT_BUF_SIZE];
  wchar_t *EndPtr;
  int x;
  GetPrivateProfileString(L"SpellCheck", L"Recheck_Delay", L"500", Buf,
                          DEFAULT_BUF_SIZE, IniFilePath);
  x = wcstol(Buf, &EndPtr, 10);
  if (*EndPtr)
    SetRecheckDelay(500, 0);
  else
    SetRecheckDelay(x, 0);

  //--------------------------------------------//
  //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
  //--------------------------------------------//
  // with function :
  // setCommand(int index,                      // zero based number to indicate
  // the order of command
  //            wchar_t *commandName,             // the command name that you
  //            want to see in plugin menu
  //            PFUNCPLUGINCMD functionPointer, // the symbol of function
  //            (function pointer) associated with this command. The body should
  //            be defined below. See Step 4.
  //            ShortcutKey *shortcut,          // optional. Define a shortcut
  //            to trigger this command
  //            bool check0nInit                // optional. Make this menu item
  //            be checked visually
  //            );
  ShortcutKey *shKey = new ShortcutKey;
  shKey->_isAlt = true;
  shKey->_isCtrl = false;
  shKey->_isShift = false;
  shKey->_key = 0x41 + 'a' - 'a';
  setNextCommand(TEXT("Spell Check Document Automatically"),
                 SwitchAutoCheckText, shKey, false);
  shKey = new ShortcutKey;
  shKey->_isAlt = true;
  shKey->_isCtrl = false;
  shKey->_isShift = false;
  shKey->_key = 0x41 + 'n' - 'a';
  setNextCommand(TEXT("Find Next Misspelling"), FindNextMistake, shKey, false);
  shKey = new ShortcutKey;
  shKey->_isAlt = true;
  shKey->_isCtrl = false;
  shKey->_isShift = false;
  shKey->_key = 0x41 + 'b' - 'a';
  setNextCommand(TEXT("Find Previous Misspelling"), FindPrevMistake, shKey,
                 false);

  shKey = new ShortcutKey;
  shKey->_isAlt = true;
  shKey->_isCtrl = false;
  shKey->_isShift = false;
  shKey->_key = 0x41 + 'd' - 'a';
  setNextCommand(TEXT("Change Current Language"), QuickLangChangeContext, shKey,
                 false);
  setNextCommand(TEXT("---"), NULL, NULL, false);

  setNextCommand(TEXT("Settings..."), StartSettings, NULL, false);
  setNextCommand(TEXT("Online Manual"), StartManual, NULL, false);
  setNextCommand(TEXT("About"), StartAboutDlg, NULL, false);
}

void AddIcons() {
  AutoCheckIcon = new toolbarIcons;
  AutoCheckIcon->hToolbarBmp =
      (HBITMAP)::LoadImage((HINSTANCE)hModule, MAKEINTRESOURCE(IDB_AUTOCHECK2),
                           IMAGE_BITMAP, 16, 16, LR_LOADMAP3DCOLORS);
  ::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
                (WPARAM)funcItem[0]._cmdID, (LPARAM)AutoCheckIcon);
}

void UpdateLangsMenu() { SendEvent(EID_UPDATE_LANGS_MENU); }

HMENU GetDSpellCheckMenu() {
  HMENU PluginsMenu =
      (HMENU)SendMsgToNpp(&nppData, NPPM_GETMENUHANDLE, NPPPLUGINMENU);
  HMENU DSpellCheckMenu = 0;
  int Count = GetMenuItemCount(PluginsMenu);
  int StrLen = 0;
  wchar_t *Buf = 0;
  for (int i = 0; i < Count; i++) {
    StrLen = GetMenuString(PluginsMenu, i, 0, 0, MF_BYPOSITION);
    Buf = new wchar_t[StrLen + 1];
    GetMenuString(PluginsMenu, i, Buf, StrLen + 1, MF_BYPOSITION);
    if (wcscmp(Buf, NPP_PLUGIN_NAME) == 0) {
      MENUITEMINFO Mif;
      Mif.fMask = MIIM_SUBMENU;
      Mif.cbSize = sizeof(MENUITEMINFO);
      BOOL Res = GetMenuItemInfo(PluginsMenu, i, TRUE, &Mif);

      if (Res)
        DSpellCheckMenu = (HMENU)Mif.hSubMenu;

      CLEAN_AND_ZERO_ARR(Buf);
      break;
    }
    CLEAN_AND_ZERO_ARR(Buf);
  }
  return DSpellCheckMenu;
}

HMENU GetLangsSubMenu(HMENU DSpellCheckMenuArg) {
  HMENU DSpellCheckMenu;
  if (!DSpellCheckMenuArg)
    DSpellCheckMenu = GetDSpellCheckMenu();
  else
    DSpellCheckMenu = DSpellCheckMenuArg;
  if (!DSpellCheckMenu)
    return 0;

  MENUITEMINFO Mif;

  Mif.fMask = MIIM_SUBMENU;
  Mif.cbSize = sizeof(MENUITEMINFO);

  BOOL Res =
      GetMenuItemInfo(DSpellCheckMenu, QUICK_LANG_CHANGE_ITEM, TRUE, &Mif);
  if (!Res)
    return NULL;

  return Mif.hSubMenu; // TODO: CHECK IS THIS CORRECT FIX
}

void InitClasses() {
  INITCOMMONCONTROLSEX icc;

  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icc);

  InitCheckedListBox((HINSTANCE)hModule);

  SuggestionsInstance = new Suggestions;
  SuggestionsInstance->init((HINSTANCE)hModule, nppData._nppHandle, nppData);
  SuggestionsInstance->DoDialog();

  SettingsDlgInstance = new SettingsDlg;
  SettingsDlgInstance->init((HINSTANCE)hModule, nppData._nppHandle, nppData);

  AboutDlgInstance = new AboutDlg;
  AboutDlgInstance->init((HINSTANCE)hModule, nppData._nppHandle);

  ProgressInstance = new Progress;
  ProgressInstance->init((HINSTANCE)hModule, nppData._nppHandle);

  LangListInstance = new LangList;
  LangListInstance->init((HINSTANCE)hModule, nppData._nppHandle);

  SelectProxyInstance = new SelectProxy;
  SelectProxyInstance->init((HINSTANCE)hModule, nppData._nppHandle);

  RemoveDicsInstance = new RemoveDics;
  RemoveDicsInstance->init((HINSTANCE)hModule, nppData._nppHandle);

  SpellCheckerInstance =
      new SpellChecker(IniFilePath, SettingsDlgInstance, &nppData,
                       SuggestionsInstance, LangListInstance);

  DownloadDicsDlgInstance = new DownloadDicsDlg;
  DownloadDicsDlgInstance->init((HINSTANCE)hModule, nppData._nppHandle,
                                SpellCheckerInstance);
}

void commandMenuCleanUp() {
  CLEAN_AND_ZERO(funcItem[0]._pShKey);
  CLEAN_AND_ZERO(funcItem[1]._pShKey);
  CLEAN_AND_ZERO(funcItem[2]._pShKey);
  CLEAN_AND_ZERO(funcItem[3]._pShKey);
  if (AutoCheckIcon)
    DeleteObject(AutoCheckIcon->hToolbarBmp);
  CLEAN_AND_ZERO(AutoCheckIcon);
  // We should deallocate shortcuts here
}
//
// Function that initializes plug-in commands
//
static int counter = 0;

bool setNextCommand(wchar_t *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk,
                    bool check0nInit) {
  if (counter >= nbFunc)
    return false;

  if (!pFunc) {
    counter++;
    return false;
  }

  lstrcpy(funcItem[counter]._itemName, cmdName);
  funcItem[counter]._pFunc = pFunc;
  funcItem[counter]._init2Check = check0nInit;
  funcItem[counter]._pShKey = sk;
  counter++;

  return true;
}

void WaitTillThreadsClosed() {
  HANDLE threadHandles[] = {hNetworkThread};
  WaitForMultipleObjects(1, threadHandles, true, INFINITE);
}

void AutoCheckStateReceived(BOOL state) { AutoCheckState = state; }

BOOL GetAutoCheckState() { return AutoCheckState; }
