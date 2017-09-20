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
  bool AddAvailableLanguages(std::vector<LanguageName> *LangsAvailable,
                             const wchar_t *CurrentLanguage,
                             const wchar_t *MultiLanguages,
                             HunspellInterface *HunspellSpeller);
  void FillSugestionsNum(int SuggestionsNum);
  void FillLibInfo(int Status, wchar_t *AspellPath, wchar_t *HunspellPath,
                   wchar_t *HunspellAdditionalPath);
  void DisableLanguageCombo(bool Disable);
  void SetFileTypes(bool CheckThose, const wchar_t *FileTypes);
  void SetSuggType(int SuggType);
  void SetCheckComments(bool Value);
  int GetSelectedLib();
  void SetLibMode(int LibMode);
  void SetDecodeNames(bool Value);
  void SetOneUserDic(bool Value);
  void ApplyLibChange(SpellChecker *SpellCheckerInstance);

protected:
    INT_PTR WINAPI run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
  /* NppData struct instance */
  NppData NppDataInstance;

  /* handles of controls */
  COLORREF AspellStatusColor = NULL;
  HBRUSH DefaultBrush = nullptr;
  HWND HAspellStatus = nullptr;
  HWND HLibPath = nullptr;
  HWND HSuggestionsNum = nullptr;
  HWND HComboLanguage = nullptr;
  HWND HCheckNotThose = nullptr;
  HWND HCheckOnlyThose = nullptr;
  HWND HFileTypes = nullptr;
  HWND HCheckComments = nullptr;
  HWND HLibLink = nullptr;
  HWND HSuggType = nullptr;
  HWND HLibrary = nullptr;
  HWND HLibGroupBox = nullptr;
  HWND HDownloadDics = nullptr;
  HWND HRemoveDics = nullptr;
  HWND HDecodeNames = nullptr;
  HWND HOneUserDic = nullptr;
  HWND HHunspellPathGroupBox = nullptr;
  HWND HHunspellPathType = nullptr;
  HWND HAspellResetPath = nullptr;
  HWND HHunspellResetPath = nullptr;
  HWND HSystemPath = nullptr;

  HMODULE _hUxTheme;
  OTDProc _OpenThemeData;
};

class AdvancedDlg : public StaticDialog {
public:
  void ApplySettings(SpellChecker *SpellCheckerInstance);
  void FillDelimiters(const char *Delimiters);
  void SetDelimetersEdit(const wchar_t* Delimiters);
  void SetConversionOpts(bool ConvertYo, bool ConvertSingleQuotesArg,
                         bool RemoveSingleApostrophe);
  void SetRecheckDelay(int Delay);
  int GetRecheckDelay();
  void SetSuggBoxSettings(LRESULT Size, LRESULT Trans);
  void SetUnderlineSettings(int Color, int Style);
  void SetIgnore(bool IgnoreNumbersArg, bool IgnoreCStartArg,
                 bool IgnoreCHaveArg, bool IgnoreCAllArg, bool Ignore_Arg,
                 bool Ignore_SA_Apostrophe_Arg, bool IgnoreOneLetter);
  void SetBufferSize(int Size);

protected:
    INT_PTR WINAPI run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
  HWND HEditDelimiters = nullptr;
  HWND HDefaultDelimiters = nullptr;
  HWND HIgnoreYo = nullptr;
  HWND HConvertSingleQuotes = nullptr;
  HWND HRecheckDelay = nullptr;
  HWND HUnderlineColor = nullptr;
  HWND HUnderlineStyle = nullptr;
  HWND HIgnoreNumbers = nullptr;
  HWND HIgnoreCStart = nullptr;
  HWND HIgnoreCHave = nullptr;
  HWND HIgnoreCAll = nullptr;
  HWND HIgnoreOneLetter = nullptr;
  HWND HIgnore_ = nullptr;
  HWND HIgnoreSEApostrophe = nullptr;
  HWND HSliderSize = nullptr;
  HWND HSliderTransparency = nullptr;
  HWND HBufferSize = nullptr;
  HWND HRemoveBoundaryApostrophes = nullptr;

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
