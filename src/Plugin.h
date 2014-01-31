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

#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"
#include "SettingsDlg.h"

const TCHAR NPP_PLUGIN_NAME[] = TEXT("DSpellCheck");

const int nbFunc = 8;
#define QUICK_LANG_CHANGE_ITEM 3

class LangList;
class DownloadDicsDlg;
class SelectProxy;
class SpellChecker;
class Progress;
class RemoveDics;

//
// Initialization of your plugin data
// It will be called while plugin loading
//
void pluginInit(HANDLE hModuleArg);

//
// Cleaning of your plugin
// It will be called while plugin unloading
//
void pluginCleanUp();

//
//Initialization of your plugin commands
//
void commandMenuInit();

//
//Clean up your plugin commands allocation (if any)
//
void commandMenuCleanUp();

//
// Function which sets your command
//
bool setNextCommand(TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);

// WARNING: Consequent messages guaranteed to be delivered in order, only if they are in corresponding order in enum
// TODO: Change events system to PostThreadMessage
typedef enum {
  EID_SWITCH_AUTOCHECK,
  EID_SET_SUGGESTIONS_BOX_TRANSPARENCY,
  EID_LOAD_SETTINGS,
  EID_FILL_DIALOGS,
  EID_APPLY_SETTINGS,
  EID_APPLY_MULTI_LANG_SETTINGS,
  EID_HIDE_DIALOG,
  EID_CHECK_FILE_NAME,
  EID_RECHECK_VISIBLE,
  EID_RECHECK_VISIBLE_BOTH_VIEWS,
  EID_RECHECK_INTERSECTION,
  EID_COPY_MISSPELLINGS_TO_CLIPBOARD,
  EID_KILLTHREAD,
  EID_THREADKILLED,
  EID_INIT_SUGGESTIONS_BOX,
  EID_HIDE_SUGGESTIONS_BOX,
  EID_SHOW_SUGGESTION_MENU,
  EID_UPDATE_SELECT_PROXY,
  EID_RECHECK_MODIFIED_ZONE,
  EID_INIT_CONTEXT_MENU,
  EID_APPLYMENUACTION,
  EID_DEFAULT_DELIMITERS,
  EID_FIND_NEXT_MISTAKE,
  EID_FIND_PREV_MISTAKE,
  EID_INIT_DOWNLOAD_COMBOBOX,
  EID_FILL_DOWNLOAD_DICS_DIALOG,
  EID_REMOVE_SELECTED_DICS,
  EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS,
  EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS_NO_UPDATE,
  EID_HIDE_DOWNLOAD_DICS,
  EID_UPDATE_REMOVE_DICS_OPTIONS,
  EID_UPDATE_FROM_REMOVE_DICS_OPTIONS,
  EID_UPDATE_LANG_LISTS,
  EID_UPDATE_LANGS_MENU,
  EID_LANG_CHANGE,
  EID_LIB_CHANGE,
  EID_APPLY_PROXY_SETTINGS,
  EID_SHOW_SELECT_PROXY,
  EID_MAX,
} EventId;

typedef enum {
  EID_KILLNETWORKTHREAD,
  EID_NETWORKTHREADKILLED,
  EID_DOWNLOAD_SELECTED,
  EID_CANCEL_DOWNLOAD,
  EID_FILL_FILE_LIST,
  EID_NETWORK_MAX
} NetworkEventId;

void SendEvent (EventId Event);
void SendNetworkEvent (NetworkEventId Event);
void PostMessageToMainThread (UINT Msg, WPARAM WParam = 0, LPARAM LParam = 0);
DWORD WaitForEvent (EventId Event, DWORD WaitTime = INFINITE);
DWORD WaitForNetworkEvent (NetworkEventId Event, DWORD WaitTime = INFINITE);
DWORD WaitForMultipleEvents (EventId EventFirst, EventId EventLast, DWORD WaitTime = INFINITE);
void SetDelimiters (const char *Str);
const char *GetDelimiters ();
void setEncodingById (int EncId);
BOOL GetAutoCheckText ();
void updateAutocheckStatus ();
void LoadSettings ();
void RecheckVisible ();
void CreateThreadResources ();
void InitClasses ();
void CreateHooks ();
void UpdateLangsMenu ();
void AddIcons ();
HMENU GetDSpellCheckMenu ();
HMENU GetLangsSubMenu (HMENU DSpellCheckMenuArg = 0);
HANDLE getHModule ();
LangList *GetLangList ();
RemoveDics *GetRemoveDics ();
SelectProxy *GetSelectProxy ();
Progress *GetProgress ();
DownloadDicsDlg *GetDownloadDics ();
FuncItem *get_funcItem ();
void SetModifiedZoneShared (long Start, long End);
void GetModifiedZoneShared (long &Start, long &End);
void PostMessageToMainThread (UINT Msg, WPARAM WParam, LPARAM LParam);
void GetDefaultHunspellPath_ (TCHAR *&Path);
HANDLE *GethEvent ();
void SetContextMenuIdStart (int Id);
void SetLangsMenuIdStart (int Id);
void SetUseAllocatedIds (BOOL Id);
int GetContextMenuIdStart ();
int GetLangsMenuIdStart ();
BOOL GetUseAllocatedIds ();
SpellChecker *GetSpellChecker ();
DWORD GetCustomGUIMessageId (CustomGUIMessage::e MessageId);
void RegisterCustomMessages ();

// From DllMain, possibly move to DllMain.h
void SetRecheckDelay (int Value, int WriteToIni = 1);
int GetRecheckDelay ();

#endif //PLUGINDEFINITION_H
