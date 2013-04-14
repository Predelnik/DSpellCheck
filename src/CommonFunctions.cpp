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
#include "iconv.h"
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
  size_t OutSize = _tcslen (Str) + 1;
  Target = new char[OutSize];
  char *OutBuf = Target;

#ifndef UNICODE
  strcpy (Target, Str);
#else // if UNICODE
  iconv_t Converter = iconv_open ("CHAR", "UCS-2LE");

  size_t InSize = (_tcslen (Str) + 1) * sizeof (TCHAR);
  size_t res = iconv (Converter, (const char **) &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);
  if (res == (size_t)(-1))
  {
    *Target = '\0';
  }
#endif // UNICODE
}

void SetString (TCHAR *&Target, const char *Str)
{
  CLEAN_AND_ZERO_ARR (Target);
  size_t OutSize = sizeof (TCHAR) * (strlen (Str) + 1);
  Target = new TCHAR [OutSize];
  char *OutBuf = (char *) Target;

#ifndef UNICODE
  strcpy (Target, Str);
#else // if UNICODE
  iconv_t Converter = iconv_open ("UCS-2LE", "CHAR");

  size_t InSize = strlen (Str) + 1;
  size_t res = iconv (Converter, (const char **) &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);

  if (res == (size_t)(-1))
  {
    *Target = '\0';
  }
#endif // UNICODE
}

// In case source is in utf-8
void SetStringSUtf8 (TCHAR *&Target, const char *Str)
{
  #ifndef UNICODE
  iconv_t Converter = iconv_open ("CHAR", "UTF-8");
  #else // !UNICODE
  iconv_t Converter = iconv_open ("UCS-2LE", "UTF-8");
  #endif // !UNICODE
  CLEAN_AND_ZERO_ARR (Target);
  size_t InSize = strlen (Str) + 1;
  size_t OutSize = (Utf8Length (Str) + 1) * sizeof (TCHAR);
  Target = new TCHAR[OutSize];
  char *OutBuf = (char *) Target;
  size_t res = iconv (Converter, &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);

  if (res == (size_t)(-1))
  {
    *Target = '\0';
  }
}

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const TCHAR *Str)
{
  #ifndef UNICODE
  iconv_t Converter = iconv_open ("UTF-8", "CHAR");
  #else // !UNICODE
  iconv_t Converter = iconv_open ("UTF-8", "UCS-2LE");
  #endif // !UNICODE
  size_t InSize = (_tcslen (Str) + 1) * sizeof (TCHAR);
  size_t OutSize = 6 * _tcslen (Str) + 1; // Maximum Possible UTF-8 size
  char *TempBuf = new char[OutSize];
  char *OutBuf = (char *) TempBuf;
  size_t res = iconv (Converter, (const char **) &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);
  if (res == (size_t)(-1))
  {
    Target = new char[1];
    *Target = '\0';
    CLEAN_AND_ZERO_ARR (TempBuf)
    return;
  }
  SetString (Target, TempBuf); // Cutting off unnecessary symbols.
  CLEAN_AND_ZERO_ARR (TempBuf);
}

// In case source is in utf-8
void SetStringSUtf8 (char *&Target, const char *Str)
{
  iconv_t Converter = iconv_open ("CHAR", "UTF-8");
  CLEAN_AND_ZERO_ARR (Target);
  size_t InSize = strlen (Str) + 1;
  size_t OutSize = Utf8Length (Str) + 1;
  Target = new char[OutSize];
  char *OutBuf = (char *) Target;
  size_t res = iconv (Converter, &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);

  if (res == (size_t)(-1))
  {
    *Target = '\0';
  }
}

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const char *Str)
{
  iconv_t Converter = iconv_open ("UTF-8", "CHAR");
  size_t InSize = strlen (Str) + 1;
  size_t OutSize = 6 * strlen (Str) + 1; // Maximum Possible UTF-8 size
  char *TempBuf = new char[OutSize];
  char *OutBuf = (char *) TempBuf;
  size_t res = iconv (Converter, (const char **) &Str, &InSize, &OutBuf, &OutSize);
  iconv_close (Converter);
  if (res == (size_t)(-1))
  {
    Target = new char[1];
    *Target = '\0';
    CLEAN_AND_ZERO_ARR (TempBuf)
    return;
  }
  SetString (Target, TempBuf); // Cutting off unnecessary symbols.
  CLEAN_AND_ZERO_ARR (TempBuf);
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
  const char *it = 0;
  it = s;

  for (; *it; it = Utf8Inc (it)) {
    for (x = set; *x; x = Utf8Inc (x))
    {
      if (Utf8FirstCharsAreEqual (it, x))
        goto continue_outer;
    }
    break;
continue_outer:;
  }
  return it - s;
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
  return _tcscmp (a, b) < 0;
}

bool SortCompareChars (char *a, char *b)
{
  return strcmp (a, b) < 0;
}
