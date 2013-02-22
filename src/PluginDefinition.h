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

const TCHAR NPP_PLUGIN_NAME[] = TEXT("Decent Spell Check Plugin");

const int nbFunc = 1;


//
// Initialization of your plugin data
// It will be called while plugin loading
//
void pluginInit(HANDLE hModule);

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


//
// Your plugin command functions
//
void hello();
void CheckText ();
void helloFX();
void WhatIsNpp();
void insertDateTime(bool format);
void insertShortDateTime();
void insertLongDateTime();
void insertCurrentPath(int which);
void insertCurrentFullPath();
void insertCurrentFileName();
void insertCurrentDirectory();
void insertHtmlCloseTag();
void getFileNamesDemo();
void getSessionFileNamesDemo();
void saveCurrentSessionDemo();
void DockableDlgDemo();

LRESULT SendMsgToEditor(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
BOOL AspellInitSettings ();

#endif //PLUGINDEFINITION_H
