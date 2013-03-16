#include <CommCtrl.h>

#include "LangList.h"
#include "Controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "CommonFunctions.h"
#include "SpellChecker.h"
#include "Plugin.h"

#include "resource.h"

void LangList::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_CHOOSE_MULTIPLE_LANGUAGES, false, false);
  }
  else
  {
    goToCenter ();
    display ();
  }
}

void LangList::init (HINSTANCE hInst, HWND Parent)
{
  return Window::init (hInst, Parent);
}

HWND LangList::GetListBox ()
{
  return HLangList;
}

void LangList::ApplyChoice (SpellChecker *SpellCheckerInstance)
{
  int count = ListBox_GetCount (HLangList);
  TCHAR Buf[DEFAULT_BUF_SIZE];
  TCHAR *ItemBuf = 0;
  int ItemBufLen = 0;
  char *ConvertedBuf = 0;
  BOOL FirstOne = TRUE;
  Buf[0] = _T ('\0');
  for (int i = 0; i < count; i++)
  {
    if (CheckedListBox_GetCheckState (HLangList, i))
    {
      ItemBufLen = ListBox_GetTextLen (HLangList, i);
      ItemBuf = new TCHAR[ItemBufLen + 1];
      ListBox_GetText (HLangList, i, ItemBuf);
      if (!FirstOne)
        _tcscat (Buf, _T ("|"));

      _tcscat_s (Buf, ItemBuf);
      FirstOne = FALSE;
      CLEAN_AND_ZERO_ARR (ItemBuf);
    }
  }
  SetStringDUtf8 (ConvertedBuf, Buf);
  SpellCheckerInstance->SetMultipleLanguages (ConvertedBuf);
  CLEAN_AND_ZERO_ARR (ConvertedBuf);
}

BOOL CALLBACK LangList::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      HLangList = ::GetDlgItem (_hSelf, IDC_LANGLIST);
      return TRUE;
    }
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDOK:
        SendEvent (EID_APPLY_MULTI_LANG_SETTINGS);
      case IDCANCEL:
        if (HIWORD (wParam) == BN_CLICKED)
          display (false);

        break;
      }
    }
  };
  return FALSE;
}
