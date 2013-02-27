#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include "tchar.h"

void SetString (char *&Target, const char *Str);

void SetString (TCHAR *&Target, const TCHAR *Str);

void SetString (char *&Target, const TCHAR *Str);

void SetString (TCHAR *&Target, const char *Str);

// In case source is in utf-8
void SetStringSUtf8 (char *&Target, const char *Str);
void SetStringSUtf8 (TCHAR *&Target, const char *Str);

// In case destination is in utf-8
void SetStringDUtf8 (char *&Target, const char *Str);
void SetStringDUtf8 (char *&Target, const TCHAR *Str);

void SetParsedString (TCHAR *&Dest, TCHAR *Source);
#endif // COMMON_FUNCTIONS_H;
