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
#include "LangListDialog.h"
#include "DownloadDicsDlg.h"
#include "Plugin.h"
#include "RemoveDicsDialog.h"
#include "SpellChecker.h"

#include "resource.h"
#include "LanguageName.h"
#include "HunspellController.h"
#include "Settings.h"

SimpleDlg::SimpleDlg() : StaticDialog() {
  _hUxTheme = 0;
  _hUxTheme = ::LoadLibrary(TEXT("uxtheme.dll"));
  if (_hUxTheme)
    _OpenThemeData = (OTDProc)::GetProcAddress(_hUxTheme, "OpenThemeData");
}

void SimpleDlg::init(HINSTANCE hInst, HWND Parent, NppData nppData) {
  NppDataInstance = nppData;
  return Window::init(hInst, Parent);
}

static HWND CreateToolTip(int toolID, HWND hDlg, PTSTR pszText) {
  if (!toolID || !hDlg || !pszText) {
    return FALSE;
  }
  // Get the window of the tool.
  HWND hwndTool = GetDlgItem(hDlg, toolID);

  // Create the tooltip. g_hInst is the global instance handle.
  HWND hwndTip =
      CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP,
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                     hDlg, NULL, (HINSTANCE)getHModule(), NULL);

  if (!hwndTool || !hwndTip) {
    return (HWND)NULL;
  }

  // Associate the tooltip with the tool.
  TOOLINFO toolInfo = {0};
  toolInfo.cbSize = sizeof(toolInfo);
  toolInfo.hwnd = hDlg;
  toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  toolInfo.uId = (UINT_PTR)hwndTool;
  toolInfo.lpszText = pszText;
  SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

  return hwndTip;
}

SimpleDlg::~SimpleDlg() {
  if (_hUxTheme) FreeLibrary(_hUxTheme);
}

// Called from main thread, beware!
void SimpleDlg::applySettings(SettingsData &settings)
{
  TCHAR *buf = 0;
  auto *sc = getSpellChecker();
  int LangCount = ComboBox_GetCount(HComboLanguage);
  int curSel = ComboBox_GetCurSel(HComboLanguage);

  auto type = selectedSpellerType ();

  auto status = sc->getStatus();

  if (IsWindowEnabled(HComboLanguage) && curSel >= 0) {
    if (curSel == LangCount - 1) {
      settings.activeSpellerSettings().activeLanguage = multipleLanguagesStr;
    } else {
      settings.activeSpellerSettings().activeLanguage = status->languageList[curSel].OrigName;
    }
  }
  sc->RecheckVisible();
  buf = new TCHAR[DEFAULT_BUF_SIZE];
  {
    settings.suggestionCount = _ttoi (getWindowText(HSuggestionsNum).c_str ());
  }

  settings.spellerSettings[type].path = getWindowText (HLibPath);
  settings.spellerSettings[type].additionalPath = getWindowText (HSystemPath);

  if (Button_GetCheck (HCheckOnlyThose) == BST_CHECKED)
    settings.fileMaskOption = FileMaskOption::inclusion;
  else if (Button_GetCheck (HCheckNotThose) == BST_CHECKED)
    settings.fileMaskOption = FileMaskOption::exclusion;

  settings.fileMask = getWindowText (HFileTypes);
  settings.useLanguageNameAliases = (Button_GetCheck (HDecodeNames) == BST_CHECKED);
  settings.skipCode = (Button_GetCheck (HCheckComments) == BST_CHECKED);
  settings.unifiedUserDictionary = (Button_GetCheck (HOneUserDic) == BST_CHECKED);
  settings.suggestionControlType = static_cast<SuggestionControlType> (ComboBox_GetCurSel(HSuggType));

  UpdateLangsMenu();
  CLEAN_AND_ZERO_ARR(buf);
}

void SimpleDlg::SetLibMode(int LibMode) {
  ComboBox_SetCurSel(HSpellerTypeCombo, LibMode);
}

