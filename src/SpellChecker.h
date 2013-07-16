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

#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H
// Class that will do most of the job with spellchecker

#include "MainDef.h"

struct AspellSpeller;
struct LanguageName;
struct AspellWordList;
class SettingsDlg;
class LangList;
class Suggestions;
class AbstractSpellerInterface;
class AspellInterface;
class HunspellInterface;
class SelectProxy;

struct SuggestionsMenuItem
{
  TCHAR *Text;
  BYTE Id;
  BOOL Separator;
  SuggestionsMenuItem (TCHAR *TextArg, BYTE IdArg, BOOL SeparatorArg = FALSE)
  {
    Text = 0;
    SetString (Text, TextArg);
    Id = IdArg;
    Separator = SeparatorArg;
  }
  ~SuggestionsMenuItem ()
  {
    CLEAN_AND_ZERO_ARR (Text);
  };
};

void InsertSuggMenuItem (HMENU Menu, TCHAR *Text, BYTE Id, int InsertPos, BOOL Separator = FALSE);

class SpellChecker
{
public:
  SpellChecker (const TCHAR *IniFilePathArg, SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
    Suggestions *SuggestionsInstanceArg, LangList *LangListInstanceArg);
  ~SpellChecker ();
  void RecheckVisibleBothViews ();
  BOOL WINAPI NotifyEvent (DWORD Event);
  BOOL WINAPI NotifyNetworkEvent (DWORD Event);
  BOOL WINAPI NotifyMessage (UINT Msg, WPARAM wParam, LPARAM lParam);
  void RecheckVisible (BOOL NotIntersectionOnly = FALSE);
  void RecheckModified ();
  void ErrorMsgBox (const TCHAR * message);

  BOOL AspellReinitSettings ();
  void SetHunspellLanguage (const TCHAR *Str);
  void SetAspellLanguage (const TCHAR *Str);
  void SetDelimiters (const char *Str);
  void SetSuggestionsNum (int Num);
  void SetAspellPath (const TCHAR *Path);
  void SetMultipleLanguages (const TCHAR *MultiString, AbstractSpellerInterface *Speller);
  void SetHunspellPath (const TCHAR *Path);
  void SetHunspellAdditionalPath (const TCHAR *Path);
  void SetConversionOptions (BOOL ConvertYo, BOOL ConvertSingleQuotesArg, BOOL RemoveBoundaryApostrophesArg);
  void SetCheckThose (int CheckThoseArg);
  void SetFileTypes (TCHAR *FileTypesArg);
  void SetCheckComments (BOOL Value);
  void SetHunspellMultipleLanguages (const char *MultiLanguagesArg);
  void SetAspellMultipleLanguages (const char *MultiLanguagesArg);
  void SetUnderlineColor (int Value);
  void SetUnderlineStyle (int Value);
  void SetProxyUserName (TCHAR *Str);
  void SetProxyHostName (TCHAR *Str);
  void SetProxyPassword (TCHAR *Str);
  void SetProxyPort (int Value);
  void SetUseProxy (BOOL Value);
  void SetProxyAnonymous (BOOL Value);
  void SetProxyType (int Value);
  void SetIgnore (BOOL IgnoreNumbersArg, BOOL IgnoreCStartArg, BOOL IgnoreCHaveArg, BOOL IgnoreCAllArg,
    BOOL Ignore_Arg, BOOL IgnoreSEApostropheArg, BOOL IgnoreOneLetterArg);
  void SetSuggBoxSettings (int Size, int Transparency, int SaveIni = 1);
  void SetBufferSize (int Size, BOOL SaveToIni = 1);
  void SetSuggType (int SuggType);
  void SetLibMode (int i);
  void SetDecodeNames (BOOL Value);
  void SetOneUserDic (BOOL Value);
  BOOL GetOneUserDic ();
  void SetShowOnlyKnow (BOOL Value);
  void SetInstallSystem (BOOL Value);
  void FillDialogs (BOOL NoDisplayCall = FALSE);
  void ReinitLanguageLists ();
  TCHAR *GetHunspellPath () {return HunspellPath; };
  TCHAR *GetHunspellAdditionalPath () {return AdditionalHunspellPath; };
  TCHAR *GetLangByIndex (int i);
  BOOL GetShowOnlyKnown ();
  BOOL GetInstallSystem ();
  BOOL GetDecodeNames ();
  void DoPluginMenuInclusion (BOOL Invalidate = FALSE);
  HunspellInterface *GetHunspellSpeller () {return HunspellSpeller; };
  int GetLibMode ();
  BOOL HunspellReinitSettings (BOOL ResetDirectory);
  void SetRemoveUserDics (BOOL Value);
  void SetRemoveSystem (BOOL Value);
  BOOL GetRemoveUserDics ();
  BOOL GetRemoveSystem ();
  TCHAR *GetProxyUserName ();
  TCHAR *GetProxyHostName ();
  TCHAR *GetProxyPassword ();
  int GetProxyPort ();
  BOOL GetUseProxy ();
  BOOL GetProxyAnonymous ();
  int GetProxyType ();
  long PreviousA, PreviousB;

private:
  enum CheckTextMode
  {
    UNDERLINE_ERRORS = 0,
    FIND_FIRST = 1,
    FIND_LAST = 2
  };

