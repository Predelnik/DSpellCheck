// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "SuggestionsButton.h"

#include "npp/NppInterface.h"
#include "resource.h"
#include "ContextMenuHandler.h"
#include "MenuItem.h"
#include "Settings.h"
#include "WindowsDefs.h"

#define MOUSELEAVE 0x0001
#define MOUSEHOVER 0x0002

void SuggestionsButton::do_dialog() {
  HWND temp = GetFocus();
  create(IDD_SUGGESTIONS);
  SetFocus(temp);
}

bool reg_msg(HWND h_wnd, DWORD dw_msg_type) {
  TRACKMOUSEEVENT tme_event_track; // tracking information

  tme_event_track.cbSize = sizeof(TRACKMOUSEEVENT);
  tme_event_track.dwFlags = 0;

  if ((dw_msg_type & MOUSELEAVE) != 0u)
    tme_event_track.dwFlags |= TME_LEAVE;
  if ((dw_msg_type & MOUSEHOVER) != 0u)
    tme_event_track.dwFlags |= TME_HOVER;

  tme_event_track.hwndTrack = h_wnd;
  tme_event_track.dwHoverTime = HOVER_DEFAULT;
  //      tmeEventTrack.dwHoverTime = 10;

  if (TrackMouseEvent(&tme_event_track) == 0) {
    wchar_t sz_error[64];

    swprintf(sz_error, L"Can't TrackMouseEvent ! ErrorId: %d", GetLastError());
    MessageBox(h_wnd, sz_error, L"Error", MB_OK | MB_ICONSTOP);

    return false;
  }

  return true;
}

int SuggestionsButton::get_result() const { return m_menu_result; }

SuggestionsButton::SuggestionsButton(HINSTANCE h_inst, HWND parent,
                                     NppInterface &npp, ContextMenuHandler &context_menu_handler,
                                     const Settings &settings)
    : m_menu_result(0), m_npp(npp),
      m_settings (settings), m_context_menu_handler(context_menu_handler) {
  Window::init(h_inst, parent);
  m_state_pressed = false;
  m_state_hovered = false;
  m_state_menu = false;
}

void SuggestionsButton::show_suggestion_menu() {
  SendMessage(getHSelf(), WM_SHOWANDRECREATEMENU, 0, 0);
}

void SuggestionsButton::on_settings_changed() {
  display(false);
  SetLayeredWindowAttributes(
      getHSelf(), 0,
      static_cast<BYTE>((255 * m_settings.suggestion_button_opacity) / 100),
      LWA_ALPHA);
}

void SuggestionsButton::set_transparency() {
  SetWindowLong(getHSelf(), GWL_EXSTYLE,
                GetWindowLong(getHSelf(), GWL_EXSTYLE) | WS_EX_LAYERED);
  SetLayeredWindowAttributes(
      getHSelf(), 0,
      static_cast<BYTE>((255 * m_settings.suggestion_button_opacity) / 100),
      LWA_ALPHA);
  display(true);
  display(false);
}

INT_PTR SuggestionsButton::run_dlg_proc(UINT message, WPARAM w_param,
                                        LPARAM l_param) {
  POINT p;
  BITMAP bmp;
  int id_img;
  PAINTSTRUCT ps;

  switch (message) {
  case WM_INITDIALOG:

    display(false);
    return 0;

  case WM_MOUSEMOVE:
    m_state_hovered = true;
    reg_msg(_hSelf, MOUSELEAVE);
    RedrawWindow(_hSelf, nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE);
    return 0;

  case WM_MOUSEHOVER:
    return 0;

  case WM_LBUTTONDOWN:
    m_state_pressed = true;
    RedrawWindow(_hSelf, nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE);
    return 0;

  case WM_LBUTTONUP:
    if (!m_state_pressed)
      return 0;
    show_suggestion_menu();
    return 0;

  case WM_MOUSEWHEEL:
    ShowWindow(_hSelf, SW_HIDE);
    PostMessage(m_npp.get_scintilla_hwnd(m_npp.active_view()), message, w_param,
                l_param);
    return 0;

  case WM_PAINT: {
    if (!isVisible())
      return 0;

    id_img = IDR_DOWNARROW;
    if (m_state_pressed || m_state_menu)
      id_img = IDR_DOWNARROW_PUSH;
    else if (m_state_hovered)
      id_img = IDR_DOWNARROW_HOVER;

    HDC dc = BeginPaint(_hSelf, &ps);
    HDC hdc_memory = ::CreateCompatibleDC(dc);
    HBITMAP h_bmp = LoadBitmap(_hInst, MAKEINTRESOURCE(id_img));
    GetObject(h_bmp, sizeof(bmp), &bmp);
    auto h_old_bitmap = static_cast<HBITMAP>(SelectObject(hdc_memory, h_bmp));
    SetStretchBltMode(dc, HALFTONE);
    RECT r;
    GetWindowRect(_hSelf, &r);
    StretchBlt(dc, 0, 0, r.right - r.left, r.bottom - r.top, hdc_memory, 0, 0,
               bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    SelectObject(hdc_memory, h_old_bitmap);
    DeleteDC(hdc_memory);
    DeleteObject(h_bmp);
    EndPaint(_hSelf, &ps);
    RECT rect;
    GetWindowRect(_hSelf, &rect);
    ValidateRect(_hSelf, &rect);
    return 0;
  }

  case WM_SHOWANDRECREATEMENU: {
    tagTPMPARAMS tpm_params;
    tpm_params.cbSize = sizeof(tagTPMPARAMS);
    GetWindowRect(_hSelf, &tpm_params.rcExclude);
    p.x = 0;
    p.y = 0;
    ClientToScreen(_hSelf, &p);
    m_state_menu = true;
    HMENU popup_menu = CreatePopupMenu();
    MenuItem::append_to_menu(popup_menu, m_context_menu_handler.get_suggestion_menu_items());
    SetForegroundWindow(m_npp.get_editor_handle ());
    m_menu_result =
        TrackPopupMenuEx(popup_menu, TPM_HORIZONTAL | TPM_RIGHTALIGN, p.x,
                         p.y, _hSelf, &tpm_params);
    PostMessage(m_npp.get_editor_handle (), WM_NULL, 0, 0);
    SetFocus(m_npp.get_scintilla_hwnd(m_npp.active_view()));
    m_state_menu = false;
    DestroyMenu(popup_menu);
    return 0;
  }

  case WM_COMMAND:
    if (HIWORD(w_param) == 0)
      m_context_menu_handler.process_menu_result(w_param);

    return 0;

  case WM_MOUSELEAVE:
    m_state_hovered = false;
    m_state_pressed = false;
    RedrawWindow(_hSelf, nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE);
    return 0;
  }
  return 0;
}
