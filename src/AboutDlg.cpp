#include <CommCtrl.h>

#include "AboutDlg.h"
#include "resource.h"

void AboutDlg::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_ABOUT);
    goToCenter ();
  }
  display ();
}

void AboutDlg::init (HINSTANCE hInst, HWND Parent)
{
  return Window::init (hInst, Parent);
}

BOOL CALLBACK AboutDlg::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    return TRUE;
  case WM_NOTIFY:
    {
      switch (((LPNMHDR)lParam)->code)
      {
      case NM_CLICK:          // Fall through to the next case.
      case NM_RETURN:
        {
          PNMLINK pNMLink = (PNMLINK)lParam;
          LITEM   item    = pNMLink->item;

          if ((item.iLink == 0))
          {
            ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
          }

          return TRUE;
        }
      }
      return FALSE;
    }
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDOK:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          display (false);
          return TRUE;
        }
      }
    }
  }
  return FALSE;
}