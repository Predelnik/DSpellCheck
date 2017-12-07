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

#include "SuggestionsButton.h"

#include "MainDef.h"
#include "Plugin.h"
#include "resource.h"
#include "SpellChecker.h"
#include "npp/NppInterface.h"

#define MOUSELEAVE 0x0001
#define MOUSEHOVER 0x0002

void SuggestionsButton::do_dialog()
{
    HWND temp = GetFocus();
    create(IDD_SUGGESTIONS);
    SetFocus(temp);
}

bool reg_msg(HWND h_wnd, DWORD dw_msg_type)
{
    TRACKMOUSEEVENT tme_event_track; // tracking information

    tme_event_track.cbSize = sizeof(TRACKMOUSEEVENT);
    tme_event_track.dwFlags = 0;

    if (dw_msg_type & MOUSELEAVE)
        tme_event_track.dwFlags |= TME_LEAVE;
    if (dw_msg_type & MOUSEHOVER)
        tme_event_track.dwFlags |= TME_HOVER;

    tme_event_track.hwndTrack = h_wnd;
    tme_event_track.dwHoverTime = HOVER_DEFAULT;
    //      tmeEventTrack.dwHoverTime = 10;

    if (!TrackMouseEvent(&tme_event_track))
    {
        wchar_t sz_error[64];

        swprintf(sz_error, L"Can't TrackMouseEvent ! ErrorId: %d", GetLastError());
        MessageBox(h_wnd, sz_error, L"Error", MB_OK | MB_ICONSTOP);

        return false;
    }

    return true;
}

HMENU SuggestionsButton::get_popup_menu() { return m_popup_menu; }

int SuggestionsButton::get_result() { return m_menu_result; }

SuggestionsButton::SuggestionsButton(HINSTANCE h_inst, HWND parent, NppInterface& npp): m_menu_result(0),
                                                                                        m_popup_menu(nullptr),
                                                                                        m_npp(npp)
{
    Window::init(h_inst, parent);
    m_state_pressed = false;
    m_state_hovered = false;
    m_state_menu = false;
}


INT_PTR SuggestionsButton::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param)
{
    POINT p;
    BITMAP bmp;
    int id_img;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_INITDIALOG:

        display(false);
        m_popup_menu = CreatePopupMenu();

        return false;

    case WM_MOUSEMOVE:
        m_state_hovered = true;
        reg_msg(_hSelf, MOUSELEAVE);
        RedrawWindow(_hSelf, nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE);
        return false;

    case WM_MOUSEHOVER:
        return false;

    case WM_LBUTTONDOWN:
        m_state_pressed = true;
        RedrawWindow(_hSelf, nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE);
        return false;

    case WM_LBUTTONUP:
        if (!m_state_pressed)
            return false;
        get_spell_checker()->show_suggestion_menu();
        return false;

    case WM_MOUSEWHEEL:
        ShowWindow(_hSelf, SW_HIDE);
        PostMessage(m_npp.get_scintilla_hwnd(m_npp.active_view()), message, w_param, l_param);
        return false;

    case WM_PAINT:
        {
            if (!isVisible())
                return false;

            id_img = IDR_DOWNARROW;
            if (m_state_pressed || m_state_menu)
                id_img = IDR_DOWNARROW_PUSH;
            else if (m_state_hovered)
                id_img = IDR_DOWNARROW_HOVER;

            HDC dc = BeginPaint(_hSelf, &ps);
            HDC hdc_memory = ::CreateCompatibleDC(dc);
            HBITMAP h_bmp = LoadBitmap(_hInst, MAKEINTRESOURCE(id_img));
            GetObject(h_bmp, sizeof(bmp), &bmp);
            HBITMAP h_old_bitmap = (HBITMAP)SelectObject(hdc_memory, h_bmp);
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
            return false;
        }

    case WM_SHOWANDRECREATEMENU:
        {
            tagTPMPARAMS tpm_params;
            tpm_params.cbSize = sizeof(tagTPMPARAMS);
            GetWindowRect(_hSelf, &tpm_params.rcExclude);
            p.x = 0;
            p.y = 0;
            ClientToScreen(_hSelf, &p);
            m_state_menu = true;
            SetForegroundWindow(m_npp_data_instance.npp_handle);
            m_menu_result = TrackPopupMenuEx(m_popup_menu, TPM_HORIZONTAL | TPM_RIGHTALIGN,
                                             p.x, p.y, _hSelf, &tpm_params);
            PostMessage(m_npp_data_instance.npp_handle, WM_NULL, 0, 0);
            SetFocus(m_npp.get_scintilla_hwnd(m_npp.active_view()));
            m_state_menu = false;
            DestroyMenu(m_popup_menu);
            m_popup_menu = CreatePopupMenu();
            return false;
        }

    case WM_COMMAND:
        if (HIWORD(w_param) == 0)
            get_spell_checker()->process_menu_result(w_param);

        return false;

    case WM_MOUSELEAVE:
        m_state_hovered = false;
        m_state_pressed = false;
        RedrawWindow(_hSelf, nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE);
        return false;
    }
    return false;
}
