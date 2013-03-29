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
#include "CommonFunctions.h"
#include "MainDef.h"
#include "PluginInterface.h"

void SetString (char *&Target, const char *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  Target = new char[strlen (Str) + 1];
  strcpy (Target, Str);
}

void SetString (TCHAR *&Target, const TCHAR *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  Target = new TCHAR[_tcslen (Str) + 1];
  _tcscpy (Target, Str);
}

void SetString (char *&Target, const TCHAR *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  int size = _tcslen (Str) + 1;
  Target = new char[size];
#ifndef UNICODE
  strcpy (Target, Str);
#else // if UNICODE
  WideCharToMultiByte (CP_ACP, 0, Str, size, Target, size, 0, 0);
#endif // UNICODE
}

void SetString (TCHAR *&Target, const char *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  int size = strlen (Str) + 1;
  Target = new TCHAR[size];
#ifndef UNICODE
  strcpy (Target, Str);
#else // if UNICODE
  MultiByteToWideChar (CP_ACP, 0, Str, size, Target, size);
#endif // UNICODE
}

// In case source is in utf-8
void SetStringSUtf8 (TCHAR *&Target, const char *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  int StrSize = strlen (Str) + 1;
  WCHAR *WcharBuf = new WCHAR[StrSize];
  MultiByteToWideChar(CP_UTF8, 0, Str, StrSize, WcharBuf, StrSize);
#ifdef UNICODE
  Target = WcharBuf;
  return;
#else
  int TargetSize = WideCharToMultiByte (CP_ACP, 0, WcharBuf, StrSize, 0, 0, 0, 0);
  Target = new char[TargetSize];
  WideCharToMultiByte (CP_ACP, 0, WcharBuf, StrSize, Target, TargetSize, 0, 0);

  CLEAN_AND_ZERO_ARR (WcharBuf);
#endif
}

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const TCHAR *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  int StrSize = _tcslen (Str) + 1;
#ifdef UNICODE
  const WCHAR *WcharBuf = Str;
#else
  WCHAR *WcharBuf = new WCHAR[StrSize];
  MultiByteToWideChar(CP_ACP, 0, Str, TargetSize, WcharBuf, StrSize);
#endif
  int TargetSize = 0;
  TargetSize = WideCharToMultiByte (CP_UTF8, 0, WcharBuf, StrSize, 0, 0, 0, 0);
  Target = new char[TargetSize];
  WideCharToMultiByte (CP_UTF8, 0, WcharBuf, StrSize, Target, TargetSize, 0, 0);
#ifndef UNICODE
  CLEAN_AND_ZERO_ARR (WcharBuf);
#endif UNICODE
}

// In case source is in utf-8
void SetStringSUtf8 (char *&Target, const char *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  int StrSize = strlen (Str) + 1;
  WCHAR *WcharBuf = new WCHAR[StrSize];
  int TargetSize = 0;
  TargetSize = MultiByteToWideChar(CP_UTF8, 0, Str, StrSize, 0, 0);
  Target = new char[TargetSize];
  TargetSize = MultiByteToWideChar(CP_UTF8, 0, Str, StrSize, WcharBuf, TargetSize);
  WideCharToMultiByte (CP_ACP, 0, WcharBuf, TargetSize, Target, TargetSize, 0, 0);
  CLEAN_AND_ZERO_ARR (WcharBuf);
}

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const char *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  int StrSize = strlen (Str) + 1;
  WCHAR *WcharBuf = new WCHAR[StrSize];
  MultiByteToWideChar(CP_ACP, 0, Str, StrSize, WcharBuf, StrSize);
  int TargetSize = WideCharToMultiByte (CP_UTF8, 0, WcharBuf, StrSize, 0, 0, 0, 0);
  Target = new char[TargetSize];
  WideCharToMultiByte (CP_UTF8, 0, WcharBuf, StrSize, Target, TargetSize, 0, 0);
  CLEAN_AND_ZERO_ARR (WcharBuf);
}

// This function is more or less transferred from gcc source
BOOL MatchSpecialChar (TCHAR *Dest, TCHAR *&Source)
{
  int len, i;
  TCHAR c, n;
  BOOL m;

  m = TRUE;

  switch (c = *(Source++))
  {
  case 'a':
    *Dest = '\a';
    break;
  case 'b':
    *Dest = '\b';
    break;
  case 't':
    *Dest = '\t';
    break;
  case 'f':
    *Dest = '\f';
    break;
  case 'n':
    *Dest = '\n';
    break;
  case 'r':
    *Dest = '\r';
    break;
  case 'v':
    *Dest = '\v';
    break;
  case '\\':
    *Dest = '\\';
    break;
  case '0':
    *Dest = '\0';
    break;

  case 'x':
  case 'u':
  case 'U':
    /* Hexadecimal form of wide characters.  */
    len = (c == 'x' ? 2 : (c == 'u' ? 4 : 8));
    n = 0;
#ifndef UNICODE
    len = 2;
#endif
    for (i = 0; i < len; i++)
    {
      char buf[2] = { '\0', '\0' };

      c = *(Source++);
      if (c > UCHAR_MAX
        || !(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')))
        return FALSE;

      buf[0] = (unsigned char) c;
      n = n << 4;
      n += (TCHAR) strtol (buf, NULL, 16);
    }
    *Dest = n;
    break;

  default:
    /* Unknown backslash codes are simply not expanded.  */
    m = FALSE;
    break;
  }
  return m;
}

void SetParsedString (TCHAR *&Dest, TCHAR *Source)
{
  Dest = new TCHAR [_tcslen (Source) + 1];
  TCHAR *LastPos = 0;
  TCHAR *ResString = Dest;
  while (*Source)
  {
    LastPos = Source;
    if (*Source == '\\')
    {
      Source++;
      if (!MatchSpecialChar (Dest, Source))
      {
        Source = LastPos;
        *Dest = *(Source++);
        Dest++;
      }
      else
      {
        Dest++;
      }
    }
    else
    {
      *Dest = *(Source++);
      Dest++;
    }
  }
  *Dest = 0;
  Dest = ResString;
}

HWND GetScintillaWindow(const NppData *NppDataArg)
{
  int which = -1;
  SendMessage(NppDataArg->_nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
  if (which == -1)
    return NULL;
  if (which == 1)
    return NppDataArg->_scintillaSecondHandle;
  return (which == 0) ? NppDataArg->_scintillaMainHandle : NppDataArg->_scintillaSecondHandle;
}

LRESULT SendMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  HWND wEditor = GetScintillaWindow(NppDataArg);
  return SendMessage(wEditor, Msg, wParam, lParam);
}

LRESULT SendMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return SendMessage(ScintillaWindow, Msg, wParam, lParam);
}

// Remember: it's better to use PostMsg wherever possible, to avoid gui update on each message send etc etc
// Also it's better to avoid get current scintilla window too much times, since it's obviously uses 1 SendMsg call
LRESULT PostMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  HWND wEditor = GetScintillaWindow(NppDataArg);
  return PostMessage(wEditor, Msg, wParam, lParam);
}

