#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H
// Class that will do most of the job with spellchecker

class SettingsDlg;
class Suggestions;

class SpellChecker
{
public:
  SpellChecker (const TCHAR *IniFilePathArg, SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
    Suggestions *SuggestionsInstanceArg);
  ~SpellChecker ();
  BOOL WINAPI NotifyEvent (DWORD Event);
  void RecheckVisible ();
  void ErrorMsgBox (const TCHAR * message);

  void SetLanguage (const char *Str);
  void SetDelimiters (const char *Str, int SaveToIni = 1);
  void SetSuggestionsNum (int Num);

private:
  void CreateWordUnderline (HWND ScintillaWindow, int start, int end);
  void RemoveWordUnderline (int start, int end);
  void ClearAllUnderlines ();
  void Cleanup ();
  const char *GetDelimiters ();
  const char *GetLanguage ();
  BOOL AspellReinitSettings ();
  BOOL AspellClear ();
  BOOL CheckWord (char *Word);
  char *GetVisibleText(long *offset);
  char *GetDocumentText ();
  void CheckText (char *TextToCheck, long offset); // Called only inside listening thread
  void CheckVisible ();
  void setEncodingById (int EncId);
  void LoadSettings ();
  void UpdateAutocheckStatus (int SaveSetting = 1);
  void SwitchAutoCheck ();
  void ShowSuggestionsMenu ();
  void InitSuggestionsBox ();
  char *GetWordUnderMouse ();
  char *GetWordAt (int CharPos, char *Text);

  void SaveToIni (const TCHAR *Name, const TCHAR *Value, BOOL InQuotes = 0);
  void SaveToIni (const TCHAR *Name, int Value);
  void SaveToIniUtf8 (const TCHAR *Name, const char *Value, BOOL InQuotes = 0);

  void LoadFromIni (TCHAR *&Value, const TCHAR *Name, const TCHAR *DefaultValue, BOOL InQuotes = 0);
  void LoadFromIni (int &Value, const TCHAR *Name, int DefaultValue);
  void LoadFromIniUtf8 (char *&Value, const TCHAR *Name, const char *DefaultValue, BOOL InQuotes = 0);

private:

  BOOL AutoCheckText;
  char *Language;
  int SuggestionsNum;
  char *DelimUtf8; // String without special characters but maybe with escape characters (like '\n' and stuff)
  char *DelimUtf8Converted; // String where escape characters are properly converted to corresponding symbols

  int WUCPosition; // WUC = Word Under Cursor (Position in global doc coordinates
  int WUCLength;
  NppData *NppDataInstance;
  BOOL ConvertingIsNeeded;
  TCHAR *IniFilePath;
  SettingsDlg *SettingsDlgInstance;
  AspellSpeller *Speller;
  Suggestions *SuggestionsInstance;
  char *VisibleText;
  int VisibleTextLength;
  long VisibleTextOffset;
};
#endif // SPELLCHECKER_H