void SimpleDlg::updateVisibility(const SettingsData &settings, const SpellCheckerStatus &status) {
  switch (settings.activeSpellerType) {
    case SpellerType::aspell: {
      ShowWindow(HAspellStatus, 1);
      ShowWindow(HDownloadDics, 0);
      switch (status.aspellStatus)
      {
      case AspellStatus::ok:
        if (status.languageList.size () > 0) {
          AspellStatusColor = COLOR_OK;
          Static_SetText (HAspellStatus, _T ("Aspell Status: OK"));
        }
        else {
          AspellStatusColor = COLOR_FAIL;
          Static_SetText (HAspellStatus, _T ("Aspell Status: No Dictionaries"));
        }
        break;
      case AspellStatus::fail:
        AspellStatusColor = COLOR_FAIL;
        Static_SetText (HAspellStatus, _T ("Aspell Status: Fail"));
        break;
      }

      TCHAR *Path = 0;
      GetActualAspellPath(Path, settings.activeSpellerSettings().path.c_str ());
      Edit_SetText(HLibPath, Path);

      Static_SetText(HLibGroupBox, _T ("Aspell Location"));
      ShowWindow(HLibLink, 1);
      ShowWindow(HRemoveDics, 0);
      ShowWindow(HDecodeNames, 0);
      ShowWindow(HOneUserDic, 0);
      ShowWindow(HAspellResetPath, 1);
      ShowWindow(HHunspellResetPath, 0);
      ShowWindow(HHunspellPathGroupBox, 0);
      ShowWindow(HHunspellPathType, 0);
      ShowWindow(HLibPath, 1);
      ShowWindow(HSystemPath, 0);
      // SetWindowText (HLibLink, _T ("<A
      // HREF=\"http://aspell.net/win32/\">Aspell Library and Dictionaries for
      // Win32</A>"));
      CLEAN_AND_ZERO_ARR(Path);
    } break;
    case SpellerType::hunspell: {
      ShowWindow(HAspellStatus, 0);
      ShowWindow(HDownloadDics, 1);
      ShowWindow(HLibLink,
                 0);  // Link to dictionaries doesn't seem to be working anyway
      ShowWindow(HRemoveDics, 1);
      ShowWindow(HDecodeNames, 1);
      ShowWindow(HOneUserDic, 1);
      ShowWindow(HAspellResetPath, 0);
      ShowWindow(HHunspellResetPath, 1);
      ShowWindow(HHunspellPathGroupBox, 1);
      ShowWindow(HHunspellPathType, 1);
      if (ComboBox_GetCurSel(HHunspellPathType) == 0) {
        ShowWindow(HLibPath, 1);
        ShowWindow(HSystemPath, 0);
      } else {
        ShowWindow(HLibPath, 0);
        ShowWindow(HSystemPath, 1);
      }
      // SetWindowText (HLibLink, _T ("<A
      // HREF=\"http://wiki.openoffice.org/wiki/Dictionaries\">Hunspell
      // Dictionaries</A>"));
      Static_SetText(HLibGroupBox, _T ("Hunspell Settings"));
      Edit_SetText(HLibPath, settings.activeSpellerSettings().path.c_str ());
      Edit_SetText(HSystemPath, settings.activeSpellerSettings().additionalPath.c_str ());
    } break;
  }
}

void SimpleDlg::FillSugestionsNum(int SuggestionsNum) {
  TCHAR Buf[10];
  _itot_s(SuggestionsNum, Buf, 10);
  Edit_SetText(HSuggestionsNum, Buf);
}

void SimpleDlg::SetOneUserDic(BOOL Value) {
  Button_SetCheck(HOneUserDic, Value ? BST_CHECKED : BST_UNCHECKED);
}

SpellerType SimpleDlg::selectedSpellerType() {
  return static_cast<SpellerType>(ComboBox_GetCurSel(HSpellerTypeCombo));
}

static int CALLBACK
BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM lpData) {
  // If the BFFM_INITIALIZED message is received
  // set the path to the start path.
  switch (uMsg) {
    case BFFM_INITIALIZED: {
      if (NULL != lpData) {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
      }
    }
  }

  return 0;  // The function should always return 0.
}

void SimpleDlg::onSelectedSpellerChange ()
{
  auto settingsCopy =
    std::make_unique<SettingsData> (*getSpellChecker()->getSettings ());
  auto newSpellerType = selectedSpellerType();
  settingsCopy->activeSpellerType = newSpellerType;
  PostMessageToMainThread (TM_SETTINGS_CHANGED,
    reinterpret_cast<WPARAM>(settingsCopy.release ()));
}

