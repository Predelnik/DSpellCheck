//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#ifndef SCINTILLA_H
#include "Scintilla.h"
#endif //SCINTILLA_H


const int nb_char = 64;

struct NppData {
    HWND npp_handle;
    HWND scintilla_main_handle;
    HWND scintilla_second_handle;
};

typedef void (__cdecl * Pfuncplugincmd)();

struct ShortcutKey {
    bool is_ctrl;
    bool is_alt;
    bool is_shift;
    UCHAR key;
};

struct FuncItem {
    wchar_t item_name[nb_char];
    Pfuncplugincmd p_func;
    int cmd_id;
    bool init2_check;
    ShortcutKey* p_sh_key;
};

// You should implement (or define an empty function body) those functions which are called by Notepad++ plugin manager
// ReSharper disable CppInconsistentNaming
extern "C" __declspec(dllexport) void setInfo(NppData);
extern "C" __declspec(dllexport) const wchar_t* getName();
extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int*);
extern "C" __declspec(dllexport) void beNotified(SCNotification*);
extern "C" __declspec(dllexport) LRESULT messageProc(UINT message, WPARAM w_param, LPARAM l_param);

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode();
#endif //UNICODE
// ReSharper restore CppInconsistentNaming

#endif //PLUGININTERFACE_H
