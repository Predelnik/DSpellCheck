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
#include "Plugin.h"
#include "resource.h"
#include "SpellChecker.h"

#include "CommonFunctions.h"

#define MOUSELEAVE 0x0001
#define MOUSEHOVER 0x0002

void Suggestions::DoDialog() {
  HWND Temp = GetFocus();
  create(IDD_SUGGESTIONS);
  SetFocus(Temp);
}

bool RegMsg(HWND hWnd, DWORD dwMsgType) {
  TRACKMOUSEEVENT tmeEventTrack; // tracking information

  tmeEventTrack.cbSize = sizeof(TRACKMOUSEEVENT);
  tmeEventTrack.dwFlags = 0;

  if (dwMsgType & MOUSELEAVE)
    tmeEventTrack.dwFlags |= TME_LEAVE;
  if (dwMsgType & MOUSEHOVER)
    tmeEventTrack.dwFlags |= TME_HOVER;

  tmeEventTrack.hwndTrack = hWnd;
  tmeEventTrack.dwHoverTime = HOVER_DEFAULT;
  //      tmeEventTrack.dwHoverTime = 10;

  if (!TrackMouseEvent(&tmeEventTrack)) {
    wchar_t szError[64];

    swprintf(szError, L"Can't TrackMouseEvent ! ErrorId: %d", GetLastError());
    MessageBox(hWnd, szError, L"Error", MB_OK | MB_ICONSTOP);

    return false;
  }

  return true;
}

HMENU Suggestions::GetPopupMenu() { return PopupMenu; }

int Suggestions::GetResult() { return MenuResult; }

Suggestions::Suggestions() {
  StatePressed = false;
  StateHovered = false;
  StateMenu = false;
}

void Suggestions::initDlg(HINSTANCE hInst, HWND Parent, NppData nppData) {
  NppDataInstance = nppData;
  return Window::init(hInst, Parent);
}

INT_PTR Suggestions::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) {
  POINT p;
  HDC Dc;
  HBITMAP hBmp;
  HBITMAP hOldBitmap;
  BITMAP Bmp;
  int idImg = 0;
  PAINTSTRUCT ps;

  switch (Message) {
  case WM_INITDIALOG:

    display(false);
    PopupMenu = CreatePopupMenu();

    return false;

  case WM_MOUSEMOVE:
    StateHovered = true;
    RegMsg(_hSelf, MOUSELEAVE);
    RedrawWindow(_hSelf, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    return false;

  case WM_MOUSEHOVER:
    return false;

  case WM_LBUTTONDOWN:
    StatePressed = true;
    RedrawWindow(_hSelf, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    return false;

  case WM_LBUTTONUP:
    if (!StatePressed)
      return false;
    getSpellChecker()->showSuggestionMenu();
    return false;

  case WM_MOUSEWHEEL:
    ShowWindow(_hSelf, SW_HIDE);
    PostMessage(GetScintillaWindow(&NppDataInstance), Message, wParam, lParam);
    return false;

  case WM_PAINT:
    if (!isVisible())
      return false;

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
    hOldBitmap = (HBITMAP)SelectObject(hdcMemory, hBmp);
    SetStretchBltMode(Dc, HALFTONE);
    RECT R;
    GetWindowRect(_hSelf, &R);
    StretchBlt(Dc, 0, 0, R.right - R.left, R.bottom - R.top, hdcMemory, 0, 0,
               Bmp.bmWidth, Bmp.bmHeight, SRCCOPY);
    SelectObject(hdcMemory, hOldBitmap);
    DeleteDC(hdcMemory);
    DeleteObject(hBmp);
    EndPaint(_hSelf, &ps);
    RECT Rect;
    GetWindowRect(_hSelf, &Rect);
    ValidateRect(_hSelf, &Rect);
    return false;

  case WM_SHOWANDRECREATEMENU:
    tagTPMPARAMS TPMParams;
    TPMParams.cbSize = sizeof(tagTPMPARAMS);
    GetWindowRect(_hSelf, &TPMParams.rcExclude);
    p.x = 0;
    p.y = 0;
    ClientToScreen(_hSelf, &p);
    StateMenu = true;
    SetForegroundWindow(NppDataInstance._nppHandle);
    MenuResult = TrackPopupMenuEx(PopupMenu, TPM_HORIZONTAL | TPM_RIGHTALIGN,
                                  p.x, p.y, _hSelf, &TPMParams);
    PostMessage(NppDataInstance._nppHandle, WM_NULL, 0, 0);
    SetFocus(GetScintillaWindow(&NppDataInstance));
    StateMenu = false;
    DestroyMenu(PopupMenu);
    PopupMenu = CreatePopupMenu();
    return false;

  case WM_COMMAND:
    if (HIWORD(wParam) == 0)
      getSpellChecker()->ProcessMenuResult(wParam);

    return false;

  case WM_MOUSELEAVE:
    StateHovered = false;
    StatePressed = false;
    RedrawWindow(_hSelf, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    return false;
  }
  return false;
}