BOOL CALLBACK
SimpleDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) {
  TCHAR Buf[DEFAULT_BUF_SIZE];
  int x;
  TCHAR *EndPtr;

  switch (message) {
    case WM_INITDIALOG: {
      // Retrieving handles of dialog controls
      HComboLanguage = ::GetDlgItem(_hSelf, IDC_COMBO_LANGUAGE);
      HSuggestionsNum = ::GetDlgItem(_hSelf, IDC_SUGGESTIONS_NUM);
      HAspellStatus = ::GetDlgItem(_hSelf, IDC_ASPELL_STATUS);
      HLibPath = ::GetDlgItem(_hSelf, IDC_ASPELLPATH);
      HCheckNotThose = ::GetDlgItem(_hSelf, IDC_FILETYPES_CHECKNOTTHOSE);
      HCheckOnlyThose = ::GetDlgItem(_hSelf, IDC_FILETYPES_CHECKTHOSE);
      HFileTypes = ::GetDlgItem(_hSelf, IDC_FILETYPES);
      HCheckComments = ::GetDlgItem(_hSelf, IDC_CHECKCOMMENTS);
      HLibLink = ::GetDlgItem(_hSelf, IDC_LIB_LINK);
      HSuggType = ::GetDlgItem(_hSelf, IDC_SUGG_TYPE);
      HSpellerTypeCombo = ::GetDlgItem(_hSelf, IDC_LIBRARY);
      HLibGroupBox = ::GetDlgItem(_hSelf, IDC_LIB_GROUPBOX);
      HDownloadDics = ::GetDlgItem(_hSelf, IDC_DOWNLOADDICS);
      HRemoveDics = ::GetDlgItem(_hSelf, IDC_REMOVE_DICS);
      HDecodeNames = ::GetDlgItem(_hSelf, IDC_DECODE_NAMES);
      HOneUserDic = ::GetDlgItem(_hSelf, IDC_ONE_USER_DIC);
      HHunspellPathGroupBox = ::GetDlgItem(_hSelf, IDC_HUNSPELL_PATH_GROUPBOX);
      HHunspellPathType = ::GetDlgItem(_hSelf, IDC_HUNSPELL_PATH_TYPE);
      HAspellResetPath = ::GetDlgItem(_hSelf, IDC_RESETASPELLPATH);
      HHunspellResetPath = ::GetDlgItem(_hSelf, IDC_RESETHUNSPELLPATH);
      HSystemPath = ::GetDlgItem(_hSelf, IDC_SYSTEMPATH);
      ComboBox_AddString(HSpellerTypeCombo, _T ("Aspell"));
      ComboBox_AddString(HSpellerTypeCombo, _T ("Hunspell"));
      ComboBox_AddString(HHunspellPathType, _T ("For Current User"));
      ComboBox_AddString(HHunspellPathType, _T ("For All Users"));
      ComboBox_SetCurSel(HHunspellPathType, 0);
      ShowWindow(HLibPath, 1);
      ShowWindow(HSystemPath, 0);

      ComboBox_AddString(HSuggType, _T ("Special Suggestion Button"));
      ComboBox_AddString(HSuggType, _T ("Use N++ Context Menu"));
      DefaultBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

      AspellStatusColor = RGB(0, 0, 0);
      return TRUE;
    } break;
    case WM_CLOSE: {
      EndDialog(_hSelf, 0);
      DeleteObject(DefaultBrush);
      return FALSE;
    } break;
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDC_COMBO_LANGUAGE:
          if (HIWORD(wParam) == CBN_SELCHANGE) {
            if (ComboBox_GetCurSel(HComboLanguage) ==
                ComboBox_GetCount(HComboLanguage) - 1) {
              getLangListDialog()->DoDialog();
            }
          }
          break;
        case IDC_LIBRARY:
          if (HIWORD(wParam) == CBN_SELCHANGE) {
            onSelectedSpellerChange ();
          }
          break;
        case IDC_HUNSPELL_PATH_TYPE:
          if (HIWORD(wParam) == CBN_SELCHANGE) {
            if (ComboBox_GetCurSel(HHunspellPathType) == 0 ||
                selectedSpellerType() == SpellerType::aspell) {
              ShowWindow(HLibPath, 1);
              ShowWindow(HSystemPath, 0);
            } else {
              ShowWindow(HLibPath, 0);
              ShowWindow(HSystemPath, 1);
            }
          }
          break;
        case IDC_SUGGESTIONS_NUM: {
          if (HIWORD(wParam) == EN_CHANGE) {
            Edit_GetText(HSuggestionsNum, Buf, DEFAULT_BUF_SIZE);
            if (!*Buf) return TRUE;

            x = _tcstol(Buf, &EndPtr, 10);
            if (*EndPtr)
              Edit_SetText(HSuggestionsNum, _T ("5"));
            else if (x > 20)
              Edit_SetText(HSuggestionsNum, _T ("20"));
            else if (x < 1)
              Edit_SetText(HSuggestionsNum, _T ("1"));

            return TRUE;
          }
        } break;
        case IDC_REMOVE_DICS:
          if (HIWORD(wParam) == BN_CLICKED) {
            getRemoveDicsDialog()->DoDialog();
          }
          break;
        case IDC_RESETASPELLPATH:
        case IDC_RESETHUNSPELLPATH: {
          if (HIWORD(wParam) == BN_CLICKED) {
            TCHAR *Path = 0;
            auto type = selectedSpellerType();
            switch (type) {
              case SpellerType::aspell:
                GetDefaultAspellPath(Path);
                break;
              case SpellerType::hunspell:
                GetDefaultHunspellPath_(Path);
                break;
              case SpellerType::COUNT:
                break;
              default:
                break;
            }
            if (selectedSpellerType() == SpellerType::aspell ||
                ComboBox_GetCurSel(HHunspellPathType) == 0)
              Edit_SetText(HLibPath, Path);
            else
              Edit_SetText(HSystemPath, _T (".\\plugins\\config\\Hunspell"));

            CLEAN_AND_ZERO_ARR(Path);
            return TRUE;
          }
        } break;
        case IDC_DOWNLOADDICS: {
          if (HIWORD(wParam) == BN_CLICKED) {
            GetDownloadDics()->DoDialog();
          }
        } break;
        case IDC_BROWSEASPELLPATH:
          auto type = selectedSpellerType();
          switch (type) {
            case SpellerType::aspell: {
              OPENFILENAME ofn;
              ZeroMemory(&ofn, sizeof(ofn));
              ofn.lStructSize = sizeof(ofn);
              ofn.hwndOwner = _hSelf;
              TCHAR *Buf = new TCHAR[Edit_GetTextLength(HLibPath) + 1];
              Edit_GetText(HLibPath, Buf, DEFAULT_BUF_SIZE);
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
              if (GetOpenFileName(&ofn) == TRUE)
                Edit_SetText(HLibPath, ofn.lpstrFile);
            } break;
            case SpellerType::hunspell: {
              // Thanks to http://vcfaq.mvps.org/sdk/20.htm
              BROWSEINFO bi = {0};
              TCHAR path[MAX_PATH];

              LPITEMIDLIST pidlRoot = NULL;
              SHGetFolderLocation(_hSelf, 0, NULL, NULL, &pidlRoot);

              bi.pidlRoot = pidlRoot;
              bi.lpszTitle = _T ("Pick a Directory");
              bi.pszDisplayName = path;
              bi.ulFlags = BIF_RETURNONLYFSDIRS;
              bi.lpfn = BrowseCallbackProc;
              TCHAR *Buf = new TCHAR[Edit_GetTextLength(HLibPath) + 1];
              Edit_GetText(HLibPath, Buf, DEFAULT_BUF_SIZE);
              TCHAR NppPath[MAX_PATH];
              TCHAR FinalPath[MAX_PATH];
              BOOL ok;
              SendMsgToNpp(&ok, &NppDataInstance, NPPM_GETNPPDIRECTORY,
                           MAX_PATH, (LPARAM)NppPath);
              if (!ok) break;
              PathCombine(FinalPath, NppPath, Buf);
              bi.lParam = (LPARAM)FinalPath;
              LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
              if (pidl != 0) {
                // get the name of the folder
                TCHAR *szPath = new TCHAR[MAX_PATH];
                SHGetPathFromIDList(pidl, szPath);
                Edit_SetText(HLibPath, szPath);
                CoTaskMemFree(pidl);
                // free memory used

                CoUninitialize();
              }
            } break;
            case SpellerType::COUNT:
              break;
            default:
              break;
          }
          break;
      }
    } break;
    case WM_NOTIFY:

      switch (((LPNMHDR)lParam)->code) {
        case NM_CLICK:  // Fall through to the next case.
        case NM_RETURN: {
          PNMLINK pNMLink = (PNMLINK)lParam;
          LITEM item = pNMLink->item;

          if ((((LPNMHDR)lParam)->hwndFrom == HLibLink) && (item.iLink == 0)) {
            ShellExecute(NULL, _T ("open"), item.szUrl, NULL, NULL, SW_SHOW);
          }

          break;
        }
      }
      break;
    case WM_CTLCOLORSTATIC:
      if (GetDlgItem(_hSelf, IDC_ASPELL_STATUS) == (HWND)lParam) {
        HDC hDC = (HDC)wParam;
        HTHEME hTheme;

        if (_OpenThemeData)
          hTheme = _OpenThemeData(_hSelf, _T ("TAB"));
        else
          hTheme = 0;

        SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, AspellStatusColor);
        if (hTheme)
          return 0;
        else
          return (INT_PTR)DefaultBrush;
      }
      break;
  }
  return FALSE;
}

