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

class SettingsDlg;
class LangListDialog;
class Suggestions;
class SpellerController;
class AspellController;
class HunspellController;
class SelectProxy;
class Settings;

struct AspellSpeller;
struct LanguageName;
struct AspellWordList;
struct SpellerSettings;
struct SettingsData;

enum class SpellerType;

#define DEFAULT_DELIMITERS L",.!?\":;{}()[]\\/=+-^$*<>|#$@%&~\u2026\u2116\u2014\u00AB\u00BB\u2013\u2022\u00A9\u203A\u201C\u201D\u00B7\u00A0\u0060\u2192\u00d7"

#include "LanguageName.h"

enum class AspellStatus
{
  fail,
  ok,
};

struct SpellCheckerStatus
{
  std::vector<LanguageName> languageList;
  AspellStatus aspellStatus = AspellStatus::fail;
};

struct SuggestionsMenuItem
{
  TCHAR *Text;
  BYTE Id;
  BOOL Separator;
  SuggestionsMenuItem (TCHAR *TextArg, BYTE IdArg, BOOL SeparatorArg = FALSE);
  ~SuggestionsMenuItem ()
  {
    CLEAN_AND_ZERO_ARR (Text);
  };
};

void InsertSuggMenuItem (HMENU Menu, TCHAR *Text, BYTE Id, int InsertPos, BOOL Separator = FALSE);

HWND GetScintillaWindow(const NppData *NppDataArg);
LRESULT SendMsgToActiveEditor(BOOL *ok, HWND ScintillaWindow, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
BOOL SendMsgToBothEditors (const NppData *NppDataArg, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToActiveEditor(HWND ScintillaWindow, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);

class SpellChecker
{
public:
  SpellChecker (const TCHAR *IniFilePathArg, SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
    Suggestions *SuggestionsInstanceArg, LangListDialog *LangListInstanceArg);
  ~SpellChecker ();
  void RecheckVisibleBothViews ();
  BOOL WINAPI NotifyEvent (DWORD Event);

  void onHunspellDictionariesChange ();

  BOOL WINAPI NotifyNetworkEvent (DWORD Event);
  BOOL WINAPI NotifyMessage (UINT Msg, WPARAM wParam, LPARAM lParam);
  void RecheckVisible (BOOL NotIntersectionOnly = FALSE);
  void RecheckModified ();
  void ErrorMsgBox (const TCHAR * message);

  void AspellReinitSettings ();
  void setDelimiters (const wchar_t *str);
  void SetMultipleLanguages (const TCHAR *MultiString, SpellerController *Speller);
  void SetProxyUserName (TCHAR *Str);
  void SetProxyHostName (TCHAR *Str);
  void SetProxyPassword (TCHAR *Str);
  void SetProxyPort (int Value);
  void SetUseProxy (BOOL Value);
  void SetProxyAnonymous (BOOL Value);
  void SetProxyType (int Value);
  void SetShowOnlyKnow (BOOL Value);
  void SetInstallSystem (BOOL Value);
  void reinitLanguageLists ();
  TCHAR *GetLangByIndex (int i);
  BOOL GetShowOnlyKnown ();
  BOOL GetInstallSystem ();
  void DoPluginMenuInclusion (BOOL Invalidate = FALSE);
  HunspellController *GetHunspellSpeller ();;
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
  void SetSuggestionsBoxTransparency ();

private:
  enum CheckTextMode
  {
    UNDERLINE_ERRORS = 0,
    FIND_FIRST = 1,
    FIND_LAST = 2,
    GET_FIRST = 3, // Returns position of first (for recurring usage)
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
  const TCHAR *GetLanguage ();
  void CallLangContextMenu ();
  void GetDefaultHunspellPath (TCHAR *&Path);
  BOOL CheckWord (char *Word, long Start, long End);
  void GetVisibleLimits(long &Start, long &Finish);
  char *GetVisibleText(long *offset, BOOL NotIntersectionOnly = FALSE);
  int CheckText (char *TextToCheck, long offset, CheckTextMode Mode);
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
  char *GetWordAt (long CharPos, char *Text, long Offset);
  void SetDefaultDelimiters ();
  void hideSuggestionBox ();
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
  void LoadFromIni (std::wstring &value, const TCHAR *name, const TCHAR *defaultValue, BOOL inQuotes = 0);
  void LoadFromIni (bool &Value, const TCHAR *Name, bool DefaultValue);
  void LoadFromIniUtf8 (char *&Value, const TCHAR *Name, const char *DefaultValue, BOOL InQuotes = 0);
  void CopyMisspellingsToClipboard();
  int CheckTextDefaultAnswer (CheckTextMode Mode);
  void setNewSettings (const SettingsData &settingsArg);
  void onSettingsChanged (const SettingsData &settingsData);
  SpellerController *activeSpeller ();
  const wchar_t *currentLanguage ();
  const SettingsData &settings () const;

private:

  std::shared_ptr<SpellCheckerStatus> m_status;
  BOOL SettingsLoaded;
  BOOL AutoCheckText;
  BOOL checkTextEnabled;
  BOOL WUCisRight;
  int MultiLangMode;
  char *DelimUtf8Converted; // String where escape characters are properly converted to corresponding symbols
  char *DelimConverted; // Same but in ANSI encoding
  TCHAR *ServerNames[3]; // Only user ones, there'll also be bunch of predetermined ones
  TCHAR *DefaultServers[3];
  int LastUsedAddress; // equals USER_SERVER_CONST + num if user address is used, otherwise equals number of default server
  int AddressIsSet;
  BOOL ShowOnlyKnown;
  BOOL InstallSystem;
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
  bool firstTime = true;

  int Lexer;
  std::vector <SuggestionsMenuItem *> *SuggestionMenuItems;
  std::vector <char *> *LastSuggestions;
  _locale_t utf8_l;
  long ModifiedStart;
  long ModifiedEnd;
  long WUCPosition; // WUC = Word Under Cursor (Position in global doc coordinates),
  long WUCLength;
  long CurrentPosition;
  NppData *NppDataInstance;
  EncodingType currentEncoding;
  TCHAR *IniFilePath;
  char *SelectedWord;
  SettingsDlg *settingsDlgInstance;
  Suggestions *SuggestionsInstance;
  LangListDialog *LangListInstance;
  char *VisibleText;
  int VisibleTextLength;
  long VisibleTextOffset;
  BOOL RemoveUserDics;
  BOOL RemoveSystem;

  enum_vector<SpellerType, std::unique_ptr<SpellerController>> m_spellers;
  std::unique_ptr<Settings> m_settings;

  // 6 - is arbitrary maximum size, actually almost everywhere it's 1
  char *YoANSI;
  char *YeANSI;
  char *yoANSI;
  char *yeANSI;
  char *PunctuationApostropheANSI;

public:
  auto getStatus ()->decltype (m_status) { return m_status; } // TODO-MSVC2015: Move this function up, remove trailing type
  std::shared_ptr<const SettingsData> getSettings ();
};
#endif // SPELLCHECKER_H
