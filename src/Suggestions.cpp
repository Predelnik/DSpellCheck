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

#include "Suggestions.h"

#include "MainDef.h"
#include "plugin.h"
#include "resource.h"

#define MOUSELEAVE      0x0001
#define MOUSEHOVER      0x0002

void Suggestions::DoDialog ()
{
  HWND Temp = GetFocus ();
  create (IDD_SUGGESTIONS);
  SetFocus (Temp);
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
    RECT R;
    GetWindowRect (_hSelf, &R);
    StretchBlt (Dc, 0, 0, R.right - R.left, R.bottom - R.top, hdcMemory, 0, 0, Bmp.bmWidth, Bmp.bmHeight, SRCCOPY);
    SelectObject (hdcMemory, hOldBitmap);
    DeleteDC(hdcMemory);
    DeleteObject(hBmp);
    EndPaint(_hSelf, &ps);
    RECT Rect;
    GetWindowRect (_hSelf, &Rect);
    ValidateRect (_hSelf, &Rect);
    return TRUE;

  case WM_SHOWANDRECREATEMENU:
    tagTPMPARAMS TPMParams;
    TPMParams.cbSize = sizeof (tagTPMPARAMS);
    GetWindowRect (_hSelf, &TPMParams.rcExclude);
    p.x = 0; p.y = 0;
    ClientToScreen (_hSelf, &p);
    StateMenu = TRUE;
    MenuResult = TrackPopupMenuEx (PopupMenu, TPM_HORIZONTAL | TPM_RIGHTALIGN, p.x, p.y, _hSelf, &TPMParams);
    SetFocus (_hParent);
    SendEvent (EID_APPLYMENUACTION);
    StateMenu = FALSE;
    DestroyMenu (PopupMenu);
    PopupMenu = CreatePopupMenu ();
    return TRUE;

  case WM_COMMAND:
    if (HIWORD (wParam) == 0)
      PostMessageToMainThread (TM_MENU_RESULT, wParam, 0);
    return TRUE;

  case WM_MOUSELEAVE:
    StateHovered = FALSE;
    StatePressed = FALSE;
    RedrawWindow (_hSelf, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    return TRUE;
  }
  return FALSE;
}