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
class SuggestionsButton;
class AbstractSpellerInterface;
class AspellInterface;
class HunspellInterface;
class SelectProxy;

struct SuggestionsMenuItem {
  std::wstring Text;
  BYTE Id;
  bool Separator;
  SuggestionsMenuItem(const wchar_t *TextArg, int IdArg, bool SeparatorArg = false);
  ~SuggestionsMenuItem() { };
};

void InsertSuggMenuItem(HMENU Menu, const wchar_t* Text, BYTE Id, int InsertPos,
                        bool Separator = false);

HWND GetScintillaWindow(const NppData *NppDataArg);
LRESULT SendMsgToActiveEditor(HWND ScintillaWindow, UINT Msg,
                              WPARAM wParam = 0, LPARAM lParam = 0);
bool SendMsgToBothEditors(const NppData *NppDataArg, UINT Msg,
                          WPARAM wParam = 0, LPARAM lParam = 0);
LRESULT PostMsgToActiveEditor(HWND ScintillaWindow, UINT Msg, WPARAM wParam = 0,
                              LPARAM lParam = 0);

class SpellChecker {
  enum CheckTextMode {
    UNDERLINE_ERRORS = 0,
    FIND_FIRST = 1,
    FIND_LAST = 2,
    GET_FIRST = 3, // Returns position of first (for recurring usage)
  };

public:
  SpellChecker(const wchar_t *IniFilePathArg,
               SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
               SuggestionsButton *SuggestionsInstanceArg,
               LangList *LangListInstanceArg);
  ~SpellChecker();
  void RecheckVisibleBothViews();
  void applySettings();
  void applyMultiLangSettings();
  void applyProxySettings();
  void showSuggestionMenu();
  void precalculateMenu();
  void RecheckVisible(bool NotIntersectionOnly = false);
  void RecheckModified();
  void ErrorMsgBox(const wchar_t *message);

