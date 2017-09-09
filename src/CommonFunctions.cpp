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
#include "Plugin.h"
#include "PluginInterface.h"

void SetString(char *&Target, const char *Str) {
  if (Target == Str)
    return;
  CLEAN_AND_ZERO_ARR(Target);
  Target = new char[strlen(Str) + 1];
  strcpy(Target, Str);
}

void SetString(wchar_t *&Target, const wchar_t *Str) {
  if (Target == Str)
    return;
  CLEAN_AND_ZERO_ARR(Target);
  Target = new wchar_t[wcslen(Str) + 1];
  wcscpy(Target, Str);
}

void SetString(char *&Target, const wchar_t *Str) {
#ifndef UNICODE
  if (Target == Str)
    return;
#endif // !UNICODE
  CLEAN_AND_ZERO_ARR(Target);
  size_t OutSize = wcslen(Str) + 1;
  size_t PrevOutSize = 0;
  Target = new char[OutSize];
  char *OutBuf = Target;
  size_t res = (size_t)-1;

#ifndef UNICODE
  strcpy(Target, Str);
#else // if UNICODE
  iconv_t Converter = iconv_open("CHAR//IGNORE", "UCS-2LE");
  size_t InSize = (wcslen(Str) + 1) * sizeof(wchar_t);

  while (*Str) {
#ifndef UNICODE
    InSize = 1;
#else
    InSize = 2;
#endif
    if (!InSize)
      break;
    PrevOutSize = OutSize;
    res = iconv(Converter, (const char **)&Str, &InSize, &OutBuf, &OutSize);
    if (PrevOutSize - OutSize > 1) {
      // If char is multichar then we count it as not converted
      OutBuf -= (PrevOutSize - OutSize);
      *(OutBuf) = 0;
      OutSize = PrevOutSize;
    }
  }

  *(OutBuf) = 0;
  iconv_close(Converter);
  if (res == (size_t)(-1)) {
    *Target = '\0';
  }
#endif // UNICODE
}

void SetString(wchar_t *&Target, const char *Str) {
#ifndef UNICODE
  if (Target == Str)
    return;
#endif // !UNICODE
  CLEAN_AND_ZERO_ARR(Target);
  size_t OutSize = sizeof(wchar_t) * (strlen(Str) + 1);
  Target = new wchar_t[OutSize];
  char *OutBuf = (char *)Target;

#ifndef UNICODE
  strcpy(Target, Str);
#else  // if UNICODE
  iconv_t Converter = iconv_open("UCS-2LE//IGNORE", "CHAR");

  size_t InSize = strlen(Str) + 1;
  size_t res =
      iconv(Converter, (const char **)&Str, &InSize, &OutBuf, &OutSize);
  iconv_close(Converter);

  if (res == (size_t)(-1)) {
    *Target = '\0';
  }
#endif // UNICODE
}

// In case source is in utf-8
void SetStringSUtf8(wchar_t *&Target, const char *Str) {
#ifndef UNICODE
  if (Target == Str)
    return;
  if (Target == Str)
    return;
  iconv_t Converter = iconv_open("CHAR", "UTF-8");
#else // !UNICODE
  iconv_t Converter = iconv_open("UCS-2LE//IGNORE", "UTF-8");
#endif // !UNICODE
  CLEAN_AND_ZERO_ARR(Target);
  size_t InSize = strlen(Str) + 1;
  size_t OutSize = (Utf8Length(Str) + 1) * sizeof(wchar_t);
  Target = new wchar_t[OutSize];
  char *OutBuf = (char *)Target;
  size_t res = iconv(Converter, &Str, &InSize, &OutBuf, &OutSize);
  iconv_close(Converter);

  if (res == (size_t)(-1)) {
    *Target = '\0';
  }
}

// In case destination is in utf-8
void SetStringDUtf8(char *&Target, const wchar_t *Str) {
#ifndef UNICODE
  if (Target == Str)
    return;
  iconv_t Converter = iconv_open("UTF-8", "CHAR");
#else // !UNICODE
  iconv_t Converter = iconv_open("UTF-8//IGNORE", "UCS-2LE");
#endif // !UNICODE
  size_t InSize = (wcslen(Str) + 1) * sizeof(wchar_t);
  size_t OutSize = 6 * wcslen(Str) + 1; // Maximum Possible UTF-8 size
  char *TempBuf = new char[OutSize];
  char *OutBuf = (char *)TempBuf;
  size_t res =
      iconv(Converter, (const char **)&Str, &InSize, &OutBuf, &OutSize);
  iconv_close(Converter);
  if (res == (size_t)(-1)) {
    Target = new char[1];
    *Target = '\0';
    CLEAN_AND_ZERO_ARR(TempBuf)
    return;
  }
  SetString(Target, TempBuf); // Cutting off unnecessary symbols.
  CLEAN_AND_ZERO_ARR(TempBuf);
}

