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
  iconv_t Converter = iconv_open ("CHAR//IGNORE", "UCS-2LE");

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
  iconv_t Converter = iconv_open ("UCS-2LE//IGNORE", "CHAR");

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
  iconv_t Converter = iconv_open ("UCS-2LE//IGNORE", "UTF-8");
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
  iconv_t Converter = iconv_open ("UTF-8//IGNORE", "UCS-2LE");
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
  iconv_t Converter = iconv_open ("CHAR//IGNORE", "UTF-8");
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
  iconv_t Converter = iconv_open ("UTF-8//IGNORE", "CHAR");
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

LRESULT SendMsgToNpp (const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  return SendMessage(NppDataArg->_nppHandle, Msg, wParam, lParam);
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
  return (((c & 0x80) == 0)                       // 0xxxxxxx
    || ((c & 0xC0) == 0xC0 && (c & 0x20) == 0)    // 110xxxxx
    || ((c & 0xE0) == 0xE0 && (c & 0x10) == 0)    // 1110xxxx
    || ((c & 0xF0) == 0xF0 && (c & 0x08) == 0)    // 11110xxx
    || ((c & 0xF8) == 0xF8 && (c & 0x04) == 0)   // 111110xx
    || ((c & 0xFC) == 0xFC && (c & 0x02) == 0));
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
  else if ((c & 0xF0) > 0 && (c & 0x08) == 0)
    return 4;
  else if ((c & 0xF8) > 0 && (c & 0x04) == 0)
    return 5;
  else if ((c & 0xFC) > 0 && (c & 0x02) == 0)
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

const TCHAR *const AliasesFrom[] = {_T ("af_Za"), _T ("ak_GH"), _T ("bg_BG"), _T ("ca_ES"), _T ("ca_ANY"), _T ("cop_EG"), _T ("cs_CZ"), _T ("cy_GB"), _T ("da_DK"), _T ("de_AT"), _T ("de_CH"), _T ("de_DE"),
  _T ("de_DE_comb"), _T ("de_DE_neu"), _T ("de_DE_frami"), _T ("el_GR"), _T ("en_AU"), _T ("en_CA"), _T ("en_GB"), _T ("en_GB-oed"), _T ("en_NZ"), _T ("en_US"), _T ("en_ZA"), _T ("eo_EO"), _T ("es_AR"),
  _T ("es_BO"), _T ("es_CL"), _T ("es_CO"), _T ("es_CR"), _T ("es_CU"), _T ("es_DO"), _T ("es_EC"), _T ("es_ES"), _T ("es_GT"), _T ("es_HN"), _T ("es_MX"), _T ("es_NEW"), _T ("es_NI"), _T ("es_PA"), _T ("es_PE"),
  _T ("es_PR"), _T ("es_PY"), _T ("es_SV"), _T ("es_UY"), _T ("es_VE"), _T ("et_EE"), _T ("fi_FI"), _T ("fo_FO"), _T ("fr_FR"), _T ("fr_FR-1990_1-3-2"), _T ("fr_FR-classique_1-3-2"), _T ("fr_FR_1-3-2"), _T ("fy_NL"),
  _T ("ga_IE"), _T ("gd_GB"), _T ("gl_ES"), _T ("gu_IN"), _T ("he_IL"), _T ("hi_IN"), _T ("hil_PH"), _T ("hr_HR"), _T ("hu_HU"), _T ("ia"), _T ("id_ID"), _T ("is_IS"), _T ("it_IT"), _T ("ku_TR"),
  _T ("la"), _T ("lt_LT"), _T ("lv_LV"), _T ("mg_MG"), _T ("mi_NZ"), _T ("mk_MK"), _T ("mos_BF"), _T ("mr_IN"), _T ("ms_MY"), _T ("nb_NO"), _T ("ne_NP"), _T ("nl_NL"), _T ("nn_NO"), _T ("nr_ZA"),
  _T ("ns_ZA"), _T ("ny_MW"), _T ("oc_FR"), _T ("pl_PL"), _T ("pt_BR"), _T ("pt_PT"), _T ("ro_RO"), _T ("ru_RU"), _T ("ru_RU_ie"), _T ("ru_RU_ye"), _T ("ru_RU_yo"), _T ("rw_RW"), _T ("sk_SK"), _T ("si_SI"), _T ("sq_AL"),
  _T ("ss_ZA"), _T ("st_ZA"), _T ("sv_SE"), _T ("sw_KE"),  _T ("tet_ID"), _T ("th_TH"), _T ("tl_PH"), _T ("tn_ZA"), _T ("ts_ZA"), _T ("uk_UA"), _T ("ur_PK"), _T ("ve_ZA"), _T ("vi-VN"), _T ("xh_ZA"), _T ("zu_ZA")
};
const TCHAR *const AliasesTo[] = {_T ("Afrikaans"), _T ("Akan"), _T ("Bulgarian"), _T ("Catalan (Spain)"), _T ("Catalan (Any)"), _T ("Coptic (Bohairic dialect)"), _T ("Czech"), _T ("Welsh"),
  _T ("Danish"), _T ("German (Austria)"), _T ("German (Switzerland)"), _T ("German (Germany)"), _T ("German (Old and New Spelling)"), _T ("German (New Spelling)"),
  _T ("German (Additional)"), _T ("Greek"), _T ("English (Australia)"), _T ("English (Canada)"), _T ("English (Great Britain)"), _T ("English (Great Britain) [OED]"), _T ("English (New Zealand)"),
  _T ("English (United States)"), _T ("English (South Africa)"), _T ("Esperanto"), _T ("Spanish (Argentina)"), _T ("Spanish (Bolivia)"), _T ("Spanish (Chile)"), _T ("Spanish (Colombia)"), _T ("Spanish (Costa Rica)"),
  _T ("Spanish (Cuba)"), _T ("Spanish (Dominican Republic)"), _T ("Spanish (Ecuador)"), _T ("Spanish (Spain)"), _T ("Spanish (Guatemala)"), _T ("Spanish (Honduras)"), _T ("Spanish (Mexico)"),
  _T ("Spanish (New)"), _T ("Spanish (Nicaragua)"), _T ("Spanish (Panama)"), _T ("Spanish (Peru)"), _T ("Spanish (Puerto Rico)"), _T ("Spanish (Paraguay)"), _T ("Spanish (El Salvador)"), _T ("Spanish (Uruguay)"),
  _T ("Spanish (Bolivarian Republic of Venezuela)"), _T ("Estonian"), _T ("Finnish"), _T ("Faroese"), _T ("French"), _T ("French (1990)"), _T ("French (Classique)"), _T ("French"), _T ("Frisian"), _T ("Irish"), _T ("Scottish Gaelic"),
  _T ("Galician"), _T ("Gujarati"), _T ("Hebrew"), _T ("Hindi"), _T ("Filipino"), _T ("Croatian"), _T ("Hungarian"), _T ("Interlingua"), _T ("Indonesian"), _T ("Icelandic"), _T ("Italian"), _T ("Kurdish"),
  _T ("Latin"), _T ("Lithuanian"), _T ("Latvian"), _T ("Malagasy"), _T ("Maori"), _T ("Macedonian (FYROM)"), _T ("Mossi"), _T ("Marathi"), _T ("Malay"), _T ("Norwegian (Bokmal)"), _T ("Nepali"), _T ("Dutch"),
  _T ("Norwegian (Nynorsk)"), _T ("Ndebele"), _T ("Northern Sotho"), _T ("Chichewa"), _T ("Occitan"), _T ("Polish"), _T ("Portuguese (Brazil)"), _T ("Portuguese (Portugal)"), _T ("Romanian"), _T ("Russian"),
  _T ("Russian (without io)"), _T ("Russian (without io)"), _T ("Russian (with io)"), _T ("Kinyarwanda"), _T ("Slovak"), _T ("Slovenian"), _T ("Alban"), _T ("Swazi"), _T ("Northern Sotho"), _T ("Swedish"),
  _T ("Kiswahili"), _T ("Tetum"), _T ("Thai"), _T ("Tagalog"), _T ("Setswana"), _T ("Tsonga"), _T ("Ukrainian"), _T ("Urdu"), _T ("Venda"), _T ("Vietnamese"), _T ("isiXhosa"), _T ("isiZulu")
};

// Language Aliases
BOOL SetStringWithAliasApplied (TCHAR *&Target, TCHAR *OrigName)
{
  int Left, Right;
  Left = 0;
  Right = countof (AliasesFrom);
  while (1)
  {
    int CentralElement = (Left + Right) / 2;
    int CompareResult = _tcscmp (AliasesFrom[CentralElement], OrigName);
    if (CompareResult == 0)
    {
      SetString (Target, AliasesTo[CentralElement]);
      return TRUE;
    }
    else if (Right - Left <= 1)
      break;
    else if (CompareResult < 0)
      Left = CentralElement;
    else
      Right = CentralElement;
  }
  SetString (Target, OrigName);
  return FALSE;
}