  bool AspellReinitSettings();
  void SetHunspellLanguage(const wchar_t *Str);
  void SetAspellLanguage(const wchar_t *Str);
  void SetDelimiters(const char *Str);
  void SetSuggestionsNum(int Num);
  void SetAspellPath(const wchar_t *Path);
  void SetMultipleLanguages(const wchar_t *MultiString,
                            AbstractSpellerInterface *Speller);
  void SetHunspellPath(const wchar_t *Path);
  void SetHunspellAdditionalPath(const wchar_t *Path);
  void SetConversionOptions(bool ConvertYo, bool ConvertSingleQuotesArg,
                            bool RemoveBoundaryApostrophesArg);
  void SetCheckThose(int CheckThoseArg);
  void SetFileTypes(const wchar_t* FileTypesArg);
  void SetCheckComments(bool Value);
  void SetHunspellMultipleLanguages(const char *MultiLanguagesArg);
  void SetAspellMultipleLanguages(const char *MultiLanguagesArg);
  void SetUnderlineColor(int Value);
  void SetUnderlineStyle(int Value);
  void SetProxyUserName(const wchar_t* Str);
  void SetProxyHostName(const wchar_t* Str);
  void SetProxyPassword(const wchar_t* Str);
  void SetProxyPort(int Value);
  void SetUseProxy(bool Value);
  void SetProxyAnonymous(bool Value);
  void SetProxyType(int Value);
  void SetIgnore(bool IgnoreNumbersArg, bool IgnoreCStartArg,
                 bool IgnoreCHaveArg, bool IgnoreCAllArg, bool Ignore_Arg,
                 bool IgnoreSEApostropheArg, bool IgnoreOneLetterArg);
    std::wstring GetDefaultHunspellPath();
  void SetSuggBoxSettings(int Size, int Transparency, int SaveIni = 1);
  void SetBufferSize(int Size);
  void SetSuggType(int SuggType);
  void SetLibMode(int i);
  void SetDecodeNames(bool Value);
  void SetOneUserDic(bool Value);
  bool GetOneUserDic();
  void SetShowOnlyKnow(bool Value);
  void SetInstallSystem(bool Value);
  void FillDialogs(bool NoDisplayCall = false);
  void ReinitLanguageLists(bool UpdateDialogs);
  const wchar_t *GetHunspellPath() const { return HunspellPath.c_str (); };
  const wchar_t* GetHunspellAdditionalPath() const { return AdditionalHunspellPath.c_str (); };
  wchar_t *GetLangByIndex(int i);
  bool GetShowOnlyKnown();
  bool GetInstallSystem();
  bool GetDecodeNames();
  void DoPluginMenuInclusion(bool Invalidate = false);
  HunspellInterface *GetHunspellSpeller() { return HunspellSpeller.get (); };
  int GetLibMode();
  bool HunspellReinitSettings(bool ResetDirectory);
  void SetRemoveUserDics(bool Value);
  void SetRemoveSystem(bool Value);
  bool GetRemoveUserDics();
  bool GetRemoveSystem();
  const wchar_t *GetProxyUserName() const;
  const wchar_t *GetProxyHostName() const;
  const wchar_t *GetProxyPassword() const;
  int GetProxyPort();
  bool GetUseProxy();
  bool GetProxyAnonymous();
  int GetProxyType();
  long PreviousA, PreviousB;
  void SetSuggestionsBoxTransparency();
  void addUserServer (std::wstring server);
  bool getAutoCheckText () const { return AutoCheckText; }
  void ProcessMenuResult(WPARAM MenuId);
  void copyMisspellingsToClipboard();
  void LoadSettings();
  void SwitchAutoCheck();
  void InitSuggestionsBox();
  void HideSuggestionBox();
  void SetDefaultDelimiters();
  void FindNextMistake();
  void FindPrevMistake();
  void CheckFileName();
  void fillDownloadDicsDialog();
  void updateSelectProxy();
  void updateFromRemoveDicsOptions();
  void updateRemoveDicsOptions();
  void updateFromDownloadDicsOptions();
  void updateFromDownloadDicsOptionsNoUpdate();
  void libChange();
  void langChange();
  void ResetDownloadCombobox();
  void setRecheckDelay (int value);
  int getRecheckDelay ();

private:
  void GetLimitsAndRecheckModified();
  HWND GetCurrentScintilla();
  void CreateWordUnderline(HWND ScintillaWindow, long start, long end);
  void RemoveUnderline(HWND ScintillaWindow, long start, long end);
  void ClearAllUnderlines();
  void ClearVisibleUnderlines();
  void Cleanup();
  const char *GetDelimiters();
  bool CheckWord(std::string Word, long Start, long End);
  void GetVisibleLimits(long &Start, long &Finish);
    std::vector<char> GetVisibleText(long* offset, bool NotIntersectionOnly = false);
  int CheckText(char *TextToCheck, long offset, CheckTextMode Mode);
  void CheckVisible(bool NotIntersectionOnly = false);
  void setEncodingById(int EncId);
  void SaveSettings();
  void UpdateAutocheckStatus(int SaveSetting = 1);
    std::vector<SuggestionsMenuItem> FillSuggestionsMenu(HMENU Menu);
  bool GetWordUnderCursorIsRight(long &Pos, long &Length,
                                 bool UseTextCursor = false);
  char *GetWordAt(long CharPos, char *Text, long Offset);
  bool CheckTextNeeded();
  int CheckWordInCommentOrString(LRESULT Style) const;
  LRESULT GetStyle(int Pos);
  void RefreshUnderlineStyle();
  void ApplyConversions(std::string& Word);
  void PrepareStringForConversion();
  void PreserveCurrentAddressIndex();
  void FillDownloadDics();
  void ResetHotSpotCache();
  void CheckSpecialDelimeters(char *&Word, const char *TextStart,
                              long &WordStart, long &WordEnd);

  void SaveToIni(const wchar_t *Name, const wchar_t *Value,
                 const wchar_t *DefaultValue, bool InQuotes = 0);
  void SaveToIni(const wchar_t *Name, int Value, int DefaultValue);
  void SaveToIniUtf8(const wchar_t *Name, const char *Value,
                     const char *DefaultValue, bool InQuotes = 0);

