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

#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

struct NppData;

void SetString (char *&Target, const char *Str);

void SetString (TCHAR *&Target, const TCHAR *Str);

void SetString (char *&Target, const TCHAR *Str);

void SetString (TCHAR *&Target, const char *Str);

// In case source is in UTF-8
void SetStringSUtf8 (char *&Target, const char *Str);
void SetStringSUtf8 (TCHAR *&Target, const char *Str);

// In case destination is in UTF-8
void SetStringDUtf8 (char *&Target, const char *Str);
void SetStringDUtf8 (char *&Target, const TCHAR *Str);

BOOL SetStringWithAliasApplied (TCHAR *&Target, TCHAR *OrigName);

void SetParsedString (TCHAR *&Dest, TCHAR *Source);
HWND GetScintillaWindow(const NppData *NppDataArg);
LRESULT SendMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT SendMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
void SendMsgToBothEditors (const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

LRESULT SendMsgToNpp (const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

char *Utf8Dec(const char *string, const char *current);
char *Utf8chr (const char *s, const char *sfc);
int Utf8GetCharSize (char c);
char * Utf8strtok (char *s1, const char *Delimit, char **Context);
char *Utf8Inc (const char *string);
char *Utf8pbrk(const char *s, const char *set);
size_t Utf8Length (const char *String);
BOOL Utf8IsCont (char c);

bool SortCompare (TCHAR *a, TCHAR *b);
bool EquivTCHARStrings (TCHAR *a, TCHAR *b);
size_t HashTCHARString (TCHAR *a);
bool EquivCharStrings (char *a, char *b);
size_t HashCharString (char *a);
bool SortCompareChars (char *a, char *b);

BOOL CheckForDirectoryExistence (TCHAR *Path, BOOL Silent = TRUE, HWND NppWindow = 0);
TCHAR *GetLastSlashPosition (TCHAR *Path);

#endif // COMMON_FUNCTIONS_H;
