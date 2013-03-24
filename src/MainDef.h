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
#define countof(A) sizeof(A)/sizeof((A)[0])
#define DEFAULT_BUF_SIZE 4096

#define SUGGESTIONS_BOX 0
#define SUGGESTIONS_CONTEXT_MENU 1

// Global Menu ID
#define DSPELLCHECK_MENU_ID 193

// Menu item IDs
#define MID_ADDTODICTIONARY 101
#define MID_IGNOREALL 102

// Custom WMs (Only for our windows and threads)
#define WM_SHOWANDRECREATEMENU WM_USER + 1000
#define TM_MODIFIED_ZONE_INFO  WM_USER + 1001
#define TM_WRITE_SETTING       WM_USER + 1002
#define TM_MENU_RESULT         WM_USER + 1004
#define TM_PRECALCULATE_MENU   WM_USER + 1005
#endif MAINDEF_H