LRESULT PostMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return PostMessage(ScintillaWindow, Msg, wParam, lParam);
}

// These functions are mostly taken from http://research.microsoft.com

BOOL Utf8IsLead (char c)
{
  return (((c & 0x80) == 0)                 // 0xxxxxxx
    || ((c & 0xC0) == 0xC0 && (c & 0x20) == 0)    // 110xxxxx
    || ((c & 0xE0) == 0xE0 && (c & 0x10) == 0)    // 1110xxxx
    || ((c & 0xF0) == 0xF0 && (c & 0x08) == 0)    // 11110xxx
    || ((c & 0xF8) == 0xF8 && (c & 0x04) == 0));  // 111110xx
}

BOOL Utf8IsCont (char c)
{
  return ((c & 0x80) == 0x80 && (c & 0x40) == 0);  // 10xxxxx
}

char *Utf8Dec(const char *string, const char *current)
{
  const char *temp;
  if (string > current)
    return 0;

  temp = current - 1;
  while ( string <= temp && (!Utf8IsLead (*temp) ))
    temp--;

  return (char *) temp;
}

char *Utf8Inc (const char *string)
{
  const char *temp;
  temp = string + 1;
  while (*temp && !Utf8IsLead (*temp))
    temp++;

  return (char *) temp;
}

int Utf8GetCharSize (char c)
{
  if ((c & 0x80) == 0)
    return 1;
  else if ((c & 0xC0) > 0 && (c & 0x20) == 0)
    return 2;
  else if ((c & 0xE0) > 0 && (c & 0x10) == 0)
    return 3;
  else if ((c & 0xE0) > 0 && (c & 0x10) == 0)
    return 4;
  else if ((c & 0xF0) > 0 && (c & 0x08) == 0)
    return 5;
  else if ((c & 0xF8) > 0 && (c & 0x04) == 0)
    return 6;
  return 0;
}

BOOL Utf8FirstCharsAreEqual (const char *Str1, const char *Str2)
{
  int FirstCharSize1 = Utf8GetCharSize (*Str1);
  int FirstCharSize2 = Utf8GetCharSize (*Str2);
  if (FirstCharSize1 != FirstCharSize2)
    return FALSE;
  return (strncmp (Str1, Str2, FirstCharSize1) == 0);
}

char *Utf8pbrk(const char *s, const char *set)
{
  const char *x;
  for (; *s; s = Utf8Inc (s))
    for (x = set; *x; x = Utf8Inc (x))
      if (Utf8FirstCharsAreEqual (s, x))
        return (char *) s;
  return NULL;
}

long Utf8spn (const char *s, const char *set)
{
  const char *x;
  size_t i;

  for (i = 0; *s; s = Utf8Inc (s), i++) {
    for (x = set; *x; x = Utf8Inc (x))
      if (Utf8FirstCharsAreEqual (s, x))
        goto continue_outer;
    break;
continue_outer:;
  }
  return i;
}

char *Utf8chr (const char *s, const char *sfc) // Char is first from the string sfc (string with first char)
{
  while (*s)
  {
    if (s && Utf8FirstCharsAreEqual (s, sfc))
      return (char *) s;
    s = Utf8Inc (s);
  }
  return 0;
}

char * Utf8strtok (char *s1, const char *Delimit, char **Context)
{
  char *tmp;

  /* Skip leading delimiters if new string. */
  if ( s1 == NULL ) {
    s1 = *Context;
    if (s1 == NULL)         /* End of story? */
      return NULL;
    else
      s1 += Utf8spn (s1, Delimit);
  } else {
    s1 += Utf8spn (s1, Delimit);
  }

  /* Find end of segment */
  tmp = Utf8pbrk (s1, Delimit);
  if (tmp) {
    /* Found another delimiter, split string and save state. */
    *tmp = '\0';
    tmp++;
    while (!Utf8IsLead (*(tmp)))
    {
      *tmp = '\0';
      tmp++;
    }

    *Context = tmp;
  } else {
    /* Last segment, remember that. */
    *Context = NULL;
  }

  return s1;
}

size_t Utf8Length (const char *String)
{
  char *It = const_cast <char *> (String);
  size_t Size = 0;
  while (*It)
  {
    Size++;
    It = Utf8Inc (It);
  }
  return Size;
}

bool SortCompare(TCHAR *a, TCHAR *b)
{
  return _tcscmp(a, b) < 0;
}