  HWND GetCurrentScintilla ();
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
  const TCHAR *GetLanguage ();
  void CallLangContextMenu ();
  void GetDefaultHunspellPath (TCHAR *&Path);
  BOOL CheckWord (char *Word, long Start, long End);
  void GetVisibleLimits(long &Start, long &Finish);
  char *GetVisibleText(long *offset, BOOL NotIntersectionOnly = FALSE);
  char *GetDocumentText ();
  BOOL CheckText (char *TextToCheck, long offset, CheckTextMode Mode);
  void CheckVisible (BOOL NotIntersectionOnly = FALSE);
  void setEncodingById (int EncId);
  void SaveSettings ();
  void LoadSettings ();
  void UpdateAutocheckStatus (int SaveSetting = 1);
  void SwitchAutoCheck ();
  void FillSuggestionsMenu (HMENU Menu);
  void ProcessMenuResult (UINT MenuId);
  void InitSuggestionsBox ();
  BOOL GetWordUnderCursorIsRight (long &Pos, long &Length, BOOL UseTextCursor = FALSE);
  void SetSuggestionsBoxTransparency ();
  char *GetWordAt (long CharPos, char *Text, long Offset);
  void SetDefaultDelimiters ();
  void HideSuggestionBox ();
  void GetLimitsAndRecheckModified ();
  BOOL CheckTextNeeded ();
  int CheckWordInCommentOrString (int Style);
  void WriteSetting ();
  int GetStyle (int Pos);
  void RefreshUnderlineStyle ();
  void WriteSetting (LPARAM lParam);
  void ApplyConversions (char *Word);
  void PrepareStringForConversion ();
  void ResetDownloadCombobox ();
  void PreserveCurrentAddressIndex ();
  void FillDownloadDics ();
  void ResetHotSpotCache ();
  void CheckSpecialDelimeters (char *&Word, const char *TextStart, long &WordStart, long &WordEnd);

  void SaveToIni (const TCHAR *Name, const TCHAR *Value, const TCHAR * DefaultValue, BOOL InQuotes = 0);
  void SaveToIni (const TCHAR *Name, int Value, int DefaultValue);
  void SaveToIniUtf8 (const TCHAR *Name, const char *Value, const char * DefaultValue, BOOL InQuotes = 0);

