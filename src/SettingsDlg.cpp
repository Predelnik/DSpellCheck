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

#include "SettingsDlg.h"

#include "aspell.h"
#include "Controls/CheckedList/CheckedList.h"
#include "CommonFunctions.h"
#include "LangList.h"
#include "DownloadDicsDlg.h"
#include "Plugin.h"
#include "RemoveDics.h"
#include "SpellChecker.h"

#include "resource.h"
#include "LanguageName.h"
#include "HunspellInterface.h"

#include <uxtheme.h>
#include <Vsstyle.h>

SimpleDlg::SimpleDlg () : StaticDialog ()
{
}

void SimpleDlg::init (HINSTANCE hInst, HWND Parent, NppData nppData)
{
  NppDataInstance = nppData;
  return Window::init (hInst, Parent);
}

void SimpleDlg::DisableLanguageCombo (BOOL Disable)
{
  ComboBox_ResetContent (HComboLanguage);
  EnableWindow (HComboLanguage, !Disable);
  ListBox_ResetContent (GetRemoveDics ()->GetListBox ());
  EnableWindow (HRemoveDics, !Disable);
}

// Called from main thread, beware!
BOOL SimpleDlg::AddAvailableLanguages (std::vector <LanguageName> *LangsAvailable, const TCHAR *CurrentLanguage, const TCHAR *MultiLanguages, HunspellInterface *HunspellSpeller)
{
  ComboBox_ResetContent (HComboLanguage);
  ListBox_ResetContent (GetLangList ()->GetListBox ());
  ListBox_ResetContent (GetRemoveDics ()->GetListBox ());

  int SelectedIndex = 0;
  unsigned int i = 0;
  TCHAR *ConvertedBuf = 0;

  for (i = 0; i < LangsAvailable->size (); i++)
  {
    if (_tcscmp (CurrentLanguage, LangsAvailable->at (i).OrigName) == 0)
      SelectedIndex = i;

    ComboBox_AddString (HComboLanguage, LangsAvailable->at (i).AliasName);
    ListBox_AddString (GetLangList ()->GetListBox (), LangsAvailable->at (i).AliasName);
    if (HunspellSpeller)
    {
      TCHAR Buf[DEFAULT_BUF_SIZE];
      _tcscpy (Buf, LangsAvailable->at (i).AliasName);
      if (HunspellSpeller->GetLangOnlySystem (LangsAvailable->at (i).OrigName))
        _tcscat (Buf, _T (" [!For All Users]"));

      ListBox_AddString (GetRemoveDics ()->GetListBox (), Buf);
    }
  }

  if (_tcscmp (CurrentLanguage, _T ("<MULTIPLE>")) == 0)
    SelectedIndex = i;
  if (LangsAvailable->size () != 0)
    ComboBox_AddString (HComboLanguage, _T ("Multiple Languages..."));

  ComboBox_SetCurSel (HComboLanguage, SelectedIndex);

  TCHAR *Context = 0;
  TCHAR *MultiLanguagesCopy = 0;
  TCHAR *Token = 0;
  int Index = 0;
  SetString (MultiLanguagesCopy, MultiLanguages);
  CheckedListBox_EnableCheckAll (GetLangList ()->GetListBox (), BST_UNCHECKED);
  Token = _tcstok_s (MultiLanguagesCopy, _T ("|"), &Context);
  while (Token)
  {
    Index = -1;
    for (unsigned int i = 0; i < LangsAvailable->size (); i++)
    {
      if (_tcscmp (LangsAvailable->at (i).OrigName, Token) == 0)
      {
        Index = i;
        break;
      }
    }
    if (Index != -1)
      CheckedListBox_SetCheckState (GetLangList ()->GetListBox (), Index, BST_CHECKED);
    Token = _tcstok_s (NULL, _T ("|"), &Context);
  }
  CLEAN_AND_ZERO_ARR (MultiLanguagesCopy);
  return TRUE;
}

static HWND CreateToolTip(int toolID, HWND hDlg, PTSTR pszText)
{
  if (!toolID || !hDlg || !pszText)
  {
    return FALSE;
  }
  // Get the window of the tool.
  HWND hwndTool = GetDlgItem(hDlg, toolID);

  // Create the tooltip. g_hInst is the global instance handle.
  HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
    WS_POPUP |TTS_ALWAYSTIP,
    CW_USEDEFAULT, CW_USEDEFAULT,
    CW_USEDEFAULT, CW_USEDEFAULT,
    hDlg, NULL,
    (HINSTANCE) getHModule (), NULL);

  if (!hwndTool || !hwndTip)
  {
    return (HWND)NULL;
  }

  // Associate the tooltip with the tool.
  TOOLINFO toolInfo = { 0 };
  toolInfo.cbSize = sizeof(toolInfo);
  toolInfo.hwnd = hDlg;
  toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  toolInfo.uId = (UINT_PTR)hwndTool;
  toolInfo.lpszText = pszText;
  SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

  return hwndTip;
}

