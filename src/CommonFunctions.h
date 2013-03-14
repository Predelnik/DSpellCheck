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
HWND GetScintillaWindow(const NppData *NppDataArg);
LRESULT SendMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT SendMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToEditor(const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToEditor(HWND ScintillaWindow, const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

char *Utf8Dec(const char *string, const char *current);
char *Utf8chr (const char *s, const char *sfc);
char * Utf8strtok (char *s1, const char *Delimit, char **Context);
char *Utf8Inc (const char *string);
#endif // COMMON_FUNCTIONS_H;
