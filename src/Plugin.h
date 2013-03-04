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

const int nbFunc = 4;

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

typedef enum {
  EID_SWITCH_AUTOCHECK,
  EID_LOAD_SETTINGS,
  EID_FILL_DIALOGS,
  EID_APPLY_SETTINGS,
  EID_HIDE_DIALOG,
  EID_RECHECK_VISIBLE,
  EID_KILLTHREAD,
  EID_THREADKILLED,
  EID_INITSUGGESTIONS,
  EID_MAX
} EventId;

void SendEvent (EventId Event);
void SetDelimeters (const char *Str);
const char *GetDelimeters ();
void SetLanguage (const char *Str);
void setEncodingById (int EncId);
BOOL GetAutoCheckText ();
void updateAutocheckStatus ();
void LoadSettings ();
void RecheckVisible ();
FuncItem *get_funcItem ();

#endif //PLUGINDEFINITION_H