SimpleDlg::~SimpleDlg ()
{
}

void SimpleDlg::ApplyLibChange (SpellChecker *SpellCheckerInstance)
{
  SpellCheckerInstance->SetLibMode (GetSelectedLib ());
  SpellCheckerInstance->ReinitLanguageLists ();
  SpellCheckerInstance->DoPluginMenuInclusion ();
}
// Called from main thread, beware!
void SimpleDlg::ApplySettings (SpellChecker *SpellCheckerInstance)
{
  TCHAR *Buf = 0;
  char *Lang = 0;
  int TextLen = 0;
  int LangCount = ComboBox_GetCount (HComboLanguage);
  int CurSel = ComboBox_GetCurSel (HComboLanguage);

  if (IsWindowEnabled (HComboLanguage))
  {
    if (CurSel == LangCount - 1)
    {
      if (GetSelectedLib ())
        SpellCheckerInstance->SetHunspellLanguage (_T ("<MULTIPLE>"));
      else
        SpellCheckerInstance->SetAspellLanguage (_T ("<MULTIPLE>"));
    }
    else
    {
      if (GetSelectedLib () == 0)
        SpellCheckerInstance->SetAspellLanguage (SpellCheckerInstance->GetLangByIndex (CurSel));
      else
        SpellCheckerInstance->SetHunspellLanguage (SpellCheckerInstance->GetLangByIndex (CurSel));
    }
  }
  SpellCheckerInstance->RecheckVisible ();
  Buf = new TCHAR[DEFAULT_BUF_SIZE];
  Edit_GetText (HSuggestionsNum, Buf, DEFAULT_BUF_SIZE);
  SpellCheckerInstance->SetSuggestionsNum (_ttoi (Buf));
  Edit_GetText (HLibPath, Buf, DEFAULT_BUF_SIZE);
  if (GetSelectedLib () == 0)
    SpellCheckerInstance->SetAspellPath (Buf);
  else
    SpellCheckerInstance->SetHunspellPath (Buf);

  Edit_GetText (HSystemPath, Buf, DEFAULT_BUF_SIZE);
  SpellCheckerInstance->SetHunspellAdditionalPath (Buf);

  SpellCheckerInstance->SetCheckThose (Button_GetCheck (HCheckOnlyThose) == BST_CHECKED ? 1 : 0);
  Edit_GetText (HFileTypes, Buf, DEFAULT_BUF_SIZE);
  SpellCheckerInstance->SetFileTypes (Buf);
  SpellCheckerInstance->SetSuggType (ComboBox_GetCurSel (HSuggType));
  SpellCheckerInstance->SetCheckComments (Button_GetCheck (HCheckComments) == BST_CHECKED);
  SpellCheckerInstance->SetDecodeNames (Button_GetCheck (HDecodeNames) == BST_CHECKED);
  SpellCheckerInstance->SetOneUserDic (Button_GetCheck (HOneUserDic) == BST_CHECKED);
  UpdateLangsMenu ();
  CLEAN_AND_ZERO_ARR (Buf);
}

void SimpleDlg::SetLibMode (int LibMode)
{
  ComboBox_SetCurSel (HLibrary, LibMode);
}

void SimpleDlg::FillLibInfo (int Status, TCHAR *AspellPath, TCHAR *HunspellPath, TCHAR *HunspellAdditionalPath)
{
  if (GetSelectedLib () == 0)
  {
    ShowWindow (HAspellStatus, 1);
    ShowWindow (HDownloadDics, 0);
    if (Status == 2)
    {
      AspellStatusColor = COLOR_OK;
      Static_SetText (HAspellStatus, _T ("Aspell Status: OK"));
    }
    else if (Status == 1)
    {
      AspellStatusColor = COLOR_FAIL;
      Static_SetText (HAspellStatus, _T ("Aspell Status: No Dictionaries"));
    }
    else
    {
      AspellStatusColor = COLOR_FAIL;
      Static_SetText (HAspellStatus, _T ("Aspell Status: Fail"));
    }
    TCHAR *Path = 0;
    GetActualAspellPath (Path, AspellPath);
    Edit_SetText (HLibPath, Path);

    Static_SetText (HLibGroupBox, _T ("Aspell Location"));
    ShowWindow (HLibLink, 1);
    ShowWindow (HRemoveDics, 0);
    ShowWindow (HDecodeNames, 0);
    ShowWindow (HOneUserDic, 0);
    ShowWindow (HAspellResetPath, 1);
    ShowWindow (HHunspellResetPath, 0);
    ShowWindow (HHunspellPathGroupBox, 0);
    ShowWindow (HHunspellPathType, 0);
    ShowWindow (HLibPath, 1);
    ShowWindow (HSystemPath, 0);
    // SetWindowText (HLibLink, _T ("<A HREF=\"http://aspell.net/win32/\">Aspell Library and Dictionaries for Win32</A>"));
    CLEAN_AND_ZERO_ARR (Path);
  }
  else
  {
    ShowWindow (HAspellStatus, 0);
    ShowWindow (HDownloadDics, 1);
    ShowWindow (HLibLink, 0); // Link to dictionaries doesn't seem to be working anyway
    ShowWindow (HRemoveDics, 1);
    ShowWindow (HDecodeNames, 1);
    ShowWindow (HOneUserDic, 1);
    ShowWindow (HAspellResetPath, 0);
    ShowWindow (HHunspellResetPath, 1);
    ShowWindow (HHunspellPathGroupBox, 1);
    ShowWindow (HHunspellPathType, 1);
    if (ComboBox_GetCurSel (HHunspellPathType) == 0)
    {
      ShowWindow (HLibPath, 1);
      ShowWindow (HSystemPath, 0);
    }
    else
    {
      ShowWindow (HLibPath, 0);
      ShowWindow (HSystemPath, 1);
    }
    // SetWindowText (HLibLink, _T ("<A HREF=\"http://wiki.openoffice.org/wiki/Dictionaries\">Hunspell Dictionaries</A>"));
    Static_SetText (HLibGroupBox, _T ("Hunspell Settings"));
    Edit_SetText (HLibPath, HunspellPath);
    Edit_SetText (HSystemPath, HunspellAdditionalPath);
  }
}

