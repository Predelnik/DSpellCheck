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

#pragma once
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

struct SuggestionsMenuItem {
  wchar_t *Text;
  BYTE Id;
  BOOL Separator;
  SuggestionsMenuItem(const wchar_t *TextArg, BYTE IdArg, BOOL SeparatorArg = FALSE);
  ~SuggestionsMenuItem() { CLEAN_AND_ZERO_ARR(Text); };
};

void InsertSuggMenuItem(HMENU Menu, const wchar_t* Text, BYTE Id, int InsertPos,
                        BOOL Separator = FALSE);

HWND GetScintillaWindow(const NppData *NppDataArg);
LRESULT SendMsgToActiveEditor(HWND ScintillaWindow, UINT Msg,
                              WPARAM wParam = 0, LPARAM lParam = 0);
BOOL SendMsgToBothEditors(const NppData *NppDataArg, UINT Msg,
                          WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToActiveEditor(HWND ScintillaWindow, UINT Msg, WPARAM wParam = 0,
                              LPARAM lParam = 0);

class SpellChecker {
public:
  SpellChecker(const wchar_t *IniFilePathArg,
               SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
               Suggestions *SuggestionsInstanceArg,
               LangList *LangListInstanceArg);
  ~SpellChecker();
  void RecheckVisibleBothViews();
  BOOL WINAPI NotifyEvent(DWORD Event);
  BOOL WINAPI NotifyNetworkEvent(DWORD Event);
  BOOL WINAPI NotifyMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
  void RecheckVisible(BOOL NotIntersectionOnly = FALSE);
  void RecheckModified();
  void ErrorMsgBox(const wchar_t *message);

  BOOL AspellReinitSettings();
  void SetHunspellLanguage(const wchar_t *Str);
  void SetAspellLanguage(const wchar_t *Str);
  void SetDelimiters(const char *Str);
  void SetSuggestionsNum(int Num);
  void SetAspellPath(const wchar_t *Path);
  void SetMultipleLanguages(const wchar_t *MultiString,
                            AbstractSpellerInterface *Speller);
  void SetHunspellPath(const wchar_t *Path);
  void SetHunspellAdditionalPath(const wchar_t *Path);
  void SetConversionOptions(BOOL ConvertYo, BOOL ConvertSingleQuotesArg,
                            BOOL RemoveBoundaryApostrophesArg);
  void SetCheckThose(int CheckThoseArg);
  void SetFileTypes(wchar_t *FileTypesArg);
  void SetCheckComments(BOOL Value);
  void SetHunspellMultipleLanguages(const char *MultiLanguagesArg);
  void SetAspellMultipleLanguages(const char *MultiLanguagesArg);
  void SetUnderlineColor(int Value);
  void SetUnderlineStyle(int Value);
  void SetProxyUserName(wchar_t *Str);
  void SetProxyHostName(wchar_t *Str);
  void SetProxyPassword(wchar_t *Str);
  void SetProxyPort(int Value);
  void SetUseProxy(BOOL Value);
  void SetProxyAnonymous(BOOL Value);
  void SetProxyType(int Value);
  void SetIgnore(BOOL IgnoreNumbersArg, BOOL IgnoreCStartArg,
                 BOOL IgnoreCHaveArg, BOOL IgnoreCAllArg, BOOL Ignore_Arg,
                 BOOL IgnoreSEApostropheArg, BOOL IgnoreOneLetterArg);
  void SetSuggBoxSettings(int Size, int Transparency, int SaveIni = 1);
  void SetBufferSize(int Size);
  void SetSuggType(int SuggType);
  void SetLibMode(int i);
  void SetDecodeNames(BOOL Value);
  void SetOneUserDic(BOOL Value);
  BOOL GetOneUserDic();
  void SetShowOnlyKnow(BOOL Value);
  void SetInstallSystem(BOOL Value);
  void FillDialogs(BOOL NoDisplayCall = FALSE);
  void ReinitLanguageLists(BOOL UpdateDialogs);
  wchar_t *GetHunspellPath() { return HunspellPath; }
  void switchWriteDebugLog();
  bool getWriteDebugLog () const { return writeDebugLog; }
  wchar_t *GetHunspellAdditionalPath() { return AdditionalHunspellPath; }
  std::wstring getDebugLogPath() const;
  wchar_t *GetLangByIndex(int i);
  BOOL GetShowOnlyKnown();
  BOOL GetInstallSystem();
  BOOL GetDecodeNames();
  void DoPluginMenuInclusion(BOOL Invalidate = FALSE);
  HunspellInterface *GetHunspellSpeller() { return HunspellSpeller; };
  int GetLibMode();
  BOOL HunspellReinitSettings(BOOL ResetDirectory);
  void SetRemoveUserDics(BOOL Value);
  void SetRemoveSystem(BOOL Value);
  BOOL GetRemoveUserDics();
  BOOL GetRemoveSystem();
  wchar_t *GetProxyUserName();
  wchar_t *GetProxyHostName();
  wchar_t *GetProxyPassword();
  int GetProxyPort();
  BOOL GetUseProxy();
  BOOL GetProxyAnonymous();
  int GetProxyType();
  long PreviousA, PreviousB;
  void SetSuggestionsBoxTransparency();
  void addUserServer (std::wstring server);
  BOOL getAutoCheckText () const { return AutoCheckText; }

private:
  enum CheckTextMode {
    UNDERLINE_ERRORS = 0,
    FIND_FIRST = 1,
    FIND_LAST = 2,
    GET_FIRST = 3, // Returns position of first (for recurring usage)
  };

  HWND GetCurrentScintilla();
  void CreateWordUnderline(HWND ScintillaWindow, long start, long end);
  void RemoveUnderline(HWND ScintillaWindow, long start, long end);
  void ClearAllUnderlines();
  void FindNextMistake();
  void FindPrevMistake();
  void ClearVisibleUnderlines();
  void Cleanup();
  void CheckFileName();
  const char *GetDelimiters();
  void GetDefaultHunspellPath(wchar_t *&Path);
  BOOL CheckWord(char *Word, long Start, long End);
  void GetVisibleLimits(long &Start, long &Finish);
  char *GetVisibleText(long *offset, BOOL NotIntersectionOnly = FALSE);
  int CheckText(char *TextToCheck, long offset, CheckTextMode Mode);
  void CheckVisible(BOOL NotIntersectionOnly = FALSE);
  void setEncodingById(int EncId);
  void SaveSettings();
  void LoadSettings();
  void UpdateAutocheckStatus(int SaveSetting = 1);
  void SwitchAutoCheck();
  void FillSuggestionsMenu(HMENU Menu);
  void ProcessMenuResult(WPARAM MenuId);
  void InitSuggestionsBox();
  BOOL GetWordUnderCursorIsRight(long &Pos, long &Length,
                                 BOOL UseTextCursor = FALSE);
  char *GetWordAt(long CharPos, char *Text, long Offset);
  void SetDefaultDelimiters();
  void HideSuggestionBox();
  void GetLimitsAndRecheckModified();
  BOOL CheckTextNeeded();
  int CheckWordInCommentOrString(LRESULT Style);
  LRESULT GetStyle(int Pos);
  void RefreshUnderlineStyle();
  void WriteSetting(LPARAM lParam);
  void ApplyConversions(char *Word);
  void PrepareStringForConversion();
  void ResetDownloadCombobox();
  void PreserveCurrentAddressIndex();
  void FillDownloadDics();
  void ResetHotSpotCache();
  void CheckSpecialDelimeters(char *&Word, const char *TextStart,
                              long &WordStart, long &WordEnd);

  void SaveToIni(const wchar_t *Name, const wchar_t *Value,
                 const wchar_t *DefaultValue, BOOL InQuotes = 0);
  void SaveToIni(const wchar_t *Name, int Value, int DefaultValue);
  void SaveToIniUtf8(const wchar_t *Name, const char *Value,
                     const char *DefaultValue, BOOL InQuotes = 0);

  void LoadFromIni(wchar_t *&Value, const wchar_t *Name,
                   const wchar_t *DefaultValue, BOOL InQuotes = 0);
  void LoadFromIni(int &Value, const wchar_t *Name, int DefaultValue);
  void LoadFromIniUtf8(char *&Value, const wchar_t *Name,
                       const char *DefaultValue, BOOL InQuotes = 0);
  void CopyMisspellingsToClipboard();
  int CheckTextDefaultAnswer(CheckTextMode Mode);

private:
  std::vector<LanguageName> *CurrentLangs;
  BOOL SettingsLoaded;
  BOOL OneUserDic;
  BOOL AutoCheckText;
  BOOL CheckTextEnabled;
  BOOL WUCisRight;
  wchar_t *HunspellLanguage;
  wchar_t *HunspellMultiLanguages;
  wchar_t *AspellLanguage;
  wchar_t *AspellMultiLanguages;
  int LibMode; // 0 - Aspell, 1 - Hunspell
  int MultiLangMode;
  int SuggestionsNum;
  int SuggestionsMode;
  char *DelimUtf8; // String without special characters but maybe with escape
                   // characters (like '\n' and stuff)
  char *DelimUtf8Converted; // String where escape characters are properly
                            // converted to corresponding symbols
  char *DelimConverted;     // Same but in ANSI encoding
  wchar_t *ServerNames[3];  // Only user ones, there'll also be bunch of
                            // predetermined ones
  wchar_t *DefaultServers[3];
  int LastUsedAddress; // equals USER_SERVER_CONST + num if user address is
                       // used, otherwise equals number of default server
  int AddressIsSet;
  wchar_t *FileTypes;
  wchar_t *AspellPath;
  wchar_t *HunspellPath;
  wchar_t *AdditionalHunspellPath;
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
  LRESULT HotSpotCache[256]; // STYLE_MAX = 255
  std::map<wchar_t *, DWORD, bool (*)(wchar_t *, wchar_t *)> *SettingsToSave;
  BOOL UseProxy;
  BOOL ProxyAnonymous;
  int ProxyType;
  wchar_t *ProxyHostName;
  wchar_t *ProxyUserName;
  int ProxyPort;
  wchar_t *ProxyPassword;

  LRESULT Lexer;
  std::vector<SuggestionsMenuItem *> *SuggestionMenuItems;
  std::vector<char *> *LastSuggestions;
  long ModifiedStart;
  long ModifiedEnd;
  long WUCPosition; // WUC = Word Under Cursor (Position in global doc
                    // coordinates),
  std::size_t WUCLength;
  LRESULT CurrentPosition;
  NppData *NppDataInstance;
  EncodingType CurrentEncoding;
  wchar_t *IniFilePath;
  char *SelectedWord;
  SettingsDlg *SettingsDlgInstance;
  Suggestions *SuggestionsInstance;
  LangList *LangListInstance;
  char *VisibleText;
  std::ptrdiff_t VisibleTextLength;
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
  bool writeDebugLog = false;
};