    std::wstring LoadFromIni(const wchar_t *Name,
                             const wchar_t *defaultValue, bool InQuotes = false);
  void LoadFromIni(bool &Value, const wchar_t *Name, bool DefaultValue);
  void LoadFromIni(int &Value, const wchar_t *Name, int defaultValue);
    std::string LoadFromIniUtf8(const wchar_t *Name,
                                const char *defaultValue, bool InQuotes = 0);
  int CheckTextDefaultAnswer(CheckTextMode Mode);

private:
  std::vector<LanguageName> CurrentLangs;
  bool SettingsLoaded;
  bool OneUserDic;
  bool AutoCheckText;
  bool CheckTextEnabled;
  bool WUCisRight;
  std::wstring HunspellLanguage;
  std::wstring HunspellMultiLanguages;
  std::wstring AspellLanguage;
  std::wstring AspellMultiLanguages;
  int LibMode; // 0 - Aspell, 1 - Hunspell
  int MultiLangMode;
  int SuggestionsNum;
  int SuggestionsMode;
  char *DelimUtf8; // String without special characters but maybe with escape
                   // characters (like '\n' and stuff)
  char *DelimUtf8Converted; // String where escape characters are properly
                            // converted to corresponding symbols
  char *DelimConverted;     // Same but in ANSI encoding
  std::wstring ServerNames[3];  // Only user ones, there'll also be bunch of
                            // predetermined ones
  std::wstring DefaultServers[3];
  int LastUsedAddress; // equals USER_SERVER_CONST + num if user address is
                       // used, otherwise equals number of default server
  int AddressIsSet;
  std::wstring FileTypes;
  std::wstring AspellPath;
  std::wstring HunspellPath;
  std::wstring AdditionalHunspellPath;
  bool IgnoreYo;
  bool ConvertSingleQuotes;
  bool RemoveBoundaryApostrophes;
  bool CheckThose;
  bool CheckComments;
  int UnderlineColor;
  int UnderlineStyle;
  bool IgnoreNumbers;
  bool IgnoreCStart;
  bool IgnoreCHave;
  bool IgnoreCAll;
  bool Ignore_;
  bool IgnoreSEApostrophe;
  bool IgnoreOneLetter;
  bool DecodeNames;
  bool ShowOnlyKnown;
  bool InstallSystem;
  int SBSize;
  int SBTrans;
  int BufferSize;
  const AspellWordList *CurWordList;
  HWND CurrentScintilla;
  LRESULT HotSpotCache[256]; // STYLE_MAX = 255
  bool UseProxy;
  bool ProxyAnonymous;
  int ProxyType;
  std::wstring ProxyHostName;
  std::wstring ProxyUserName;
  int ProxyPort;
  std::wstring ProxyPassword;
  int m_recheckDelay;

  LRESULT Lexer;
  std::vector<std::string> LastSuggestions;
  long ModifiedStart;
  long ModifiedEnd;
  long WUCPosition; // WUC = Word Under Cursor (Position in global doc
                    // coordinates),
  std::size_t WUCLength;
  LRESULT CurrentPosition;
  NppData *NppDataInstance;
  EncodingType CurrentEncoding;
  wchar_t *IniFilePath;
  std::string SelectedWord;
  SettingsDlg *SettingsDlgInstance;
  SuggestionsButton *SuggestionsInstance;
  LangList *LangListInstance;
  std::vector<char> VisibleText;
  long VisibleTextOffset;
  bool RemoveUserDics;
  bool RemoveSystem;

  AbstractSpellerInterface *CurrentSpeller;
  std::unique_ptr<AspellInterface> AspellSpeller;
  std::unique_ptr<HunspellInterface> HunspellSpeller;

  // 6 - is arbitrary maximum size, actually almost everywhere it's 1
  std::string YoANSI;
  std::string YeANSI;
  std::string yoANSI;
  std::string yeANSI;
  std::string PunctuationApostropheANSI;
};
