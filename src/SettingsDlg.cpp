#include <windows.h>

#include "aspell.h"
#include "PluginDefinition.h"
#include "SettingsDlg.h"

#include "resource.h"

#include <windowsx.h>

SettingsDlg::SettingsDlg ()
{
}

UINT SettingsDlg::DoDialog (void)
{
  return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_SETTINGS), _hParent, (DLGPROC)dlgProc, (LPARAM)this);
}

void SettingsDlg::init(HINSTANCE hInst, NppData nppData)
{
  NppDataInstance = nppData;
  StaticDialog::init (hInst, nppData._nppHandle);
}

BOOL SettingsDlg::AddAvalaibleLanguages ()
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
    if (strcmp (GetLanguage (), entry->name) == 0)
      SelectedIndex = i;

    i++;
    SendMessageA(HComboLanguage, CB_ADDSTRING, 0, (LPARAM)entry->name);
  }
  ComboBox_SetCurSel (HComboLanguage, SelectedIndex);

  delete_aspell_dict_info_enumeration(dels);
  return TRUE;
}

BOOL CALLBACK SettingsDlg::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  char *LangString = NULL;
  int length = 0;

  switch (message)
  {
  case WM_INITDIALOG:
    {
      // Retrieving handles of dialog controls
      HComboLanguage = ::GetDlgItem(_hSelf, IDC_COMBO_LANGUAGE);
      HOk = ::GetDlgItem(_hSelf, IDOK);
      HCancel = ::GetDlgItem(_hSelf, IDCANCEL);
      AddAvalaibleLanguages ();
      return TRUE;
    }
  case WM_CLOSE:
    {
      EndDialog(_hSelf, 0);
      return TRUE;
    }
  case WM_COMMAND:
    {
      if (HIWORD(wParam) == BN_CLICKED)
      {
        switch (LOWORD(wParam))
        {
        case IDOK:
          length = GetWindowTextLengthA (HComboLanguage);
          LangString = new char[length + 1];
          GetWindowTextA (HComboLanguage, LangString, length + 1);
          SetLanguage (LangString);
          RecheckDocument ();
          EndDialog(_hSelf, 0);
          return TRUE;
        case IDCANCEL:
          EndDialog(_hSelf, 0);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}