void SimpleDlg::updateOnConfigurationChange(const SettingsData &settings, const SpellCheckerStatus &status)
{
  ComboBox_ResetContent (HComboLanguage);
  for (auto &item : status.languageList)
    {
      ComboBox_AddString (HComboLanguage, settings.useLanguageNameAliases ? item.AliasName : item.OrigName);
      if (settings.activeSpellerSettings().activeLanguage == item.OrigName)
        ComboBox_SetCurSel (HComboLanguage, ComboBox_GetCount (HComboLanguage) - 1);
    }
  if (!status.languageList.empty ())
    ComboBox_AddString(HComboLanguage, _T ("Multiple Languages..."));

  EnableWindow(HComboLanguage, !status.languageList.empty ());

  if (settings.activeSpellerSettings().activeLanguage == multipleLanguagesStr)
    ComboBox_SetCurSel (HComboLanguage, ComboBox_GetCount (HComboLanguage) - 1);

  ComboBox_SetCurSel (HSpellerTypeCombo, static_cast<int> (settings.activeSpellerType));
  Edit_SetText (HSuggestionsNum, std::to_wstring (settings.suggestionCount).c_str ());

  Button_SetCheck (HDecodeNames, settings.useLanguageNameAliases);
  Button_SetCheck (HOneUserDic, settings.unifiedUserDictionary);

  Button_SetCheck (HCheckOnlyThose, BST_UNCHECKED);
  Button_SetCheck (HCheckNotThose, BST_UNCHECKED);
  switch (settings.fileMaskOption)
    {
    case FileMaskOption::inclusion:
      Button_SetCheck (HCheckOnlyThose, BST_CHECKED);
      break;
    case FileMaskOption::exclusion:
      Button_SetCheck (HCheckNotThose, BST_CHECKED);
      break;
    }

  Button_SetCheck (HCheckComments, settings.skipCode ? BST_CHECKED : BST_UNCHECKED);
  Edit_SetText (HFileTypes, settings.fileMask.c_str ());
  ComboBox_SetCurSel (HSuggType, static_cast<int> (settings.suggestionControlType));
  updateVisibility (settings, status);
}

