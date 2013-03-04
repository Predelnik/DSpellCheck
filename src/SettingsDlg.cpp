#include "SettingsDlg.h"

#include "aspell.h"
#include "CommonFunctions.h"
#include "MainDef.h"
#include "Plugin.h"
#include "SpellChecker.h"

#include "resource.h"

#include <windowsx.h>

#include <uxtheme.h>

void SimpleDlg::init (HINSTANCE hInst, HWND Parent, NppData nppData)
{
  NppDataInstance = nppData;
  return Window::init (hInst, Parent);
}

// Called from main thread, beware!
BOOL SimpleDlg::AddAvalaibleLanguages (const char *CurrentLanguage)
{
  AspellConfig* aspCfg;
  AspellDictInfoList* dlist;
  AspellDictInfoEnumeration* dels;
  const AspellDictInfo* entry;

  aspCfg = new_aspell_config();

  /* the returned pointer should _not_ need to be deleted */
  dlist = get_aspell_dict_info_list(aspCfg);

  /* config is no longer needed */
  delete_aspell_config(aspCfg);

  dels = aspell_dict_info_list_elements(dlist);

  if (aspell_dict_info_enumeration_at_end(dels) == TRUE)
  {
    delete_aspell_dict_info_enumeration(dels);
    return FALSE;
  }

  UINT uElementCnt= 0;
  int SelectedIndex = 0;
  int i = 0;
  while ((entry = aspell_dict_info_enumeration_next(dels)) != 0)
  {
    // Well since this strings comes in ansi, the simplest way is too call corresponding function
    // Without using windowsx.h
    if (strcmp (CurrentLanguage, entry->name) == 0)
      SelectedIndex = i;

    i++;
    TCHAR *TBuf = 0;
    SetString (TBuf, entry->name);
    ComboBox_AddString (HComboLanguage, TBuf);
    CLEAN_AND_ZERO_ARR (TBuf);
  }
  ComboBox_SetCurSel (HComboLanguage, SelectedIndex);

  delete_aspell_dict_info_enumeration(dels);
  return TRUE;
}

// Called from main thread, beware!
void SimpleDlg::ApplySettings (SpellChecker *SpellCheckerInstance)
{
  int length = GetWindowTextLengthA (HComboLanguage);
  char *LangString = new char[length + 1];
  GetWindowTextA (HComboLanguage, LangString, length + 1);
  SpellCheckerInstance->SetLanguage (LangString);
  SpellCheckerInstance->RecheckVisible ();
  CLEAN_AND_ZERO_ARR (LangString);
}

BOOL CALLBACK SimpleDlg::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  char *LangString = NULL;
  int length = 0;

  switch (message)
  {
  case WM_INITDIALOG:
    {
      // Retrieving handles of dialog controls
      HComboLanguage = ::GetDlgItem(_hSelf, IDC_COMBO_LANGUAGE);
      return TRUE;
    }
  case WM_CLOSE:
    {
      EndDialog(_hSelf, 0);
      return TRUE;
    }
  }
  return FALSE;
}

void AdvancedDlg::FillDelimeters (const char *Delimeters)
{
  TCHAR *TBuf = 0;
  SetStringSUtf8 (TBuf, Delimeters);
  Edit_SetText (HEditDelimeters, TBuf);
  CLEAN_AND_ZERO_ARR (TBuf);
}

BOOL CALLBACK AdvancedDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      // Retrieving handles of dialog controls
      HEditDelimeters = ::GetDlgItem(_hSelf, IDC_DELIMETERS);
      return TRUE;
    }
  case WM_CLOSE:
    {
      EndDialog(_hSelf, 0);
      return TRUE;
    }
  }
  return FALSE;
}

// Called from main thread, beware!
void AdvancedDlg::ApplySettings (SpellChecker *SpellCheckerInstance)
{
  TCHAR *TBuf = 0;
  int Length = Edit_GetTextLength (HEditDelimeters);
  TBuf = new TCHAR[Length + 1];
  Edit_GetText (HEditDelimeters, TBuf, Length + 1);
  char *BufUtf8 = 0;
  SetStringDUtf8 (BufUtf8, TBuf);
  SpellCheckerInstance->SetDelimeters (BufUtf8);
  CLEAN_AND_ZERO_ARR (BufUtf8);
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

BOOL CALLBACK SettingsDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
  switch (Message)
  {
  case WM_INITDIALOG :
    {
      ControlsTabInstance.init (_hInst, _hSelf, false, true, false);
      ControlsTabInstance.setFont(TEXT("Tahoma"), 13);

      SimpleDlgInstance.init(_hInst, _hSelf, NppDataInstance);
      SimpleDlgInstance.create (IDD_SIMPLE, false, false);
      SimpleDlgInstance.display ();
      AdvancedDlgInstance.init(_hInst, _hSelf);
      AdvancedDlgInstance.create (IDD_ADVANCED, false, false);
      SendEvent (EID_FILL_DIALOGS);

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

      return TRUE;
    }

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

  case WM_COMMAND :
    {
      switch (wParam)
      {
      case IDOK:
        SendEvent (EID_APPLY_SETTINGS);
      case IDCANCEL :
        SendEvent (EID_HIDE_DIALOG);
        return TRUE;

      default :
        ::SendMessage(_hParent, WM_COMMAND, wParam, lParam);
        return TRUE;
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
  display ();
  return TRUE;
  // return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SETTINGS), _hParent, (DLGPROC)dlgProc, (LPARAM)this);
}

