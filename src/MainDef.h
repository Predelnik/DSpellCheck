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

#ifndef MAINDEF_H
#define MAINDEF_H

#define SCE_ERROR_UNDERLINE INDIC_CONTAINER + 1
#define CLEAN_AND_ZERO(x) do { if (x) {delete x; x = 0; }  } while (0);
#define CLEAN_AND_ZERO_ARR(x) do { if (x) {delete[] x; x = 0; }  } while (0);
#define CLEAN_AND_ZERO_STRING_VECTOR(x) do { if (x) {for (unsigned int i = 0; i < x->size (); i++) {delete[] x->at(i);} delete x; x = 0; }  } while (0);
#define CLEAN_AND_ZERO_POINTER_VECTOR(x) do { if (x) {for (unsigned int i = 0; i < x->size (); i++) {delete x->at(i);} delete x; x = 0; }  } while (0);
#define CLEAN_AND_ZERO_STRING_STACK(x) do { if (x) {while (x->size () > 0) {delete[] (x->top());x->pop ();} delete x; x = 0; }  } while (0);
#define countof(A) sizeof(A)/sizeof((A)[0])
#define DEFAULT_BUF_SIZE 4096

enum EncodingType
{
  ENCODING_UTF8 = 0,
  ENCODING_ANSI
};

#define SUGGESTIONS_BOX 0
#define SUGGESTIONS_CONTEXT_MENU 1

// Global Menu ID
// TODO: change it all to call for n++ message which gives you free menu IDs
#define DSPELLCHECK_MENU_ID 193
#define LANGUAGE_MENU_ID 197

// Menu item IDs
#define MID_ADDTODICTIONARY 101
#define MID_IGNOREALL 102
#define MULTIPLE_LANGS 201

#define USER_SERVER_CONST 100

#define COLOR_OK (RGB (0, 144, 0))
#define COLOR_FAIL (RGB (225, 0, 0))
#define COLOR_WARN (RGB (144, 144, 0))
#define COLOR_NEUTRAL (RGB (0, 0, 0))

// Custom WMs (Only for our windows and threads)
#define WM_SHOWANDRECREATEMENU  WM_USER + 1000
#define TM_MODIFIED_ZONE_INFO   WM_USER + 1001
#define TM_WRITE_SETTING        WM_USER + 1002
#define TM_UPDATE_ON_LIB_CHANGE WM_USER + 1003
#define TM_MENU_RESULT          WM_USER + 1004
#define TM_PRECALCULATE_MENU    WM_USER + 1005
#define TM_CHANGE_DIR           WM_USER + 1006
#define TM_ADD_USER_SERVER      WM_USER + 1007
#define TM_UPDATE_LANGS_MENU    WM_USER + 1008
#endif MAINDEF_H