const TCHAR *const IndicNames[] = {
    _T ("Plain"),        _T ("Squiggle"), _T ("TT"),   _T ("Diagonal"),
    _T ("Strike"),       _T ("Hidden"),   _T ("Box"),  _T ("Round Box"),
    _T ("Straight Box"), _T ("Dash"),     _T ("Dots"), _T ("Squiggle Low")};

BOOL CALLBACK
AdvancedDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) {
  TCHAR *EndPtr = 0;
  TCHAR Buf[DEFAULT_BUF_SIZE];
  int x;
  switch (message) {
    case WM_INITDIALOG: {
      // Retrieving handles of dialog controls
      HEditDelimiters = ::GetDlgItem(_hSelf, IDC_DELIMETERS);
      HDefaultDelimiters = ::GetDlgItem(_hSelf, IDC_DEFAULT_DELIMITERS);
      HIgnoreYo = ::GetDlgItem(_hSelf, IDC_COUNT_YO_AS_YE);
      HConvertSingleQuotes =
          ::GetDlgItem(_hSelf, IDC_COUNT_SINGLE_QUOTES_AS_APOSTROPHE);
      HRemoveBoundaryApostrophes =
          ::GetDlgItem(_hSelf, IDC_REMOVE_ENDING_APOSTROPHE);
      HRecheckDelay = ::GetDlgItem(_hSelf, IDC_RECHECK_DELAY);
      HUnderlineColor = ::GetDlgItem(_hSelf, IDC_UNDERLINE_COLOR);
      HUnderlineStyle = ::GetDlgItem(_hSelf, IDC_UNDERLINE_STYLE);
      HIgnoreNumbers = ::GetDlgItem(_hSelf, IDC_IGNORE_NUMBERS);
      HIgnoreCStart = ::GetDlgItem(_hSelf, IDC_IGNORE_CSTART);
      HIgnoreCHave = ::GetDlgItem(_hSelf, IDC_IGNORE_CHAVE);
      HIgnoreCAll = ::GetDlgItem(_hSelf, IDC_IGNORE_CALL);
      HIgnoreOneLetter = ::GetDlgItem(_hSelf, IDC_IGNORE_ONE_LETTER);
      HIgnore_ = ::GetDlgItem(_hSelf, IDC_IGNORE_);
      HIgnoreSEApostrophe = ::GetDlgItem(_hSelf, IDC_IGNORE_SE_APOSTROPHE);
      HSliderSize = ::GetDlgItem(_hSelf, IDC_SLIDER_SIZE);
      HSliderTransparency = ::GetDlgItem(_hSelf, IDC_SLIDER_TRANSPARENCY);
      HBufferSize = ::GetDlgItem(_hSelf, IDC_BUFFER_SIZE);
      SendMessage(HSliderSize, TBM_SETRANGE, TRUE, MAKELPARAM(5, 22));
      SendMessage(HSliderTransparency, TBM_SETRANGE, TRUE, MAKELPARAM(5, 100));

      Brush = 0;

      SetWindowText(HIgnoreYo, L"Cyrillic: Count io (\u0451) as e");
      CreateToolTip(
          IDC_DELIMETERS, _hSelf,
          _T ("Standard white-space symbols such as New Line ('\\n'), Carriage Return ('\\r'), Tab ('\\t'), Space (' ') are always counted as delimiters"));
      CreateToolTip(
          IDC_RECHECK_DELAY, _hSelf,
          _T ("Delay between the end of typing and rechecking the the text after it"));
      CreateToolTip(
          IDC_IGNORE_CSTART, _hSelf,
          _T ("Remember that words at the beginning of sentences would also be ignored that way."));
      CreateToolTip(
          IDC_IGNORE_SE_APOSTROPHE, _hSelf,
          _T ("Words like this sadly cannot be added to Aspell user dictionary"));
      CreateToolTip(
          IDC_REMOVE_ENDING_APOSTROPHE, _hSelf,
          _T ("Words like this are mostly mean plural possessive form in English, if you want to add such forms of words to dictionary manually, please uncheck"));

      ComboBox_ResetContent(HUnderlineStyle);
      for (int i = 0; i < countof(IndicNames); i++)
        ComboBox_AddString(HUnderlineStyle, IndicNames[i]);
      return TRUE;
    } break;
    case WM_CLOSE: {
      if (Brush) DeleteObject(Brush);
      EndDialog(_hSelf, 0);
      return FALSE;
    } break;
    case WM_DRAWITEM:
      switch (wParam) {
        case IDC_UNDERLINE_COLOR:
          HDC Dc;
          PAINTSTRUCT Ps;
          DRAWITEMSTRUCT *Ds = (LPDRAWITEMSTRUCT)lParam;
          Dc = Ds->hDC;
          /*
          Pen = CreatePen (PS_SOLID, 1, RGB (128, 128, 128));
          SelectPen (Dc, Pen);
          */
          if (Ds->itemState & ODS_SELECTED) {
            DrawEdge(Dc, &Ds->rcItem, BDR_SUNKENINNER | BDR_SUNKENOUTER,
                     BF_RECT);
          } else {
            DrawEdge(Dc, &Ds->rcItem, BDR_RAISEDINNER | BDR_RAISEDOUTER,
                     BF_RECT);
          }
          // DeleteObject (Pen);
          EndPaint(HUnderlineColor, &Ps);
          return TRUE;
      }
      break;
    case WM_CTLCOLORBTN:
      if (GetDlgItem(_hSelf, IDC_UNDERLINE_COLOR) == (HWND)lParam) {
        HDC hDC = (HDC)wParam;
        if (Brush) DeleteObject(Brush);

        SetBkColor(hDC, underlineColorBtn);
        SetBkMode(hDC, TRANSPARENT);
        Brush = CreateSolidBrush(underlineColorBtn);
        return (INT_PTR)Brush;
      }
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDC_DEFAULT_DELIMITERS:
          Edit_SetText (HEditDelimiters, DEFAULT_DELIMITERS);
          return TRUE;
        case IDC_RECHECK_DELAY:
          if (HIWORD(wParam) == EN_CHANGE) {
            Edit_GetText(HRecheckDelay, Buf, DEFAULT_BUF_SIZE);
            if (!*Buf) return TRUE;

            x = _tcstol(Buf, &EndPtr, 10);
            if (*EndPtr)
              Edit_SetText(HRecheckDelay, _T ("0"));
            else if (x > 30000)
              Edit_SetText(HRecheckDelay, _T ("30000"));
            else if (x < 0)
              Edit_SetText(HRecheckDelay, _T ("0"));

            return TRUE;
          }
          break;
        case IDC_BUFFER_SIZE:
          if (HIWORD(wParam) == EN_CHANGE) {
            Edit_GetText(HBufferSize, Buf, DEFAULT_BUF_SIZE);
            if (!*Buf) return TRUE;

            x = _tcstol(Buf, &EndPtr, 10);
            if (*EndPtr)
              Edit_SetText(HBufferSize, _T ("1024"));
            else if (x > 10 * 1024)
              Edit_SetText(HBufferSize, _T ("10240"));
            else if (x < 1)
              Edit_SetText(HBufferSize, _T ("1"));

            return TRUE;
          }
          break;
        case IDC_UNDERLINE_COLOR:
          if (HIWORD(wParam) == BN_CLICKED) {
            CHOOSECOLOR Cc;
            static COLORREF acrCustClr[16];
            memset(&Cc, 0, sizeof(Cc));
            Cc.hwndOwner = _hSelf;
            Cc.lStructSize = sizeof(CHOOSECOLOR);
            Cc.rgbResult = underlineColorBtn;
            Cc.lpCustColors = acrCustClr;
            Cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColor(&Cc) == TRUE) {
              underlineColorBtn = Cc.rgbResult;
              RedrawWindow(HUnderlineColor, 0, 0,
                           RDW_UPDATENOW | RDW_INVALIDATE | RDW_ERASE);
            }
          }
          break;
      }
  }
  return FALSE;
}

