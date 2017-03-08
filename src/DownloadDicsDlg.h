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

#include "staticdialog\staticdialog.h"
#include <Wininet.h>

struct LanguageName;

void FtpTrim (wchar_t *FtpAddress);

enum FTP_OPERATION_TYPE
{
  FILL_FILE_LIST = 0,
  DOWNLOAD_FILE,
};
class SpellChecker;

class DownloadDicsDlg : public StaticDialog
{
public:
  ~DownloadDicsDlg ();
  DownloadDicsDlg ();
  void DoDialog ();
  // Maybe hunspell interface should be passed here
  void init (HINSTANCE hInst, HWND Parent, SpellChecker *SpellCheckerInstanceArg);
  INT_PTR run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) override;
  void UpdateListBox ();
  void DoFtpOperation (FTP_OPERATION_TYPE Type, wchar_t *Address, wchar_t *FileName = nullptr, wchar_t *Location = nullptr);
  void DownloadSelected ();
  void FillFileList ();
  void RemoveTimer ();
  void OnDisplayAction ();
  void IndicateThatSavingMightBeNeeded ();
  void SetOptions (BOOL ShowOnlyKnown, BOOL InstallSystem);
  void UpdateOptions (SpellChecker * spellchecker);
  void SetCancelPressed (BOOL Value);
  void Refresh ();
  LRESULT AskReplacementMessage(wchar_t* DicName);
private:
  void DoFtpOperationThroughHttpProxy (FTP_OPERATION_TYPE Type, wchar_t *Address, wchar_t *FileName, wchar_t *Location);
private:
  std::vector<LanguageName> *CurrentLangs;
  std::vector<LanguageName> *CurrentLangsFiltered;
  HBRUSH DefaultBrush;
  COLORREF StatusColor;
  SpellChecker *SpellCheckerInstance;
  HWND LibCombo;
  HWND HFileList;
  HWND HAddress;
  HWND HStatus;
  HWND HInstallSelected;
  HWND HShowOnlyKnown;
  HWND HRefresh;
  HWND HInstallSystem;
  HICON RefreshIcon;
  HANDLE Timer;
  BOOL CancelPressed;
  int CheckIfSavingIsNeeded;
};

