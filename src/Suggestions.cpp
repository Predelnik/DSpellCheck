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

Suggestions::Suggestions ()
{
  StatePressed = FALSE;
  StateHovered = FALSE;
  StateMenu = FALSE;
}

BOOL CALLBACK Suggestions::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
  POINT p;
  HDC Dc;
  HBITMAP hBmp;
  HBITMAP hOldBitmap;
  BITMAP Bmp;
  int idImg = 0;
  PAINTSTRUCT ps;

  switch (Message)
  {
  case WM_INITDIALOG:

    display (false);
    PopupMenu = CreatePopupMenu ();

    return TRUE;

  case WM_MOUSEMOVE:
    StateHovered = TRUE;
    RegMsg(_hSelf, MOUSELEAVE);
    RedrawWindow (_hSelf, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    return TRUE;

  case WM_MOUSEHOVER:
    return TRUE;

  case WM_LBUTTONDOWN:
    StatePressed = TRUE;
    RedrawWindow (_hSelf, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    return TRUE;

  case WM_LBUTTONUP:
    if (!StatePressed)
      return TRUE;
    SendEvent (EID_SHOW_SUGGESTION_MENU);
    return TRUE;

  case WM_PAINT:
    if (!isVisible ())
      return TRUE;

    idImg = IDR_DOWNARROW;
    if (StatePressed || StateMenu)
      idImg = IDR_DOWNARROW_PUSH;
    else if (StateHovered)
      idImg = IDR_DOWNARROW_HOVER;

    HDC hdcMemory;
    Dc = BeginPaint(_hSelf, &ps);
    hdcMemory = ::CreateCompatibleDC(Dc);
    hBmp = LoadBitmap(_hInst, MAKEINTRESOURCE(idImg));
    GetObject(hBmp, sizeof(Bmp), &Bmp);
    hOldBitmap = (HBITMAP) SelectObject(hdcMemory, hBmp);
    SetStretchBltMode (Dc, HALFTONE);
    StretchBlt (Dc, 0, 0, 15, 15, hdcMemory, 0, 0, Bmp.bmWidth, Bmp.bmHeight, SRCCOPY);
    SelectObject (hdcMemory, hOldBitmap);
    DeleteDC(hdcMemory);
    DeleteObject(hBmp);
    EndPaint(_hSelf, &ps);
    return TRUE;

  case WM_SHOWANDRECREATEMENU:
    p.x = getWidth (); p.y = 0;
    ClientToScreen (_hSelf, &p);
    StateMenu = TRUE;
    MenuResult = TrackPopupMenu (PopupMenu, TPM_RETURNCMD, p.x, p.y, 0, _hSelf, 0);
    SendEvent (EID_APPLYMENUACTION);
    StateMenu = FALSE;

    DestroyMenu (PopupMenu);
    PopupMenu = CreatePopupMenu ();
    return TRUE;

  case WM_MOUSELEAVE:
    StateHovered = FALSE;
    StatePressed = FALSE;
    RedrawWindow (_hSelf, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    return TRUE;
  }
  return FALSE;
}