void SimpleDlg::FillSugestionsNum (int SuggestionsNum)
{
  TCHAR Buf[10];
  _itot_s (SuggestionsNum, Buf, 10);
  Edit_SetText (HSuggestionsNum, Buf);
}

void SimpleDlg::SetFileTypes (BOOL CheckThose, const TCHAR *FileTypes)
{
  if (!CheckThose)
  {
    Button_SetCheck (HCheckNotThose, BST_CHECKED);
    Button_SetCheck (HCheckOnlyThose, BST_UNCHECKED);
    Edit_SetText (HFileTypes, FileTypes);
  }
  else
  {
    Button_SetCheck (HCheckOnlyThose, BST_CHECKED);
    Button_SetCheck (HCheckNotThose, BST_UNCHECKED);
    Edit_SetText (HFileTypes, FileTypes);
  }
}

void SimpleDlg::SetSuggType (int SuggType)
{
  ComboBox_SetCurSel (HSuggType, SuggType);
}

void SimpleDlg::SetCheckComments (BOOL Value)
{
  Button_SetCheck (HCheckComments, Value ? BST_CHECKED : BST_UNCHECKED);
}

void SimpleDlg::SetDecodeNames (BOOL Value)
{
  Button_SetCheck (HDecodeNames, Value ? BST_CHECKED : BST_UNCHECKED);
}

void SimpleDlg::SetOneUserDic (BOOL Value)
{
  Button_SetCheck (HOneUserDic, Value ? BST_CHECKED : BST_UNCHECKED);
}

int SimpleDlg::GetSelectedLib ()
{
  return ComboBox_GetCurSel (HLibrary);
}

static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  // If the BFFM_INITIALIZED message is received
  // set the path to the start path.
  switch (uMsg)
  {
  case BFFM_INITIALIZED:
    {
      if (NULL != lpData)
      {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
      }
    }
  }

  return 0; // The function should always return 0.
}

