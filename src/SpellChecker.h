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
  void RecheckModified ();
  void ErrorMsgBox (const TCHAR * message);

  void SetLanguage (const char *Str);
  void SetDelimiters (const char *Str, int SaveToIni = 1);
  void SetSuggestionsNum (int Num);
  void SetAspellPath (const TCHAR *Path);
  void SetIgnoreYo (BOOL Ignore);
  void SetCheckThose (int CheckThoseArg);
  void SetFileTypes (TCHAR *FileTypesArg);
  void SetCheckComments (BOOL Value);

private:
  enum CheckTextMode
  {
    UNDERLINE_ERRORS = 0,
    FIND_FIRST = 1,
    FIND_LAST = 2
  };

  void CreateWordUnderline (HWND ScintillaWindow, int start, int end);
  void RemoveUnderline (HWND ScintillaWindow, int start, int end);
  void ClearAllUnderlines ();
  void FindNextMistake ();
  void FindPrevMistake ();
  void ClearVisibleUnderlines ();
  void Cleanup ();
  void CheckFileName ();
  void UpdateOverridenSuggestionsBox ();
  const char *GetDelimiters ();
  const char *GetLanguage ();
  BOOL AspellReinitSettings ();
  BOOL AspellClear ();
  BOOL CheckWord (char *Word, long Start, long End);
  void GetVisibleLimits(long &Start, long &Finish);
  char *GetVisibleText(long *offset);
  char *GetDocumentText ();
  BOOL CheckText (char *TextToCheck, long offset, CheckTextMode Mode);
  void CheckVisible ();
  void setEncodingById (int EncId);
  void LoadSettings ();
  void UpdateAutocheckStatus (int SaveSetting = 1);
  void SwitchAutoCheck ();
  void ShowSuggestionsMenu ();
  void InitSuggestionsBox ();
  BOOL GetWordUnderMouseIsRight (long &Pos, long &Length);
  void SetSuggestionsBoxTransparency ();
  char *GetWordAt (long CharPos, char *Text, long Offset);
  void SetDefaultDelimiters ();
  void HideSuggestionBox ();
  void GetLimitsAndRecheckModified ();
  BOOL CheckTextNeeded ();
  int CheckWordInCommentOrString (int WordStart, int WordEnd);
  void WriteSetting ();

  void SaveToIni (const TCHAR *Name, const TCHAR *Value, BOOL InQuotes = 0);
  void SaveToIni (const TCHAR *Name, int Value);
  void SaveToIniUtf8 (const TCHAR *Name, const char *Value, BOOL InQuotes = 0);

  void LoadFromIni (TCHAR *&Value, const TCHAR *Name, const TCHAR *DefaultValue, BOOL InQuotes = 0);
  void LoadFromIni (int &Value, const TCHAR *Name, int DefaultValue);
  void LoadFromIniUtf8 (char *&Value, const TCHAR *Name, const char *DefaultValue, BOOL InQuotes = 0);

private:

  BOOL AutoCheckText;
  BOOL AspellLoaded;
  BOOL CheckTextEnabled;
  char *Language;
  int SuggestionsNum;
  char *DelimUtf8; // String without special characters but maybe with escape characters (like '\n' and stuff)
  char *DelimUtf8Converted; // String where escape characters are properly converted to corresponding symbols
  char *DelimConverted; // Same but in ANSI encoding
  TCHAR *FileTypes;
  TCHAR *AspellPath;
  BOOL IgnoreYo;
  BOOL CheckThose;
  BOOL CheckComments;

  int Lexer;
  _locale_t  utf8_l;
  long ModifiedStart;
  long ModifiedEnd;
  long WUCPosition; // WUC = Word Under Cursor (Position in global doc coordinates
  long WUCLength;
  long CurrentPosition;
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
