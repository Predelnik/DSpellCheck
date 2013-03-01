#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

struct NppData;

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
LRESULT SendMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
#endif // COMMON_FUNCTIONS_H;