void AdvancedDlg::updateOnConfigurationChange(const SettingsData &settings)
{
  Edit_SetText (HEditDelimiters, settings.delimiters.c_str ());
  Button_SetCheck (HIgnoreCStart, settings.skipCapitalizedWords ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreNumbers, settings.skipNumbers ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreCHave, settings.skipWordsHavingCapitals ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreCAll, settings.skipWordsHavingOnlyCapitals ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnore_, settings.skipWordsWithUnderline ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreOneLetter, settings.skipOneLetterWords ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreSEApostrophe, settings.skipWordsStartingOrFinishingWithApostrophe ? BST_CHECKED : BST_UNCHECKED);
  Edit_SetText (HBufferSize, std::to_wstring (settings.bufferSizeForSearch).c_str ());

  Button_SetCheck (HRemoveBoundaryApostrophes, settings.skipStartingFinishingApostrophe ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HIgnoreYo, settings.treatYoAsYe ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HConvertSingleQuotes, settings.convertUnicodeQuotes ? BST_CHECKED : BST_UNCHECKED);

  underlineColorBtn = settings.underlineColor;
  ComboBox_SetCurSel (HUnderlineStyle, settings.underlineStyle);

  SendMessage (HSliderSize, TBM_SETPOS, TRUE, settings.suggestionBoxSize);
  SendMessage (HSliderTransparency, TBM_SETPOS, TRUE, settings.suggestionBoxTransparency);
}