BOOL CALLBACK SimpleDlg::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  char *LangString = NULL;
  int length = 0;
  TCHAR Buf[DEFAULT_BUF_SIZE];
  int x;
  TCHAR *EndPtr;

  switch (message)
  {
  case WM_INITDIALOG:
    {
      // Retrieving handles of dialog controls
      HComboLanguage = ::GetDlgItem(_hSelf, IDC_COMBO_LANGUAGE);
      HSuggestionsNum = ::GetDlgItem(_hSelf, IDC_SUGGESTIONS_NUM);
      HAspellStatus = ::GetDlgItem (_hSelf, IDC_ASPELL_STATUS);
      HLibPath = ::GetDlgItem (_hSelf, IDC_ASPELLPATH);
      HCheckNotThose = ::GetDlgItem (_hSelf, IDC_FILETYPES_CHECKNOTTHOSE);
      HCheckOnlyThose = ::GetDlgItem (_hSelf, IDC_FILETYPES_CHECKTHOSE);
      HFileTypes = ::GetDlgItem (_hSelf, IDC_FILETYPES);
      HCheckComments = ::GetDlgItem (_hSelf, IDC_CHECKCOMMENTS);
      HLibLink = ::GetDlgItem (_hSelf, IDC_LIB_LINK);
      HSuggType = ::GetDlgItem (_hSelf, IDC_SUGG_TYPE);
      HLibrary = ::GetDlgItem (_hSelf, IDC_LIBRARY);
      HLibGroupBox = ::GetDlgItem (_hSelf, IDC_LIB_GROUPBOX);
      HDownloadDics = ::GetDlgItem (_hSelf, IDC_DOWNLOADDICS);
      HRemoveDics = ::GetDlgItem (_hSelf, IDC_REMOVE_DICS);
      HDecodeNames = ::GetDlgItem (_hSelf, IDC_DECODE_NAMES);
      HOneUserDic = ::GetDlgItem (_hSelf, IDC_ONE_USER_DIC);
      HHunspellPathGroupBox = ::GetDlgItem (_hSelf, IDC_HUNSPELL_PATH_GROUPBOX);
      HHunspellPathType = ::GetDlgItem (_hSelf, IDC_HUNSPELL_PATH_TYPE);
      HAspellResetPath = ::GetDlgItem (_hSelf, IDC_RESETASPELLPATH);
      HHunspellResetPath = ::GetDlgItem (_hSelf, IDC_RESETHUNSPELLPATH);
      HSystemPath = ::GetDlgItem (_hSelf, IDC_SYSTEMPATH);
      ComboBox_AddString (HLibrary, _T ("Aspell"));
      ComboBox_AddString (HLibrary, _T ("Hunspell"));
      ComboBox_AddString (HHunspellPathType, _T ("For Current User"));
      ComboBox_AddString (HHunspellPathType, _T ("For All Users"));
      ComboBox_SetCurSel (HHunspellPathType, 0);
      ShowWindow (HLibPath, 1);
      ShowWindow (HSystemPath, 0);

      ComboBox_AddString (HSuggType, _T ("Special Suggestion Button"));
      ComboBox_AddString (HSuggType, _T ("Use N++ Context Menu"));
      DefaultBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

      AspellStatusColor = RGB (0, 0, 0);
      return TRUE;
    }
    break;
  case WM_CLOSE:
    {
      EndDialog(_hSelf, 0);
      DeleteObject (DefaultBrush);
      return TRUE;
    }
    break;
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDC_COMBO_LANGUAGE:
        if (HIWORD (wParam) == CBN_SELCHANGE)
        {
          if (ComboBox_GetCurSel (HComboLanguage) == ComboBox_GetCount (HComboLanguage) - 1)
          {
            GetLangList ()->DoDialog ();
          }
        }
        break;
      case IDC_LIBRARY:
        if (HIWORD (wParam) == CBN_SELCHANGE)
        {
          SendEvent (EID_LIB_CHANGE);
        }
        break;
      case IDC_HUNSPELL_PATH_TYPE:
        if (HIWORD (wParam) == CBN_SELCHANGE)
        {
          if (ComboBox_GetCurSel (HHunspellPathType) == 0 || GetSelectedLib () == 0)
          {
            ShowWindow (HLibPath, 1);
            ShowWindow (HSystemPath, 0);
          }
          else
          {
            ShowWindow (HLibPath, 0);
            ShowWindow (HSystemPath, 1);
          }
        }
        break;
      case IDC_SUGGESTIONS_NUM:
        {
          if (HIWORD (wParam) == EN_CHANGE)
          {
            Edit_GetText (HSuggestionsNum, Buf, DEFAULT_BUF_SIZE);
            if (!*Buf)
              return TRUE;

            x = _tcstol (Buf, &EndPtr, 10);
            if (*EndPtr)
              Edit_SetText (HSuggestionsNum, _T ("5"));
            else if (x > 20)
              Edit_SetText (HSuggestionsNum, _T ("20"));
            else if (x < 1)
              Edit_SetText (HSuggestionsNum, _T ("1"));

            return TRUE;
          }
        }
        break;
      case IDC_REMOVE_DICS:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          GetRemoveDics ()->DoDialog ();
        }
        break;
      case IDC_RESETASPELLPATH:
      case IDC_RESETHUNSPELLPATH:
        {
          if (HIWORD (wParam) == BN_CLICKED)
          {
            TCHAR *Path = 0;
            if (GetSelectedLib () == 0)
              GetDefaultAspellPath (Path);
            else
              GetDefaultHunspellPath_ (Path);

            if (GetSelectedLib () == 0 || ComboBox_GetCurSel (HHunspellPathType) == 0)
              Edit_SetText (HLibPath, Path);
            else
              Edit_SetText (HSystemPath, _T (".\\plugins\\config\\Hunspell"));

            CLEAN_AND_ZERO_ARR (Path);
            return TRUE;
          }
        }
        break;
      case IDC_DOWNLOADDICS:
        {
          if (HIWORD (wParam) == BN_CLICKED)
          {
            GetDownloadDics ()->DoDialog ();
          }
        }
        break;
      case IDC_BROWSEASPELLPATH:
        if (GetSelectedLib () == 0)
        {
          OPENFILENAME ofn;
          ZeroMemory(&ofn, sizeof(ofn));
          ofn.lStructSize = sizeof(ofn);
          ofn.hwndOwner = _hSelf;
          TCHAR *Buf = new TCHAR [Edit_GetTextLength (HLibPath) + 1];
          Edit_GetText (HLibPath, Buf, DEFAULT_BUF_SIZE);
          ofn.lpstrFile = Buf;
          // Set lpstrFile[0] to '\0' so that GetOpenFileName does not
          // use the contents of szFile to initialize itself.
          ofn.lpstrFile[0] = '\0';
          ofn.nMaxFile = DEFAULT_BUF_SIZE;
          ofn.lpstrFilter = _T ("Aspell Library (*.dll)\0*.dll\0");
          ofn.nFilterIndex = 1;
          ofn.lpstrFileTitle = NULL;
          ofn.nMaxFileTitle = 0;
          ofn.lpstrInitialDir = NULL;
          ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
          if (GetOpenFileName(&ofn)==TRUE)
            Edit_SetText (HLibPath, ofn.lpstrFile);
        }
        else
        {
          // Thanks to http://vcfaq.mvps.org/sdk/20.htm
          BROWSEINFO bi = { 0 };
          TCHAR path[MAX_PATH];

          LPITEMIDLIST pidlRoot = NULL;
          SHGetFolderLocation (_hSelf, 0, NULL, NULL, &pidlRoot);

          bi.pidlRoot = pidlRoot;
          bi.lpszTitle = _T("Pick a Directory");
          bi.pszDisplayName = path;
          bi.ulFlags = BIF_RETURNONLYFSDIRS;
          bi.lpfn = BrowseCallbackProc;
          TCHAR *Buf = new TCHAR [Edit_GetTextLength (HLibPath) + 1];
          Edit_GetText (HLibPath, Buf, DEFAULT_BUF_SIZE);
          TCHAR NppPath[MAX_PATH];
          TCHAR FinalPath[MAX_PATH];
          SendMsgToNpp (&NppDataInstance, NPPM_GETNPPDIRECTORY, MAX_PATH, (LPARAM) NppPath);
          PathCombine (FinalPath, NppPath, Buf);
          bi.lParam         = (LPARAM) FinalPath;
          LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );
          if ( pidl != 0 )
          {
            // get the name of the folder
            TCHAR *szPath = new TCHAR[MAX_PATH];
            SHGetPathFromIDList (pidl, szPath);
            Edit_SetText (HLibPath, szPath);
            CoTaskMemFree(pidl);
            // free memory used

            CoUninitialize();
          }
        }
        break;
      }
    }
    break;
  case WM_NOTIFY:

    switch (((LPNMHDR)lParam)->code)
    {
    case NM_CLICK:          // Fall through to the next case.
    case NM_RETURN:
      {
        PNMLINK pNMLink = (PNMLINK)lParam;
        LITEM   item    = pNMLink->item;

        if ((((LPNMHDR)lParam)->hwndFrom == HLibLink) && (item.iLink == 0))
        {
          ShellExecute (NULL, _T ("open"), item.szUrl, NULL, NULL, SW_SHOW);
        }

        break;
      }
    }
    break;
  case WM_CTLCOLORSTATIC:
    if (GetDlgItem (_hSelf, IDC_ASPELL_STATUS) == (HWND)lParam)
    {
      HDC hDC = (HDC)wParam;

      HTHEME hTheme = OpenThemeData(_hSelf, _T ("TAB"));
      SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
      SetBkMode(hDC, TRANSPARENT);
      SetTextColor(hDC, AspellStatusColor);
      if (hTheme)
        return 0;
      else
        return (INT_PTR) DefaultBrush;
    }
    break;
  }
  return FALSE;
}