// In case source is in utf-8
void SetStringSUtf8(char *&Target, const char *Str) {
  if (Target == Str)
    return;
  iconv_t Converter = iconv_open("CHAR//IGNORE", "UTF-8");
  CLEAN_AND_ZERO_ARR(Target);
  size_t InSize = 0;
  size_t OutSize = Utf8Length(Str) + 1;
  size_t PrevOutSize = 0;
  Target = new char[OutSize];
  size_t res = (size_t)-1;
  char *OutBuf = (char *)Target;
  while (*Str) {
    InSize = Utf8Gewchar_tSize(*Str);
    if (!InSize)
      break;
    PrevOutSize = OutSize;
    res = iconv(Converter, &Str, &InSize, &OutBuf, &OutSize);
    if (PrevOutSize - OutSize > 1) {
      // If char is multichar then we count it as not converted
      OutBuf -= (PrevOutSize - OutSize);
      *(OutBuf) = 0;
      OutSize = PrevOutSize;
    }
  }
  *(OutBuf) = 0;
  iconv_close(Converter);

  if (res == (size_t)(-1)) {
    *Target = '\0';
  }
}

// In case destination is in utf-8
void SetStringDUtf8(char *&Target, const char *Str) {
  if (Target == Str)
    return;
  iconv_t Converter = iconv_open("UTF-8//IGNORE", "CHAR");
  size_t InSize = strlen(Str) + 1;
  size_t OutSize = 6 * strlen(Str) + 1; // Maximum Possible UTF-8 size
  char *TempBuf = new char[OutSize];
  char *OutBuf = (char *)TempBuf;
  size_t res =
      iconv(Converter, (const char **)&Str, &InSize, &OutBuf, &OutSize);
  iconv_close(Converter);
  if (res == (size_t)(-1)) {
    Target = new char[1];
    *Target = '\0';
    CLEAN_AND_ZERO_ARR(TempBuf)
    return;
  }
  SetString(Target, TempBuf); // Cutting off unnecessary symbols.
  CLEAN_AND_ZERO_ARR(TempBuf);
}

