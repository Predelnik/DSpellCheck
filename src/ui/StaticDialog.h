// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid
// misunderstandings, we consider an application to constitute a
// "derivative work" for the purpose of this license if it does any of the
// following:
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef STATIC_DIALOG_H
#define STATIC_DIALOG_H

#include "Window.h"
#include <memory>
#include <vector>

#ifndef NOTEPAD_PLUS_MSGS_H
#include "Notepad_plus_msgs.h"
#endif //NOTEPAD_PLUS_MSGS_H

typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

namespace WinApi
{
  class WinBase;
}

enum PosAlign{ALIGNPOS_LEFT, ALIGNPOS_RIGHT, ALIGNPOS_TOP, ALIGNPOS_BOTTOM};

struct DLGTEMPLATEEX {
  WORD   dlgVer;
  WORD   signature;
  DWORD  helpID;
  DWORD  exStyle;
  DWORD  style;
  WORD   cDlgItems;
  short  x;
  short  y;
  short  cx;
  short  cy;
  // The structure has more fields but are variable length
} ;

class StaticDialog : public Window
{
public :

  StaticDialog() : Window() { _rc = {}; };
  ~StaticDialog(){
    if (isCreated()) {
      ::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (LONG_PTR)NULL);	//Prevent run_dlgProc from doing anything, since its virtual
      destroy();
    }
  };
  virtual void create(int dialogID, bool isRTL = false, bool msgDestParent = true);

  virtual bool isCreated() const {
    return (_hSelf != NULL);
  };

  void goToCenter();

  void display(bool toShow = true, bool activate = true) const;
  template <typename ControlType>
  std::shared_ptr<ControlType> get_control(int id)
  {
    static_assert (std::is_base_of<WinApi::WinBase, ControlType>::value, "ControlType should inherit from WinApi::WinBase");
    auto btn = std::make_shared<ControlType>();
    btn->init(GetDlgItem(_hSelf, id));
    m_controls.emplace_back(weaken (btn));
    return btn;
  }

  POINT getLeftTopPoint(HWND hwnd/*, POINT & p*/) const {
    RECT rc;
    ::GetWindowRect(hwnd, &rc);
    POINT p;
    p.x = rc.left;
    p.y = rc.top;
    ::ScreenToClient(_hSelf, &p);
    return p;
  };

  bool isCheckedOrNot(int checkControlID) const {
    return (BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, checkControlID), BM_GETCHECK, 0, 0));
  };

  void destroy() {
    ::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_hSelf);
    ::DestroyWindow(_hSelf);
  };

protected :
  RECT _rc;
  std::vector<std::weak_ptr<WinApi::WinBase>> m_controls;
  static INT_PTR WINAPI dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  virtual INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

  void alignWith(HWND handle, HWND handle2Align, PosAlign pos, POINT & point);
  HGLOBAL makeRTLResource(int dialogID, DLGTEMPLATE **ppMyDlgTemplate);
};

#endif //STATIC_DIALOG_H