void AdvancedDlg::SetUnderlineSettings (int Color, int Style)
{
  UnderlineColorBtn = Color;
  ComboBox_SetCurSel (HUnderlineStyle, Style);
}

void AdvancedDlg::FillDelimiters (const char *Delimiters)
{
  TCHAR *TBuf = 0;
  SetStringSUtf8 (TBuf, Delimiters);
  Edit_SetText (HEditDelimiters, TBuf);
  CLEAN_AND_ZERO_ARR (TBuf);
}

void AdvancedDlg::setConversionOpts (BOOL ConvertYo, BOOL ConvertSingleQuotesArg)
{
  Button_SetCheck (HIgnoreYo, ConvertYo ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HConvertSingleQuotes, ConvertSingleQuotesArg ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedDlg::SetIgnore (BOOL IgnoreNumbersArg, BOOL IgnoreCStartArg, BOOL IgnoreCHaveArg, BOOL IgnoreCAllArg,
                             BOOL Ignore_Arg, BOOL Ignore_SA_Apostrophe_Arg, BOOL IgnoreOneLetter)
{
  Button_SetCheck (HIgnoreNumbers, IgnoreNumbersArg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreCStart, IgnoreCStartArg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreCHave, IgnoreCHaveArg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreCAll, IgnoreCAllArg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreOneLetter, IgnoreOneLetter ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnore_, Ignore_Arg ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreSEApostrophe, Ignore_SA_Apostrophe_Arg ? BST_CHECKED : BST_UNCHECKED);
}

void AdvancedDlg::SetSuggBoxSettings (int Size, int Trans)
{
  SendMessage (HSliderSize, TBM_SETPOS, TRUE, Size);
  SendMessage (HSliderTransparency, TBM_SETPOS, TRUE, Trans);
}

void AdvancedDlg::SetBufferSize (int Size)
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  _itot_s (Size, Buf, 10);
  Edit_SetText (HBufferSize, Buf);
}

const TCHAR *const IndicNames[] = {_T ("Plain"), _T ("Squiggle"), _T ("TT"), _T ("Diagonal"), _T ("Strike"), _T ("Hidden"),
  _T ("Box"), _T ("Round Box"), _T ("Straight Box"), _T ("Dash"),
  _T ("Dots"), _T ("Squiggle Low")};

BOOL CALLBACK AdvancedDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  TCHAR *EndPtr = 0;
  TCHAR Buf[DEFAULT_BUF_SIZE];
  int x;
  switch (message)
  {
  case WM_INITDIALOG:
    {
      // Retrieving handles of dialog controls
      HEditDelimiters = ::GetDlgItem(_hSelf, IDC_DELIMETERS);
      HDefaultDelimiters = ::GetDlgItem (_hSelf, IDC_DEFAULT_DELIMITERS);
      HIgnoreYo = ::GetDlgItem (_hSelf, IDC_COUNT_YO_AS_YE);
      HConvertSingleQuotes = ::GetDlgItem (_hSelf, IDC_COUNT_SINGLE_QUOTES_AS_APOSTROPHE);
      HRecheckDelay = ::GetDlgItem (_hSelf, IDC_RECHECK_DELAY);
      HUnderlineColor = ::GetDlgItem (_hSelf, IDC_UNDERLINE_COLOR);
      HUnderlineStyle = ::GetDlgItem (_hSelf, IDC_UNDERLINE_STYLE);
      HIgnoreNumbers = ::GetDlgItem (_hSelf, IDC_IGNORE_NUMBERS);
      HIgnoreCStart = ::GetDlgItem (_hSelf, IDC_IGNORE_CSTART);
      HIgnoreCHave = ::GetDlgItem (_hSelf,IDC_IGNORE_CHAVE);
      HIgnoreCAll = ::GetDlgItem (_hSelf, IDC_IGNORE_CALL);
      HIgnoreOneLetter = ::GetDlgItem (_hSelf, IDC_IGNORE_ONE_LETTER);
      HIgnore_ = ::GetDlgItem (_hSelf, IDC_IGNORE_);
      HIgnoreSEApostrophe = ::GetDlgItem (_hSelf, IDC_IGNORE_SE_APOSTROPHE);
      HSliderSize = ::GetDlgItem (_hSelf, IDC_SLIDER_SIZE);
      HSliderTransparency = ::GetDlgItem (_hSelf, IDC_SLIDER_TRANSPARENCY);
      HBufferSize = ::GetDlgItem (_hSelf, IDC_BUFFER_SIZE);
      SendMessage (HSliderSize, TBM_SETRANGE, TRUE, MAKELPARAM (5, 22));
      SendMessage (HSliderTransparency, TBM_SETRANGE, TRUE, MAKELPARAM (5, 100));

      Brush = 0;

      CreateToolTip (IDC_DELIMETERS, _hSelf, _T ("Standard white-space symbols such as New Line ('\\n'), Carriage Return ('\\r'), Tab ('\\t'), Space (' ') are always counted as delimiters"));
      CreateToolTip (IDC_RECHECK_DELAY, _hSelf, _T ("Delay between the end of typing and rechecking the the text after it"));
      CreateToolTip (IDC_IGNORE_CSTART, _hSelf, _T ("Remember that words at the beginning of sentences would also be ignored that way."));
      CreateToolTip (IDC_IGNORE_SE_APOSTROPHE, _hSelf, _T ("Words like this sadly cannot be added to Aspell user dictionary"));

      ComboBox_ResetContent (HUnderlineStyle);
      for (int i = 0; i < countof (IndicNames); i++)
        ComboBox_AddString (HUnderlineStyle, IndicNames[i]);
      return TRUE;
    }
    break;
  case WM_CLOSE:
    {
      if (Brush)
        DeleteObject (Brush);
      EndDialog(_hSelf, 0);
      return TRUE;
    }
    break;
  case WM_DRAWITEM:
    switch (wParam)
    {
    case IDC_UNDERLINE_COLOR:
      HDC Dc;
      PAINTSTRUCT Ps;
      DRAWITEMSTRUCT *Ds = (LPDRAWITEMSTRUCT) lParam;
      Dc = Ds->hDC;
      /*
      Pen = CreatePen (PS_SOLID, 1, RGB (128, 128, 128));
      SelectPen (Dc, Pen);
      */
      if (Ds->itemState & ODS_SELECTED)
      {
        DrawEdge (Dc, &Ds->rcItem, BDR_SUNKENINNER | BDR_SUNKENOUTER, BF_RECT);
      }
      else
      {
        DrawEdge (Dc, &Ds->rcItem,BDR_RAISEDINNER | BDR_RAISEDOUTER, BF_RECT);
      }
      //DeleteObject (Pen);
      EndPaint (HUnderlineColor, &Ps);
      return TRUE;
    }
    break;
  case WM_CTLCOLORBTN:
    if (GetDlgItem (_hSelf, IDC_UNDERLINE_COLOR) == (HWND)lParam)
    {
      HDC hDC = (HDC)wParam;
      if (Brush)
        DeleteObject (Brush);

      SetBkColor (hDC, UnderlineColorBtn);
      SetBkMode(hDC, TRANSPARENT);
      Brush = CreateSolidBrush (UnderlineColorBtn);
      return (INT_PTR) Brush;
    }
    break;
  case WM_COMMAND:
    switch (LOWORD (wParam))
    {
    case IDC_DEFAULT_DELIMITERS:
      if (HIWORD (wParam) == BN_CLICKED)
        SendEvent (EID_DEFAULT_DELIMITERS);
      return TRUE;
    case IDC_RECHECK_DELAY:
      if (HIWORD (wParam) == EN_CHANGE)
      {
        Edit_GetText (HRecheckDelay, Buf, DEFAULT_BUF_SIZE);
        if (!*Buf)
          return TRUE;

        x = _tcstol (Buf, &EndPtr, 10);
        if (*EndPtr)
          Edit_SetText (HRecheckDelay, _T ("0"));
        else if (x > 30000)
          Edit_SetText (HRecheckDelay, _T ("30000"));
        else if (x < 0)
          Edit_SetText (HRecheckDelay, _T ("0"));

        return TRUE;
      }
      break;
    case IDC_BUFFER_SIZE:
      if (HIWORD (wParam) == EN_CHANGE)
      {
        Edit_GetText (HBufferSize, Buf, DEFAULT_BUF_SIZE);
        if (!*Buf)
          return TRUE;

        x = _tcstol (Buf, &EndPtr, 10);
        if (*EndPtr)
          Edit_SetText (HBufferSize, _T ("1024"));
        else if (x > 10 * 1024)
          Edit_SetText (HBufferSize, _T ("10240"));
        else if (x < 1)
          Edit_SetText (HBufferSize, _T ("1"));

        return TRUE;
      }
      break;
    case IDC_UNDERLINE_COLOR:
      if (HIWORD (wParam) == BN_CLICKED)
      {
        CHOOSECOLOR Cc;
        static COLORREF acrCustClr[16];
        memset (&Cc, 0, sizeof (Cc));
        Cc.hwndOwner = _hSelf;
        Cc.lStructSize = sizeof (CHOOSECOLOR);
        Cc.rgbResult = UnderlineColorBtn;
        Cc.lpCustColors = acrCustClr;
        Cc.Flags = CC_FULLOPEN  | CC_RGBINIT;
        if (ChooseColor (&Cc) == TRUE)
        {
          UnderlineColorBtn = Cc.rgbResult;
          RedrawWindow (HUnderlineColor, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE | RDW_ERASE);
        }
      }
      break;
    }
  }
  return FALSE;
}

