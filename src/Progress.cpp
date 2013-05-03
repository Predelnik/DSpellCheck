#include "Progress.h"

#include "resource.h"

void Progress::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_DIALOGPROGRESS);
  }
  else
  {
    goToCenter ();
    display ();
  }
}

void Progress::init (HINSTANCE hInst, HWND Parent)
{
  return Window::init (hInst, Parent);
}

BOOL CALLBACK Progress::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      HDescTop = GetDlgItem (_hSelf, IDC_DESCTOP);
      HDescBottom = GetDlgItem (_hSelf, IDC_DESCBOTTOM);
      HProgressBar = GetDlgItem (_hSelf, IDC_PROGRESSBAR);
      SendMessage (HProgressBar, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0,100));
      return TRUE;
    }
    break;
  };
  return FALSE;
}

void Progress::SetProgress (int value)
{
  SendMessage (HProgressBar, PBM_SETPOS, value, 0);
}

void Progress::SetBottomMessage (TCHAR *Message)
{
  Static_SetText (HDescBottom, Message);
}

void Progress::SetTopMessage (TCHAR *Message)
{
  Static_SetText (HDescTop, Message);
}

Progress::Progress(void)
{
}

Progress::~Progress(void)
{
}