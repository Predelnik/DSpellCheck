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

#pragma once

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"
#include "SettingsDlg.h"

struct SuggestionsMenuItem;
const wchar_t NPP_PLUGIN_NAME[] = TEXT("DSpellCheck");

const int nbFunc = 8;
#define QUICK_LANG_CHANGE_ITEM 3

enum class CustomGUIMessage;
class LangList;
class DownloadDicsDlg;
class SelectProxy;
class SpellChecker;
class ProgressDlg;
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
bool setNextCommand(const wchar_t* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = nullptr, bool check0nInit = false);

void SetDelimiters (const char *Str);
const char *GetDelimiters ();
void setEncodingById (int EncId);
void LoadSettings ();
void RecheckVisible ();
void InitClasses ();
void CreateHooks ();
void UpdateLangsMenu ();
LRESULT showCalculatedMenu(const std::vector<SuggestionsMenuItem>&& menuList);
void AddIcons ();
bool GetAutoCheckState ();
void AutoCheckStateReceived (bool state);
HMENU GetDSpellCheckMenu ();
HMENU GetLangsSubMenu (HMENU DSpellCheckMenuArg = nullptr);
HANDLE getHModule ();
LangList *GetLangList ();
RemoveDics *GetRemoveDics ();
SelectProxy *GetSelectProxy ();
ProgressDlg *getProgress ();
DownloadDicsDlg *GetDownloadDics ();
FuncItem *get_funcItem ();
void SetModifiedZoneShared (long Start, long End);
void GetModifiedZoneShared (long &Start, long &End);
void PostMessageToMainThread (UINT Msg, WPARAM WParam, LPARAM LParam);
void GetDefaultHunspellPath_ (wchar_t *&Path);
HANDLE *GethEvent ();
void SetContextMenuIdStart (int Id);
void SetLangsMenuIdStart (int Id);
void SetUseAllocatedIds (bool Id);
int GetContextMenuIdStart ();
int GetLangsMenuIdStart ();
bool GetUseAllocatedIds ();
SpellChecker *getSpellChecker ();
DWORD GetCustomGUIMessageId (CustomGUIMessage MessageId);
void RegisterCustomMessages ();

// From DllMain, possibly move to DllMain.h
void SetRecheckDelay (int Value, int WriteToIni = 1);
int GetRecheckDelay ();