void AdvancedDlg::SetDelimetersEdit (TCHAR *Delimiters)
{
  Edit_SetText (HEditDelimiters, Delimiters);
}

void AdvancedDlg::SetRecheckDelay (int Delay)
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  _itot (Delay, Buf, 10);
  Edit_SetText (HRecheckDelay, Buf);
}

int AdvancedDlg::GetRecheckDelay ()
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  Edit_GetText (HRecheckDelay, Buf, DEFAULT_BUF_SIZE);
  TCHAR *EndPtr;
  int x = _tcstol (Buf, &EndPtr, 10);
  return x;
}

// Called from main thread, beware!
void AdvancedDlg::ApplySettings (SpellChecker *SpellCheckerInstance)
{
  TCHAR *TBuf = 0;
  int Length = Edit_GetTextLength (HEditDelimiters);
  TBuf = new TCHAR[Length + 1];
  Edit_GetText (HEditDelimiters, TBuf, Length + 1);
  char *BufUtf8 = 0;
  SetStringDUtf8 (BufUtf8, TBuf);
  CLEAN_AND_ZERO_ARR (TBuf);
  SpellCheckerInstance->SetDelimiters (BufUtf8);
  SpellCheckerInstance->SetConversionOptions (Button_GetCheck (HIgnoreYo) == BST_CHECKED ? TRUE : FALSE,
    Button_GetCheck (HConvertSingleQuotes) == BST_CHECKED ? TRUE : FALSE);
  SpellCheckerInstance->SetUnderlineColor (UnderlineColorBtn);
  SpellCheckerInstance->SetUnderlineStyle (ComboBox_GetCurSel (HUnderlineStyle));
  SpellCheckerInstance->SetIgnore (Button_GetCheck (HIgnoreNumbers) == BST_CHECKED,
    Button_GetCheck (HIgnoreCStart) == BST_CHECKED,
    Button_GetCheck (HIgnoreCHave) == BST_CHECKED,
    Button_GetCheck (HIgnoreCAll) == BST_CHECKED,
    Button_GetCheck (HIgnore_) == BST_CHECKED,
    Button_GetCheck (HIgnoreSEApostrophe) == BST_CHECKED,
    Button_GetCheck (HIgnoreOneLetter) == BST_CHECKED
    );
  SpellCheckerInstance->SetSuggBoxSettings (SendMessage (HSliderSize, TBM_GETPOS, 0, 0),
    SendMessage (HSliderTransparency, TBM_GETPOS, 0, 0));
  int Len = Edit_GetTextLength (HBufferSize) + 1;
  TBuf = new TCHAR [Len];
  Edit_GetText (HBufferSize, TBuf, Len);
  TCHAR *EndPtr;
  int x = _tcstol (TBuf, &EndPtr, 10);
  SpellCheckerInstance->SetBufferSize (x);
  GetDownloadDics ()->UpdateListBox ();

  CLEAN_AND_ZERO_ARR (BufUtf8);
  CLEAN_AND_ZERO_ARR (TBuf);
}

