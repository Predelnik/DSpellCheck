#include "Suggestions.h"

#include "MainDef.h"
#include "plugin.h"
#include "resource.h"

#define MOUSELEAVE      0x0001
#define MOUSEHOVER      0x0002

void Suggestions::DoDialog ()
{
  create (IDD_SUGGESTIONS);
}

BOOL RegMsg(HWND hWnd, DWORD dwMsgType)
{
  TRACKMOUSEEVENT tmeEventTrack;  // tracking information

  tmeEventTrack.cbSize = sizeof(TRACKMOUSEEVENT);
  tmeEventTrack.dwFlags = 0;

  if(dwMsgType & MOUSELEAVE)
    tmeEventTrack.dwFlags |= TME_LEAVE;
  if(dwMsgType & MOUSEHOVER)
    tmeEventTrack.dwFlags |= TME_HOVER;

  tmeEventTrack.hwndTrack = hWnd;
  tmeEventTrack.dwHoverTime = HOVER_DEFAULT;
  //      tmeEventTrack.dwHoverTime = 10;

  if( !TrackMouseEvent(&tmeEventTrack) )
  {
    TCHAR szError[64];

    _stprintf (szError, _T ("Can't TrackMouseEvent ! ErrorId: %d"), GetLastError());
    MessageBox(hWnd, szError, _T ("Error"), MB_OK|MB_ICONSTOP);

    return FALSE;
  }

  return TRUE;
}

HMENU Suggestions::GetPopupMenu ()
{
  return PopupMenu;
}

int Suggestions::GetResult ()
{
  return MenuResult;
}

BOOL CALLBACK Suggestions::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
  POINT p;
  switch (Message)
  {
  case WM_INITDIALOG:
    display (false);
    PopupMenu = CreatePopupMenu ();
    return TRUE;
  case WM_MOUSEMOVE:
    RegMsg(_hSelf, MOUSELEAVE);
    return TRUE;

  case WM_SHOWANDRECREATEMENU:
    p.x = 0; p.y = 0;
    ClientToScreen (_hSelf, &p);
    MenuResult = TrackPopupMenu (PopupMenu, TPM_RETURNCMD, p.x, p.y, 0, _hSelf, 0);
    SendEvent (EID_APPLYMENUACTION);

    DestroyMenu (PopupMenu);
    PopupMenu = CreatePopupMenu ();
    return TRUE;

  case WM_MOUSELEAVE:
    return TRUE;
  }
  return FALSE;
}