// This function is more or less transferred from gcc source
BOOL MatchSpecialChar(wchar_t *Dest, wchar_t *&Source) {
  int len, i;
  wchar_t c, n;
  BOOL m;

  m = TRUE;

  switch (c = *(Source++)) {
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
    for (i = 0; i < len; i++) {
      char buf[2] = {'\0', '\0'};

      c = *(Source++);
      if (c > UCHAR_MAX ||
          !(('0' <= c && c <= '9') || ('a' <= c && c <= 'f') ||
            ('A' <= c && c <= 'F')))
        return FALSE;

      buf[0] = (unsigned char)c;
      n = n << 4;
      n += (wchar_t)strtol(buf, NULL, 16);
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

void SetParsedString(wchar_t *&Dest, wchar_t *Source) {
  Dest = new wchar_t[wcslen(Source) + 1];
  wchar_t *LastPos = 0;
  wchar_t *ResString = Dest;
  while (*Source) {
    LastPos = Source;
    if (*Source == '\\') {
      Source++;
      if (!MatchSpecialChar(Dest, Source)) {
        Source = LastPos;
        *Dest = *(Source++);
        Dest++;
      } else {
        Dest++;
      }
    } else {
      *Dest = *(Source++);
      Dest++;
    }
  }
  *Dest = 0;
  Dest = ResString;
}

// These functions are mostly taken from http://research.microsoft.com

BOOL Utf8IsLead(char c) {
  return (((c & 0x80) == 0)                          // 0xxxxxxx
          || ((c & 0xC0) == 0xC0 && (c & 0x20) == 0) // 110xxxxx
          || ((c & 0xE0) == 0xE0 && (c & 0x10) == 0) // 1110xxxx
          || ((c & 0xF0) == 0xF0 && (c & 0x08) == 0) // 11110xxx
          || ((c & 0xF8) == 0xF8 && (c & 0x04) == 0) // 111110xx
          || ((c & 0xFC) == 0xFC && (c & 0x02) == 0));
}

BOOL Utf8IsCont(char c) {
  return ((c & 0x80) == 0x80 && (c & 0x40) == 0); // 10xxxxx
}

char *Utf8Dec(const char *string, const char *current) {
  const char *temp;
  if (string >= current)
    return 0;

  temp = current - 1;
  while (string <= temp && (!Utf8IsLead(*temp)))
    temp--;

  return (char *)temp;
}

char *Utf8Inc(const char *string) {
  const char *temp;
  temp = string + 1;
  while (*temp && !Utf8IsLead(*temp))
    temp++;

  return (char *)temp;
}

int Utf8Gewchar_tSize(char c) {
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

BOOL Utf8Firswchar_tsAreEqual(const char *Str1, const char *Str2) {
  int Firswchar_tSize1 = Utf8Gewchar_tSize(*Str1);
  int Firswchar_tSize2 = Utf8Gewchar_tSize(*Str2);
  if (Firswchar_tSize1 != Firswchar_tSize2)
    return FALSE;
  return (strncmp(Str1, Str2, Firswchar_tSize1) == 0);
}

char *Utf8pbrk(const char *s, const char *set) {
  const char *x;
  for (; *s; s = Utf8Inc(s))
    for (x = set; *x; x = Utf8Inc(x))
      if (Utf8Firswchar_tsAreEqual(s, x))
        return (char *)s;
  return NULL;
}

std::ptrdiff_t Utf8spn(const char *s, const char *set) {
  const char *x;
  const char *it = 0;
  it = s;

  for (; *it; it = Utf8Inc(it)) {
    for (x = set; *x; x = Utf8Inc(x)) {
      if (Utf8Firswchar_tsAreEqual(it, x))
        goto continue_outer;
    }
    break;
  continue_outer:;
  }
  return it - s;
}

char *Utf8chr(const char *s, const char *sfc) // Char is first from the string
                                              // sfc (string with first char)
{
  while (*s) {
    if (s && Utf8Firswchar_tsAreEqual(s, sfc))
      return (char *)s;
    s = Utf8Inc(s);
  }
  return 0;
}

char *Utf8strtok(char *s1, const char *Delimit, char **Context) {
  char *tmp;

  /* Skip leading delimiters if new string. */
  if (s1 == NULL) {
    s1 = *Context;
    if (s1 == NULL) /* End of story? */
      return NULL;
    else
      s1 += Utf8spn(s1, Delimit);
  } else {
    s1 += Utf8spn(s1, Delimit);
  }

  /* Find end of segment */
  tmp = Utf8pbrk(s1, Delimit);
  if (tmp) {
    /* Found another delimiter, split string and save state. */
    *tmp = '\0';
    tmp++;
    while (!Utf8IsLead(*(tmp))) {
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

size_t Utf8Length(const char *String) {
  char *It = const_cast<char *>(String);
  size_t Size = 0;
  while (*It) {
    Size++;
    It = Utf8Inc(It);
  }
  return Size;
}

bool SortCompare(wchar_t *a, wchar_t *b) { return wcscmp(a, b) < 0; }

bool EquivCharStrings(char *a, char *b) { return (strcmp(a, b) == 0); }

size_t HashCharString(char *a) {
  size_t Hash = 7;
  for (unsigned int i = 0; i < strlen(a); i++) {
    Hash = Hash * 31 + a[i];
  }
  return Hash;
}

bool Equivwchar_tStrings(wchar_t *a, wchar_t *b) { return (wcscmp(a, b) == 0); }

size_t Hashwchar_tString(wchar_t *a) {
  size_t Hash = 7;
  for (unsigned int i = 0; i < wcslen(a); i++) {
    Hash = Hash * 31 + a[i];
  }
  return Hash;
}

bool SortCompareChars(char *a, char *b) { return strcmp(a, b) < 0; }

const wchar_t *const AliasesFrom[] = {L"af_Za",
                                      L"ak_GH",
                                      L"bg_BG",
                                      L"ca_ANY",
                                      L"ca_ES",
                                      L"cop_EG",
                                      L"cs_CZ",
                                      L"cy_GB",
                                      L"da_DK",
                                      L"de_AT",
                                      L"de_CH",
                                      L"de_DE",
                                      L"de_DE_comb",
                                      L"de_DE_frami",
                                      L"de_DE_neu",
                                      L"el_GR",
                                      L"en_AU",
                                      L"en_CA",
                                      L"en_GB",
                                      L"en_GB-oed",
                                      L"en_NZ",
                                      L"en_US",
                                      L"en_ZA",
                                      L"eo_EO",
                                      L"es_AR",
                                      L"es_BO",
                                      L"es_CL",
                                      L"es_CO",
                                      L"es_CR",
                                      L"es_CU",
                                      L"es_DO",
                                      L"es_EC",
                                      L"es_ES",
                                      L"es_GT",
                                      L"es_HN",
                                      L"es_MX",
                                      L"es_NEW",
                                      L"es_NI",
                                      L"es_PA",
                                      L"es_PE",
                                      L"es_PR",
                                      L"es_PY",
                                      L"es_SV",
                                      L"es_UY",
                                      L"es_VE",
                                      L"et_EE",
                                      L"fo_FO",
                                      L"fr_FR",
                                      L"fr_FR-1990",
                                      L"fr_FR-1990_1-3-2",
                                      L"fr_FR-classique",
                                      L"fr_FR-classique_1-3-2",
                                      L"fr_FR_1-3-2",
                                      L"fy_NL",
                                      L"ga_IE",
                                      L"gd_GB",
                                      L"gl_ES",
                                      L"gu_IN",
                                      L"he_IL",
                                      L"hi_IN",
                                      L"hil_PH",
                                      L"hr_HR",
                                      L"hu_HU",
                                      L"ia",
                                      L"id_ID",
                                      L"is_IS",
                                      L"it_IT",
                                      L"ku_TR",
                                      L"la",
                                      L"lt_LT",
                                      L"lv_LV",
                                      L"mg_MG",
                                      L"mi_NZ",
                                      L"mk_MK",
                                      L"mos_BF",
                                      L"mr_IN",
                                      L"ms_MY",
                                      L"nb_NO",
                                      L"ne_NP",
                                      L"nl_NL",
                                      L"nn_NO",
                                      L"nr_ZA",
                                      L"ns_ZA",
                                      L"ny_MW",
                                      L"oc_FR",
                                      L"pl_PL",
                                      L"pt_BR",
                                      L"pt_PT",
                                      L"ro_RO",
                                      L"ru_RU",
                                      L"ru_RU_ie",
                                      L"ru_RU_ye",
                                      L"ru_RU_yo",
                                      L"rw_RW",
                                      L"si_SI",
                                      L"sk_SK",
                                      L"sq_AL",
                                      L"ss_ZA",
                                      L"st_ZA",
                                      L"sv_SE",
                                      L"sw_KE",
                                      L"tet_ID",
                                      L"th_TH",
                                      L"tl_PH",
                                      L"tn_ZA",
                                      L"ts_ZA",
                                      L"uk_UA",
                                      L"ur_PK",
                                      L"ve_ZA",
                                      L"vi-VN",
                                      L"xh_ZA",
                                      L"zu_ZA"};
const wchar_t *const AliasesTo[] = {
    L"Afrikaans",
    L"Akan",
    L"Bulgarian",
    L"Catalan (Any)",
    L"Catalan (Spain)",
    L"Coptic (Bohairic dialect)",
    L"Czech",
    L"Welsh",
    L"Danish",
    L"German (Austria)",
    L"German (Switzerland)",
    L"German (Germany)",
    L"German (Old and New Spelling)",
    L"German (Additional)",
    L"German (New Spelling)",
    L"Greek",
    L"English (Australia)",
    L"English (Canada)",
    L"English (Great Britain)",
    L"English (Great Britain) [OED]",
    L"English (New Zealand)",
    L"English (United States)",
    L"English (South Africa)",
    L"Esperanto",
    L"Spanish (Argentina)",
    L"Spanish (Bolivia)",
    L"Spanish (Chile)",
    L"Spanish (Colombia)",
    L"Spanish (Costa Rica)",
    L"Spanish (Cuba)",
    L"Spanish (Dominican Republic)",
    L"Spanish (Ecuador)",
    L"Spanish (Spain)",
    L"Spanish (Guatemala)",
    L"Spanish (Honduras)",
    L"Spanish (Mexico)",
    L"Spanish (New)",
    L"Spanish (Nicaragua)",
    L"Spanish (Panama)",
    L"Spanish (Peru)",
    L"Spanish (Puerto Rico)",
    L"Spanish (Paraguay)",
    L"Spanish (El Salvador)",
    L"Spanish (Uruguay)",
    L"Spanish (Bolivarian Republic of Venezuela)",
    L"Estonian",
    L"Faroese",
    L"French",
    L"French (1990)",
    L"French (1990)",
    L"French (Classique)",
    L"French (Classique)",
    L"French",
    L"Frisian",
    L"Irish",
    L"Scottish Gaelic",
    L"Galician",
    L"Gujarati",
    L"Hebrew",
    L"Hindi",
    L"Filipino",
    L"Croatian",
    L"Hungarian",
    L"Interlingua",
    L"Indonesian",
    L"Icelandic",
    L"Italian",
    L"Kurdish",
    L"Latin",
    L"Lithuanian",
    L"Latvian",
    L"Malagasy",
    L"Maori",
    L"Macedonian (FYROM)",
    L"Mossi",
    L"Marathi",
    L"Malay",
    L"Norwegian (Bokmal)",
    L"Nepali",
    L"Dutch",
    L"Norwegian (Nynorsk)",
    L"Ndebele",
    L"Northern Sotho",
    L"Chichewa",
    L"Occitan",
    L"Polish",
    L"Portuguese (Brazil)",
    L"Portuguese (Portugal)",
    L"Romanian",
    L"Russian",
    L"Russian (without io)",
    L"Russian (without io)",
    L"Russian (with io)",
    L"Kinyarwanda",
    L"Slovenian",
    L"Slovak",
    L"Alban",
    L"Swazi",
    L"Northern Sotho",
    L"Swedish",
    L"Kiswahili",
    L"Tetum",
    L"Thai",
    L"Tagalog",
    L"Setswana",
    L"Tsonga",
    L"Ukrainian",
    L"Urdu",
    L"Venda",
    L"Vietnamese",
    L"isiXhosa",
    L"isiZulu"};

// Language Aliases
BOOL SetStringWithAliasApplied(wchar_t *&Target, const wchar_t *OrigName) {
  int Left, Right;
  Left = 0;
#ifdef _DEBUG
  for (int i = 0; i < static_cast<int> (countof(AliasesFrom)) - 1; i++) {
    if (wcscmp(AliasesFrom[i], AliasesFrom[i + 1]) >= 0) {
      DebugBreak();
      // String are not sorted correctly in AliasesFrom
    }
  }
#endif
  Right = countof(AliasesFrom);
  while (1) {
    int CentralElement = (Left + Right) / 2;
    int CompareResult = wcscmp(AliasesFrom[CentralElement], OrigName);
    if (CompareResult == 0) {
      SetString(Target, AliasesTo[CentralElement]);
      return TRUE;
    } else if (Right - Left <= 1)
      break;
    else if (CompareResult < 0)
      Left = CentralElement;
    else
      Right = CentralElement;
  }
  SetString(Target, OrigName);
  return FALSE;
}

static BOOL TryToCreateDir(wchar_t *Path, BOOL Silent, HWND NppWindow) {
  if (!CreateDirectory(Path, NULL)) {
    if (!Silent) {
      if (!NppWindow)
        return FALSE;

      wchar_t Message[DEFAULT_BUF_SIZE];
      swprintf(Message, L"Can't create directory %s", Path);
      MessageBox (NppWindow, Message, L"Error in directory creation",
                  MB_OK | MB_ICONERROR);
    }
    return FALSE;
  }
  return TRUE;
}

BOOL CheckForDirectoryExistence(const wchar_t* PathArg, BOOL Silent, HWND NppWindow) {
    auto Path = cpyBuf<wchar_t> (PathArg);
  for (unsigned int i = 0; i < wcslen(PathArg); i++) {
    if (Path[i] == L'\\') {
      Path[i] = L'\0';
      if (!PathFileExists(PathArg)) {
        if (!TryToCreateDir(Path.get (), Silent, NppWindow))
          return FALSE;
      }
      Path[i] = L'\\';
    }
  }
  if (!PathFileExists(PathArg)) {
    if (!TryToCreateDir(Path.get (), Silent, NppWindow))
      return FALSE;
  }
  return TRUE;
}

wchar_t *GetLastSlashPosition(wchar_t *Path) { return wcsrchr(Path, L'\\'); }

TaskWrapper::TaskWrapper(HWND targetHwnd): m_targetHwnd(targetHwnd)  {
}

TaskWrapper::~TaskWrapper() {
    cancel ();
}

void TaskWrapper::resetAliveStatus() {
    m_aliveStatus = {new concurrency::cancellation_token_source (), [](
        concurrency::cancellation_token_source *source){ source->cancel(); delete source; }};
}

void TaskWrapper::cancel() {
    if (m_aliveStatus)
        {
            m_aliveStatus->cancel();
            m_aliveStatus.reset ();
        }
}

void ProgressData::set(int progress, const std::wstring& status, bool marquee) {
    std::lock_guard<std::mutex> lg (m_mutex);
    m_progress = progress;
    m_status = status;
    m_marquee = marquee;
}

bool ProgressData::getMarquee() const {
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_marquee;
}

int ProgressData::getProgress() const {
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_progress;
}

std::wstring ProgressData::getStatus()const{
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_status;
}