SimpleDlg *SettingsDlg::GetSimpleDlg ()
{
  return &SimpleDlgInstance;
}

AdvancedDlg *SettingsDlg::GetAdvancedDlg ()
{
  return &AdvancedDlgInstance;
}

void SettingsDlg::init (HINSTANCE hInst, HWND Parent, NppData nppData)
{
  NppDataInstance = nppData;
  return Window::init (hInst, Parent);
}

void SettingsDlg::destroy()
{
  SimpleDlgInstance.destroy();
  AdvancedDlgInstance.destroy();
};

// Send appropriate event and set some npp thread properties
void SettingsDlg::ApplySettings ()
{
  SendEvent (EID_APPLY_SETTINGS);
  SetRecheckDelay (AdvancedDlgInstance.GetRecheckDelay ());
}

BOOL CALLBACK SettingsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
  switch (Message)
  {
  case WM_INITDIALOG :
    {
      ControlsTabInstance.init (_hInst, _hSelf, false, true, false);
      ControlsTabInstance.setFont(TEXT("Tahoma"), 13);

      SimpleDlgInstance.init (_hInst, _hSelf, NppDataInstance);
      SimpleDlgInstance.create (IDD_SIMPLE, false, false);
      SimpleDlgInstance.display ();
      AdvancedDlgInstance.init (_hInst, _hSelf);
      AdvancedDlgInstance.create (IDD_ADVANCED, false, false);
      AdvancedDlgInstance.SetRecheckDelay (GetRecheckDelay ());

      WindowVectorInstance.push_back(DlgInfo(&SimpleDlgInstance, TEXT("Simple"), TEXT("Simple Options")));
      WindowVectorInstance.push_back(DlgInfo(&AdvancedDlgInstance, TEXT("Advanced"), TEXT("Advanced Options")));
      ControlsTabInstance.createTabs(WindowVectorInstance);
      ControlsTabInstance.display();
      RECT rc;
      getClientRect(rc);
      ControlsTabInstance.reSizeTo(rc);
      rc.bottom -= 30;

      SimpleDlgInstance.reSizeTo(rc);
      AdvancedDlgInstance.reSizeTo(rc);

      // This stuff is copied from npp source to make tabbed window looked totally nice and white
      ETDTProc enableDlgTheme = (ETDTProc)::SendMessage(_hParent, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
      if (enableDlgTheme)
        enableDlgTheme(_hSelf, ETDT_ENABLETAB);

      SendEvent (EID_FILL_DIALOGS);

      return TRUE;
    }
    break;
  case WM_NOTIFY :
    {
      NMHDR *nmhdr = (NMHDR *)lParam;
      if (nmhdr->code == TCN_SELCHANGE)
      {
        if (nmhdr->hwndFrom == ControlsTabInstance.getHSelf())
        {
          ControlsTabInstance.clickedUpdate();
          return TRUE;
        }
      }
      break;
    }
    break;
  case WM_COMMAND :
    {
      switch (LOWORD (wParam))
      {
      case IDAPPLY:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          ApplySettings ();

          return TRUE;
        }
        break;
      case IDOK:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          SendEvent (EID_HIDE_DIALOG);
          ApplySettings ();
          return TRUE;
        }
        break;
      case IDCANCEL:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          SendEvent (EID_HIDE_DIALOG);
          return TRUE;
        }
        break;
      }
    }
  }
  return FALSE;
}

UINT SettingsDlg::DoDialog (void)
{
  if (!isCreated())
  {
    create (IDD_SETTINGS);
    goToCenter ();
  }
  else
  {
    goToCenter ();
    SendEvent (EID_FILL_DIALOGS);
  }

  return TRUE;
  // return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SETTINGS), _hParent, (DLGPROC)dlgProc, (LPARAM)this);
}