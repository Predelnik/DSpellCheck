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
  void SetLanguage (const char *Str);
  void SetDelimiters (const char *Str);
  void RecheckVisible ();
  void ErrorMsgBox (const TCHAR * message);

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
  void UpdateAutocheckStatus ();
  void SwitchAutoCheck ();
  void InitSuggestions ();
  char *GetWordUnderMouse ();
  char *GetWordAt (int CharPos, char *Text);

private:

  BOOL AutoCheckText;
  NppData *NppDataInstance;
  BOOL ConvertingIsNeeded;
  TCHAR *IniFilePath;
  SettingsDlg *SettingsDlgInstance;
  char *Language;
  AspellSpeller *Speller;
  Suggestions *SuggestionsInstance;
  char *VisibleText;
  int VisibleTextLength;
  long VisibleTextOffset;
  char *DelimUtf8; // String without special characters but maybe with escape characters (like '\n' and stuff)
  char *DelimUtf8Converted; // String where escape characters are properly converted to corrsponding symbols
};
#endif // SPELLCHECKER_H
