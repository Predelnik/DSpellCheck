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

#include "StaticDialog/StaticDialog.h"
#include "PluginInterface.h"
#include "TabBar/ControlsTab.h"

#include <uxtheme.h>

class HunspellInterface;
class SpellChecker;
struct LanguageName;
typedef HTHEME(WINAPI *OTDProc)(HWND, LPCWSTR);

class SimpleDlg : public StaticDialog {
public:
  SimpleDlg();
  ~SimpleDlg();
  void initSettings(HINSTANCE hInst, HWND Parent, NppData nppData);
  void ApplySettings(SpellChecker *SpellCheckerInstance);
  BOOL AddAvailableLanguages(std::vector<LanguageName> *LangsAvailable,
                             const wchar_t *CurrentLanguage,
                             const wchar_t *MultiLanguages,
                             HunspellInterface *HunspellSpeller);
  void FillSugestionsNum(int SuggestionsNum);
  void FillLibInfo(int Status, wchar_t *AspellPath, wchar_t *HunspellPath,
                   wchar_t *HunspellAdditionalPath);
  void DisableLanguageCombo(BOOL Disable);
  void SetFileTypes(BOOL CheckThose, const wchar_t *FileTypes);
  void SetSuggType(int SuggType);
  void SetCheckComments(BOOL Value);
  int GetSelectedLib();
  void SetLibMode(int LibMode);
  void SetDecodeNames(BOOL Value);
  void SetOneUserDic(BOOL Value);
  void ApplyLibChange(SpellChecker *SpellCheckerInstance);

protected:
  __override virtual INT_PTR WINAPI run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

private:
  /* NppData struct instance */
  NppData NppDataInstance;

  /* handles of controls */
  COLORREF AspellStatusColor = NULL;
  HBRUSH DefaultBrush = NULL;
  HWND HAspellStatus = NULL;
  HWND HLibPath = NULL;
  HWND HSuggestionsNum = NULL;
  HWND HComboLanguage = NULL;
  HWND HCheckNotThose = NULL;
  HWND HCheckOnlyThose = NULL;
  HWND HFileTypes = NULL;
  HWND HCheckComments = NULL;
  HWND HLibLink = NULL;
  HWND HSuggType = NULL;
  HWND HLibrary = NULL;
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

class AdvancedDlg : public StaticDialog {
public:
  void ApplySettings(SpellChecker *SpellCheckerInstance);
  void FillDelimiters(const char *Delimiters);
  void SetDelimetersEdit(const wchar_t* Delimiters);
  void SetConversionOpts(BOOL ConvertYo, BOOL ConvertSingleQuotesArg,
                         BOOL RemoveSingleApostrophe);
  void SetRecheckDelay(int Delay);
  int GetRecheckDelay();
  void SetSuggBoxSettings(LRESULT Size, LRESULT Trans);
  void SetUnderlineSettings(int Color, int Style);
  void SetIgnore(BOOL IgnoreNumbersArg, BOOL IgnoreCStartArg,
                 BOOL IgnoreCHaveArg, BOOL IgnoreCAllArg, BOOL Ignore_Arg,
                 BOOL Ignore_SA_Apostrophe_Arg, BOOL IgnoreOneLetter);
  void SetBufferSize(int Size);

protected:
  __override virtual INT_PTR WINAPI run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

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
  COLORREF UnderlineColorBtn;
};

class SettingsDlg : public StaticDialog {
public:
  UINT DoDialog(void);
  void initSettings(HINSTANCE hInst, HWND Parent, NppData nppData);
  SimpleDlg *GetSimpleDlg();
  AdvancedDlg *GetAdvancedDlg();

protected:
  INT_PTR WINAPI run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
  void destroy() override;
  void ApplySettings();

private:
  NppData NppDataInstance;
  SimpleDlg SimpleDlgInstance;
  AdvancedDlg AdvancedDlgInstance;
  WindowVector WindowVectorInstance;
  ControlsTab ControlsTabInstance;
};