void AdvancedDlg::SetDelimetersEdit (TCHAR *Delimiters) {
  Edit_SetText(HEditDelimiters, Delimiters);
}

void AdvancedDlg::SetRecheckDelay(int Delay) {
  TCHAR Buf[DEFAULT_BUF_SIZE];
  _itot(Delay, Buf, 10);
  Edit_SetText(HRecheckDelay, Buf);
}

int AdvancedDlg::GetRecheckDelay() {
  TCHAR Buf[DEFAULT_BUF_SIZE];
  Edit_GetText(HRecheckDelay, Buf, DEFAULT_BUF_SIZE);
  TCHAR *EndPtr;
  int x = _tcstol(Buf, &EndPtr, 10);
  return x;
}

void AdvancedDlg::applySettings(SettingsData &settings)
{
  settings.delimiters = getWindowText (HEditDelimiters);
  settings.underlineColor = underlineColorBtn;
  settings.underlineStyle = ComboBox_GetCurSel(HUnderlineStyle);
  settings.skipCapitalizedWords = (Button_GetCheck(HIgnoreCStart) == BST_CHECKED);
  settings.skipNumbers = (Button_GetCheck(HIgnoreNumbers) == BST_CHECKED);
  settings.skipWordsHavingCapitals = (Button_GetCheck(HIgnoreCHave) == BST_CHECKED);
  settings.skipWordsHavingOnlyCapitals = (Button_GetCheck(HIgnoreCAll) == BST_CHECKED);
  settings.skipWordsWithUnderline = (Button_GetCheck(HIgnore_) == BST_CHECKED);
  settings.skipWordsStartingOrFinishingWithApostrophe = (Button_GetCheck(HIgnoreSEApostrophe) == BST_CHECKED);
  settings.skipOneLetterWords = (Button_GetCheck(HIgnoreOneLetter) == BST_CHECKED);
  settings.skipStartingFinishingApostrophe = (Button_GetCheck (HRemoveBoundaryApostrophes) == BST_CHECKED);
  settings.treatYoAsYe = (Button_GetCheck (HIgnoreYo) == BST_CHECKED);
  settings.convertUnicodeQuotes = (Button_GetCheck (HConvertSingleQuotes) == BST_CHECKED);
  settings.suggestionBoxSize = SendMessage(HSliderSize, TBM_GETPOS, 0, 0);
  settings.suggestionBoxTransparency = SendMessage(HSliderTransparency, TBM_GETPOS, 0, 0);
  TCHAR *EndPtr;
  std::wstring ws = getWindowText(HBufferSize);
  int x = _tcstol(ws.c_str (), &EndPtr, 10);
  settings.bufferSizeForSearch = std::max (std::min (x, 10 * 1024), 1);
  GetDownloadDics()->UpdateListBox();
}

