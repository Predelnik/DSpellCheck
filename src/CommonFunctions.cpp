#include "CommonFunctions.h"
#include "MainDef.h"
#include "PluginInterface.h"

void SetString (char *&Target, const char *Str)
{
  CLEAN_AND_ZERO (Target);
  Target = new char[strlen (Str) + 1];
  strcpy (Target, Str);
}

void SetString (TCHAR *&Target, const TCHAR *Str)
{
  CLEAN_AND_ZERO (Target);
  Target = new TCHAR[_tcslen (Str) + 1];
  _tcscpy (Target, Str);
}

void SetString (char *&Target, const TCHAR *Str)
{
  CLEAN_AND_ZERO (Target);
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
  CLEAN_AND_ZERO (Target);
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
  CLEAN_AND_ZERO (Target);
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

  CLEAN_AND_ZERO (WcharBuf);
#endif
}

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const TCHAR *Str)
{
  CLEAN_AND_ZERO (Target);
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
  CLEAN_AND_ZERO (WcharBuf);
#endif UNICODE
}

// In case source is in utf-8
void SetStringSUtf8 (char *&Target, const char *Str)
{
  CLEAN_AND_ZERO (Target);
  int StrSize = strlen (Str) + 1;
  WCHAR *WcharBuf = new WCHAR[StrSize];
  int TargetSize = 0;
  TargetSize = MultiByteToWideChar(CP_UTF8, 0, Str, StrSize, 0, 0);
  Target = new char[TargetSize];
  TargetSize = MultiByteToWideChar(CP_UTF8, 0, Str, StrSize, WcharBuf, TargetSize);
  WideCharToMultiByte (CP_ACP, 0, WcharBuf, TargetSize, Target, TargetSize, 0, 0);
  CLEAN_AND_ZERO (WcharBuf);
}

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const char *Str)
{
  CLEAN_AND_ZERO (Target);
  int StrSize = strlen (Str) + 1;
  WCHAR *WcharBuf = new WCHAR[StrSize];
  MultiByteToWideChar(CP_ACP, 0, Str, StrSize, WcharBuf, StrSize);
  int TargetSize = WideCharToMultiByte (CP_UTF8, 0, WcharBuf, StrSize, 0, 0, 0, 0);
  Target = new char[TargetSize];
  WideCharToMultiByte (CP_UTF8, 0, WcharBuf, StrSize, Target, TargetSize, 0, 0);
  CLEAN_AND_ZERO (WcharBuf);
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
  // Get the current scintilla
  int which = -1;
  SendMessage(NppDataArg->_nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
  if (which == -1)
    return NULL;
  return (which == 0)?NppDataArg->_scintillaMainHandle:NppDataArg->_scintillaSecondHandle;
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
