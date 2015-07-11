/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation`; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H

#include "StaticDialog/StaticDialog.h"
#include "PluginInterface.h"
#include "TabBar/ControlsTab.h"
#include "MainDef.h"

#include <uxtheme.h>


enum class SpellerType;

class HunspellController;
class SpellChecker;
struct LanguageName;
struct SettingsData;
typedef HTHEME (WINAPI * OTDProc) (HWND, LPCWSTR);

class SimpleDlg : public StaticDialog
{
public:
  SimpleDlg ();
  ~SimpleDlg ();
  void init (HINSTANCE hInst, HWND Parent, NppData nppData);
  void applySettings (SettingsData &settings);
  BOOL AddAvailableLanguages (std::vector <LanguageName> *LangsAvailable, const TCHAR *CurrentLanguage, const TCHAR *MultiLanguages, HunspellController *HunspellSpeller);
  void FillSugestionsNum (int SuggestionsNum);
  void updateVisibility (const SettingsData &settings, const std::vector <LanguageName> &languageList);
  void DisableLanguageCombo (BOOL Disable);
  SpellerType selectedSpellerType ();
  void SetLibMode (int LibMode);
  void SetOneUserDic (BOOL Value);
  void onSelectedSpellerChange ();
  void updateOnConfigurationChange (const SettingsData &settings, const std::vector<LanguageName> &langList);

protected:
  virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam) override;
private:
  /* NppData struct instance */
  NppData NppDataInstance;

  /* handles of controls */
  COLORREF AspellStatusColor = NULL;
  HBRUSH DefaultBrush = NULL;
  HWND HAspellStatus = NULL;
  HWND HLibPath = NULL;
  HWND HSuggestionsNum = NULL;
  HWND HSpinSuggestionsNum = NULL;
  HWND HComboLanguage = NULL;
  HWND HCheckNotThose = NULL;
  HWND HCheckOnlyThose = NULL;
  HWND HFileTypes = NULL;
  HWND HCheckComments = NULL;
  HWND HLibLink = NULL;
  HWND HSuggType = NULL;
  HWND HSpellerTypeCombo = NULL;
  HWND HLibGroupBox = NULL;
  HWND HDownloadDics = NULL;
  HWND HRemoveDics = NULL;
  HWND HDecodeNames = NULL;
  HWND HOneUserDic = NULL;
  HWND HHunspellPathGroupBox = NULL;
  HWND HHunspellPathType = NULL;
  HWND HAspellResetPath = NULL;
  HWND HHunspellResetPath = NULL;
  HWND HSystemPath = NULL;

  HMODULE _hUxTheme;
  OTDProc _OpenThemeData;
};

class AdvancedDlg : public StaticDialog
{
public:
  void applySettings (SettingsData &settings);
  void SetDelimetersEdit (TCHAR *Delimiters);
  void SetConversionOpts (BOOL ConvertYo, BOOL ConvertSingleQuotesArg, BOOL RemoveSingleApostrophe);
  void SetRecheckDelay (int Delay);
  int GetRecheckDelay ();
  void updateOnConfigurationChange (const SettingsData &settings);

protected:
  virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam) override;
private:
  HWND HEditDelimiters = NULL;
  HWND HDefaultDelimiters = NULL;
  HWND HIgnoreYo = NULL;
  HWND HConvertSingleQuotes = NULL;
  HWND HRecheckDelay = NULL;
  HWND HUnderlineColor = NULL;
  HWND HUnderlineStyle = NULL;
  HWND HIgnoreNumbers = NULL;
  HWND HIgnoreCStart = NULL;
  HWND HIgnoreCHave = NULL;
  HWND HIgnoreCAll = NULL;
  HWND HIgnoreOneLetter = NULL;
  HWND HIgnore_ = NULL;
  HWND HIgnoreSEApostrophe = NULL;
  HWND HSliderSize = NULL;
  HWND HSliderTransparency = NULL;
  HWND HBufferSize = NULL;
  HWND HRemoveBoundaryApostrophes = NULL;

  HBRUSH Brush;
  NppData NppDataInstance;
  COLORREF underlineColorBtn;
};

class SettingsDlg : public StaticDialog
{
public:
  SimpleDlg *GetSimpleDlg ();
  AdvancedDlg *GetAdvancedDlg ();
  UINT DoDialog (void);
  void init (HINSTANCE hInst, HWND Parent, NppData nppData);
  void updateOnConfigurationChange ();
protected:
  virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam) override;
  virtual void destroy () override;
  void applySettings ();
private:
  NppData NppDataInstance;
  SimpleDlg simpleDlgInstance;
  AdvancedDlg advancedDlgInstance;
  WindowVector WindowVectorInstance;
  ControlsTab ControlsTabInstance;
};
#endif //SETTINGS_DLG_H