  void LoadFromIni (TCHAR *&Value, const TCHAR *Name, const TCHAR *DefaultValue, BOOL InQuotes = 0);
  void LoadFromIni (int &Value, const TCHAR *Name, int DefaultValue);
  void LoadFromIniUtf8 (char *&Value, const TCHAR *Name, const char *DefaultValue, BOOL InQuotes = 0);

private:

  std::vector <LanguageName> *CurrentLangs;
  BOOL SettingsLoaded;
  BOOL OneUserDic;
  BOOL AutoCheckText;
  BOOL CheckTextEnabled;
  BOOL WUCisRight;
  TCHAR *HunspellLanguage;
  TCHAR *HunspellMultiLanguages;
  TCHAR *AspellLanguage;
  TCHAR *AspellMultiLanguages;
  int LibMode; // 0 - Aspell, 1 - Hunspell
  int MultiLangMode;
  int SuggestionsNum;
  int SuggestionsMode;
  char *DelimUtf8; // String without special characters but maybe with escape characters (like '\n' and stuff)
  char *DelimUtf8Converted; // String where escape characters are properly converted to corresponding symbols
  char *DelimConverted; // Same but in ANSI encoding
  TCHAR *ServerNames[3]; // Only user ones, there'll also be bunch of predetermined ones
  TCHAR *DefaultServers[3];
  int LastUsedAddress; // equals USER_SERVER_CONST + num if user address is used, otherwise equals number of default server
  int AddressIsSet;
  TCHAR *FileTypes;
  TCHAR *AspellPath;
  TCHAR *HunspellPath;
  TCHAR *AdditionalHunspellPath;
  BOOL IgnoreYo;
  BOOL ConvertSingleQuotes;
  BOOL RemoveBoundaryApostrophes;
  BOOL CheckThose;
  BOOL CheckComments;
  int UnderlineColor;
  int UnderlineStyle;
  BOOL IgnoreNumbers;
  BOOL IgnoreCStart;
  BOOL IgnoreCHave;
  BOOL IgnoreCAll;
  BOOL Ignore_;
  BOOL IgnoreSEApostrophe;
  BOOL IgnoreOneLetter;
  BOOL DecodeNames;
  BOOL ShowOnlyKnown;
  BOOL InstallSystem;
  int SBSize;
  int SBTrans;
  int BufferSize;
  const AspellWordList *CurWordList;
  HWND CurrentScintilla;
  int HotSpotCache[256]; // STYLE_MAX = 255
  std::map<TCHAR *, DWORD, bool (*)(TCHAR *, TCHAR *)> *SettingsToSave;
  BOOL UseProxy;
  BOOL ProxyAnonymous;
  int ProxyType;
  TCHAR *ProxyHostName;
  TCHAR *ProxyUserName;
  int ProxyPort;
  TCHAR *ProxyPassword;

  int Lexer;
  std::vector <SuggestionsMenuItem *> *SuggestionMenuItems;
  std::vector <char *> *LastSuggestions;
  _locale_t  utf8_l;
  long ModifiedStart;
  long ModifiedEnd;
  long WUCPosition; // WUC = Word Under Cursor (Position in global doc coordinates),
  long WUCLength;
  long CurrentPosition;
  NppData *NppDataInstance;
  EncodingType CurrentEncoding;
  TCHAR *IniFilePath;
  char *SelectedWord;
  SettingsDlg *SettingsDlgInstance;
  Suggestions *SuggestionsInstance;
  LangList *LangListInstance;
  char *VisibleText;
  int VisibleTextLength;
  long VisibleTextOffset;
  BOOL RemoveUserDics;
  BOOL RemoveSystem;

  AbstractSpellerInterface *CurrentSpeller;
  AspellInterface *AspellSpeller;
  HunspellInterface *HunspellSpeller;

  // 6 - is arbitrary maximum size, actually almost everywhere it's 1
  char *YoANSI;
  char *YeANSI;
  char *yoANSI;
  char *yeANSI;
  char *PunctuationApostropheANSI;
};
#endif // SPELLCHECKER_H
