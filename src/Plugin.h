//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"
#include "SettingsDlg.h"

const TCHAR NPP_PLUGIN_NAME[] = TEXT("DSpellCheck");

const int nbFunc = 5;

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
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);

// WARNING: Consequent messages guaranteed to be delivered in order, only if they are in corresponding order in enum
// TODO: Change events system to PostThreadMessage
typedef enum {
  EID_SWITCH_AUTOCHECK,
  EID_LOAD_SETTINGS,
  EID_FILL_DIALOGS,
  EID_APPLY_SETTINGS,
  EID_HIDE_DIALOG,
  EID_CHECK_FILE_NAME,
  EID_RECHECK_VISIBLE,
  EID_KILLTHREAD,
  EID_THREADKILLED,
  EID_INIT_SUGGESTIONS_BOX,
  EID_HIDE_SUGGESTIONS_BOX,
  EID_SHOW_SUGGESTION_MENU,
  EID_RECHECK_MODIFIED_ZONE,
  EID_APPLYMENUACTION,
  EID_SET_SUGGESTIONS_BOX_TRANSPARENCY,
  EID_DEFAULT_DELIMITERS,
  EID_FIND_NEXT_MISTAKE,
  EID_FIND_PREV_MISTAKE,
  EID_WRITE_SETTING,
  EID_MAX,
} EventId;

void SendEvent (EventId Event);
void WaitForEvent (EventId Event, DWORD WaitTime = INFINITE);
void SetDelimiters (const char *Str);
const char *GetDelimiters ();
void SetLanguage (const char *Str);
void setEncodingById (int EncId);
BOOL GetAutoCheckText ();
void updateAutocheckStatus ();
void LoadSettings ();
void RecheckVisible ();
void CreateThreadResources ();
void InitClasses ();
void CreateHooks ();
HANDLE getHModule ();
FuncItem *get_funcItem ();
void SetModifiedZoneShared (long Start, long End);
void GetModifiedZoneShared (long &Start, long &End);
void PostMessageToMainThread (UINT Msg, WPARAM WParam, LPARAM LParam);

// From DllMain, possibly move to DllMain.h
void SetRecheckDelay (int Value);
int GetRecheckDelay ();

#endif //PLUGINDEFINITION_H