SimpleDlg *SettingsDlg::GetSimpleDlg() { return &simpleDlgInstance; }

AdvancedDlg *SettingsDlg::GetAdvancedDlg() { return &advancedDlgInstance; }

void SettingsDlg::init(HINSTANCE hInst, HWND Parent, NppData nppData) {
  NppDataInstance = nppData;
  return Window::init(hInst, Parent);
}

void SettingsDlg::destroy() {
  simpleDlgInstance.destroy();
  advancedDlgInstance.destroy();
};


void SettingsDlg::applySettings ()
{
  SetRecheckDelay (advancedDlgInstance.GetRecheckDelay ());

  auto settingsCopy = std::make_unique<SettingsData>(*getSpellChecker()->getSettings());

  simpleDlgInstance.applySettings(*settingsCopy);
  advancedDlgInstance.applySettings(*settingsCopy);

  PostMessageToMainThread (TM_SETTINGS_CHANGED,
    reinterpret_cast<WPARAM>(settingsCopy.release ()));
}

void SettingsDlg::updateOnConfigurationChange ()
{
  if (!isCreated ())
    return;

  auto status = getSpellChecker ()->getStatus ();
  auto settings = getSpellChecker ()->getSettings ();
  if (!status || !settings) // better to hide everything in that case though
    return;

  simpleDlgInstance.updateOnConfigurationChange (*settings, *status);
  advancedDlgInstance.updateOnConfigurationChange (*settings);
}

BOOL CALLBACK
SettingsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) {
  switch (Message) {
    case WM_INITDIALOG: {
      ControlsTabInstance.init(_hInst, _hSelf, false, true, false);
      ControlsTabInstance.setFont(TEXT("Tahoma"), 13);

      simpleDlgInstance.init(_hInst, _hSelf, NppDataInstance);
      simpleDlgInstance.create(IDD_SIMPLE, false, false);
      simpleDlgInstance.display();
      advancedDlgInstance.init(_hInst, _hSelf);
      advancedDlgInstance.create(IDD_ADVANCED, false, false);
      advancedDlgInstance.SetRecheckDelay(GetRecheckDelay());

      WindowVectorInstance.push_back(
          DlgInfo(&simpleDlgInstance, TEXT("Simple"), TEXT("Simple Options")));
      WindowVectorInstance.push_back(DlgInfo(
          &advancedDlgInstance, TEXT("Advanced"), TEXT("Advanced Options")));
      ControlsTabInstance.createTabs(WindowVectorInstance);
      ControlsTabInstance.display();
      RECT rc;
      getClientRect(rc);
      ControlsTabInstance.reSizeTo(rc);
      rc.bottom -= 30;

      simpleDlgInstance.reSizeTo(rc);
      advancedDlgInstance.reSizeTo(rc);

      // This stuff is copied from npp source to make tabbed window looked
      // totally nice and white
      ETDTProc enableDlgTheme = (ETDTProc)::SendMessage(
          _hParent, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
      if (enableDlgTheme) enableDlgTheme(_hSelf, ETDT_ENABLETAB);

      updateOnConfigurationChange();

      return TRUE;
    } break;
    case WM_CONTEXTMENU: {
      HMENU menu = CreatePopupMenu();
      AppendMenu(
          menu, MF_STRING, 0,
          _T ("Copy All Misspelled Words in Current Document to Clipboard"));
      TrackPopupMenuEx(menu, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
                       _hSelf, NULL);
    } break;
    case WM_NOTIFY: {
      NMHDR *nmhdr = (NMHDR *)lParam;
      if (nmhdr->code == TCN_SELCHANGE) {
        if (nmhdr->hwndFrom == ControlsTabInstance.getHSelf()) {
          ControlsTabInstance.clickedUpdate();
          return FALSE;
        }
      }
      break;
    } break;
    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case 0:  // Menu
        {
          SendEvent(EID_COPY_MISSPELLINGS_TO_CLIPBOARD);
        } break;
        case IDAPPLY:
          if (HIWORD(wParam) == BN_CLICKED) {
            applySettings();

            return FALSE;
          }
          break;
        case IDOK:
          if (HIWORD(wParam) == BN_CLICKED) {
            SendEvent(EID_HIDE_DIALOG);
            applySettings();
            return FALSE;
          }
          break;
        case IDCANCEL:
          if (HIWORD(wParam) == BN_CLICKED) {
            SendEvent(EID_HIDE_DIALOG);
            return FALSE;
          }
          break;
      }
    }
  }
  return FALSE;
}

UINT SettingsDlg::DoDialog(void) {
  if (!isCreated()) {
    create(IDD_SETTINGS);
  }

  goToCenter();
  display ();
  return TRUE;
}