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

#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H
#include "StaticDialog/StaticDialog.h"
#include "PluginInterface.h"
#include "TabBar/ControlsTab.h"

class SpellChecker;

class SimpleDlg : public StaticDialog
{
public:
  __override void init (HINSTANCE hInst, HWND Parent, NppData nppData);
  void ApplySettings (SpellChecker *SpellCheckerInstance);
  BOOL AddAvalaibleLanguages (const char *CurrentLanguage, const char *MultiLanguages);
  void FillSugestionsNum (int SuggestionsNum);
  void FillAspellInfo (BOOL Status, TCHAR *AspellPath);
  void DisableLanguageCombo (BOOL Disable);
  void SetFileTypes (BOOL CheckThose, const TCHAR *FileTypes);
  void SetCheckComments (BOOL Value);

protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);

private:
  /* NppData struct instance */
  NppData NppDataInstance;

  /* handles of controls */
  COLORREF AspellStatusColor;
  HBRUSH DefaultBrush;
  HWND HAspellStatus;
  HWND HAspellPath;
  HWND HSuggestionsNum;
  HWND HSpinSuggestionsNum;
  HWND HComboLanguage;
  HWND HCheckNotThose;
  HWND HCheckOnlyThose;
  HWND HFileTypes;
  HWND HCheckComments;
  HWND HAspellLink;
};

class AdvancedDlg : public StaticDialog
{
public:
  void ApplySettings (SpellChecker *SpellCheckerInstance);
  void FillDelimiters (const char *Delimiters);
  void SetDelimetersEdit (TCHAR *Delimiters);
  void setIgnoreYo (BOOL Value);
  void SetRecheckDelay (int Delay);
  int GetRecheckDelay ();
  void SetSuggBoxSettings (int Size, int Trans);
  void SetUnderlineSettings (int Color, int Style);
  void SetIgnore (BOOL IgnoreNumbersArg, BOOL IgnoreCStartArg, BOOL IgnoreCHaveArg, BOOL IgnoreCAllArg, BOOL Ignore_Arg);
  void SetBufferSize (int Size);

protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);

private:
  HWND HEditDelimiters;
  HWND HDefaultDelimiters;
  HWND HIgnoreYo;
  HWND HRecheckDelay;
  HWND HUnderlineColor;
  HWND HUnderlineStyle;
  HWND HIgnoreNumbers;
  HWND HIgnoreCStart;
  HWND HIgnoreCHave;
  HWND HIgnoreCAll;
  HWND HIgnore_;
  HWND HSliderSize;
  HWND HSliderTransparency;
  HWND HBufferSize;

  HBRUSH Brush;
  NppData NppDataInstance;
  COLORREF UnderlineColorBtn;
};

class SettingsDlg : public StaticDialog
{
public:
  UINT DoDialog (void);
  __override void init (HINSTANCE hInst, HWND Parent, NppData nppData);
  SimpleDlg *GetSimpleDlg ();
  AdvancedDlg *GetAdvancedDlg ();
protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
  __override virtual void destroy ();
  void ApplySettings ();
private:
  NppData NppDataInstance;
  SimpleDlg SimpleDlgInstance;
  AdvancedDlg AdvancedDlgInstance;
  WindowVector WindowVectorInstance;
  ControlsTab ControlsTabInstance;
};
#endif //SETTINGS_DLG_H
