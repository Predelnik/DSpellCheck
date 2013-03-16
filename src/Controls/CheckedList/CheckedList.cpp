//////////////////////////////////////////////////////////////////////////////
///
/// @file checkedList.c
///
/// @brief A checked listBox control in Win32 SDK C.
///
/// @author David MacDermot
///
/// @par Comments:
///         This source is distributed in the hope that it will be useful,
///         but WITHOUT ANY WARRANTY; without even the implied warranty of
///         MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
///
/// @date 9-07-10
///
/// @todo
///
/// @bug
///
//////////////////////////////////////////////////////////////////////////////

// #define UNICODE
// #define _UNICODE

#define WIN32_LEAN_AND_MEAN

#include <tchar.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "CheckedList.h"

#define WPROC _T("Wprc") ///<Generic property tag
#define ID_LISTBOX 2000  ///<An Id for the ListBox
#define FLATCHECKS 0x01  ///<Draw flat checks flag
#define CHECKALL 0x02    ///<Enable RMB check/uncheck all

/// @name Macroes
/// @{
/// @def NELEMS(a)
///
/// @brief Computes number of elements of an array.
///
/// @param a An array.
#define NELEMS(a) (sizeof(a) / sizeof((a)[0]))

/// @def ListBox_ItemFromPoint(hwndCtl, xPos, yPos)
///
/// @brief Gets the zero-based index of the item nearest the specified point
///         in a list box.
///
/// @param hwndCtl The handle of a listbox.
/// @param xPos The x coordinate of a point.
/// @param yPos The y coordinate of a point.
#define ListBox_ItemFromPoint(hwndCtl, xPos, yPos) \
  (DWORD)SendMessage((hwndCtl),LB_ITEMFROMPOINT, \
  (WPARAM)0,MAKELPARAM((UINT)(xPos),(UINT)(yPos)))

/// @}

LPCTSTR g_szClassName = _T("CheckedListBox");   ///< The classname.

/****************************************************************************/
//Functions
static LRESULT CALLBACK ListBox_Proc(HWND, UINT, WPARAM, LPARAM);
static LRESULT CALLBACK Control_Proc(HWND, UINT, WPARAM, LPARAM);

#pragma region ListBox event handlers

/// @brief Handles WM_CHAR message sent to the listbox.
///
/// @param hwnd  Handle of the listbox.
/// @param ch The character.
/// @param cRepeat The number of times the keystroke is repeated
///         as a result of the user holding down the key.
///
/// @returns VOID.
VOID ListBox_OnChar(HWND hwnd, TCHAR ch, INT cRepeat)
{
  if (VK_SPACE == ch)
  {
    HWND hParent;

    hParent = GetParent(hwnd);
    if (NULL != hParent)
    {
      // Get the current selection
      INT nIndex = ListBox_GetCurSel(hwnd);
      if (LB_ERR != nIndex)
      {
        RECT rcItem;
        ListBox_GetItemRect(hwnd, nIndex, &rcItem);
        InvalidateRect(hwnd, &rcItem, FALSE);

        // Invert the check mark
        CheckedListBox_SetCheckState(hwnd, nIndex,
          !CheckedListBox_GetCheckState(hwnd, nIndex));
      }
    }
  }
  // Do the default handling now
  FORWARD_WM_CHAR(hwnd, ch, cRepeat, DefWindowProc);
}

/// @brief Handles WM_LBUTTONDOWN message in the listbox.
///
/// @param hwnd  Handle of listbox.
/// @param fDoubleClick TRUE if this is a double click event.
/// @param x The xpos of the mouse.
/// @param y The ypos of the mouse.
/// @param keyFlags Set if certain keys down at time of click.
///
/// @returns VOID.
VOID ListBox_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags)
{
  HWND hParent;

  hParent = GetParent(hwnd);
  if (NULL != hParent)
  {
    DWORD dwRtn = ListBox_ItemFromPoint(hwnd, x, y);
    INT nIndex = LOWORD(dwRtn);
    BOOL fNonClient = HIWORD(dwRtn);

    if (!fNonClient)
    {
      // Invalidate this window
      RECT rcItem;
      ListBox_GetItemRect(hwnd, nIndex, &rcItem);
      InvalidateRect(hwnd, &rcItem, FALSE);

      // if ((nIndex == ListBox_GetCurSel(hwnd)) || fDoubleClick)
      CheckedListBox_SetCheckState(hwnd, nIndex,
        !CheckedListBox_GetCheckState(hwnd, nIndex));
    }
  }

  // Do the default inside of box handling now (such as hilite item and toggle
  //  check while keeping keyboard focus when clicked inside)
  CallWindowProc((WNDPROC)GetProp(hwnd, WPROC),
    hwnd, WM_LBUTTONDOWN, (WPARAM) (UINT)keyFlags, MAKELPARAM(x, y));
}

/// @brief Handles WM_RBUTTONDOWN message in the listbox.
///
/// @param hwnd  Handle of listbox.
/// @param fDoubleClick TRUE if this is a double click event.
/// @param x The xpos of the mouse.
/// @param y The ypos of the mouse.
/// @param keyFlags Set if certain keys down at time of click.
///
/// @returns VOID.
VOID ListBox_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags)
{
  HWND hParent;

  if (CHECKALL & (DWORD)GetWindowLongPtr(hwnd, GWLP_USERDATA))
  {
    hParent = GetParent(hwnd);
    if (NULL != hParent)
    {
      INT nCount = ListBox_GetCount(hwnd);
      INT nCheckCount = 0;

      for (INT i = 0; i < nCount; i++)
      {
        if (CheckedListBox_GetCheckState(hwnd, i))
          nCheckCount++;
      }
      BOOL bCheck = nCheckCount != nCount;
      for (INT i = 0; i < nCount; i++)
        CheckedListBox_SetCheckState(hwnd, i, bCheck);

      // Make sure to invalidate this window as well
      InvalidateRgn(hwnd, NULL, FALSE);
    }
    // Do the default handling now
    FORWARD_WM_RBUTTONDOWN(hwnd, fDoubleClick,
      x, y, keyFlags, DefWindowProc);
  }
}

#pragma endregion ListBox event handlers

#pragma region Drawing

/// @brief Handles WM_DRAWITEM message sent to the control when a visual aspect of
///         the owner-drawn listbox has changed.
///
/// @param hwnd  Handle of control.
/// @param lpDrawItem The structure that contains information about the item
///                    to be drawn and the type of drawing required.
///
/// @returns VOID.
VOID Control_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
{
  if (lpDrawItem->itemAction & (ODA_DRAWENTIRE | ODA_SELECT))
  {
    COLORREF crBk;
    COLORREF crTx;
    HDC dc = lpDrawItem->hDC;
    RECT rcBitmap = lpDrawItem->rcItem;
    RECT rcText = lpDrawItem->rcItem;

    INT iLen = ListBox_GetTextLen(lpDrawItem->hwndItem, lpDrawItem->itemID);
    TCHAR *buf = (TCHAR *) malloc ((iLen + 1) * sizeof (TCHAR));
    memset (buf, 0, (iLen + 1) * sizeof (TCHAR));
    ListBox_GetText(lpDrawItem->hwndItem, lpDrawItem->itemID, buf);

    TEXTMETRIC metrics;
    GetTextMetrics(dc, &metrics);

    // Checkboxes for larger sized fonts seem a bit crowded so adjust
    //  accordingly.
    INT iFactor = 24 < metrics.tmHeight ? -2 : -1;
    InflateRect(&rcBitmap, iFactor, iFactor);
    rcBitmap.right = rcBitmap.left + (rcBitmap.bottom - rcBitmap.top);

    rcText.left = rcBitmap.right + metrics.tmAveCharWidth;

    UINT nState = DFCS_BUTTONCHECK;

    if (lpDrawItem->itemData)
      nState |= DFCS_CHECKED;

    // Draw the checkmark using DrawFrameControl
    DrawFrameControl(dc, &rcBitmap, DFC_BUTTON, nState);

    if (FLATCHECKS & (DWORD)GetWindowLongPtr(lpDrawItem->hwndItem, GWLP_USERDATA))
    {
      //Make border thin
      FrameRect(lpDrawItem->hDC, &rcBitmap, GetSysColorBrush(COLOR_BTNSHADOW));
      // Extra pass for larger font sized checkboxes
      for(int i = 24 < metrics.tmHeight ? 3 : 2; 0 < i; i--)
      {
        InflateRect(&rcBitmap, -1, -1);
        FrameRect(lpDrawItem->hDC, &rcBitmap, GetSysColorBrush(COLOR_WINDOW));
      }
    }
    if (lpDrawItem->itemState & (ODS_SELECTED))
    {
      crBk = GetBkColor(dc);
      crTx = GetTextColor(dc);

      SetBkColor(dc, GetSysColor(COLOR_HIGHLIGHT));
      SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }

    // Erase and draw
    ExtTextOut(dc, 0, 0, ETO_OPAQUE, &rcText, 0, 0, 0);

    DrawText(dc, buf, _tcslen(buf), &rcText, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    if ((lpDrawItem->itemState & (ODS_FOCUS | ODS_SELECTED)) == (ODS_FOCUS | ODS_SELECTED))
      DrawFocusRect(dc, &rcText);

    // Ensure Defaults
    if (lpDrawItem->itemState & (ODS_SELECTED))
    {
      SetBkColor(dc, crBk);
      SetTextColor(dc, crTx);
    }
    free (buf);
  }
}

/// @brief Handles WM_MEASUREITEM message sent to the control when the owner-drawn
///         listbox is created.
///
/// @param hwnd  Handle of control.
/// @param lpMeasureItem The structure that contains the dimensions of the
///                       owner-drawn listbox.
///
/// @returns VOID.
VOID Control_OnMeasureItem(HWND hwnd, LPMEASUREITEMSTRUCT lpMeasureItem)
{
  // Get the HDC of the dialog or parent window of the custom control
  HWND hParent = GetParent(hwnd);
  HDC hDC = GetDC(hParent);

  // For common device contexts, GetDC assigns default attributes to the device
  //  context each time it is retrieved.  I guess the parent dialog's DC is common
  //  because I do not get the metrics for the dialog's font.  So I'll have to
  //  explicitly select the font into the device context to get it's metrics.
  HFONT hFont = (HFONT) SelectObject(hDC,(HFONT)SendMessage(hParent, WM_GETFONT, 0, 0L));
  TEXTMETRIC metrics;
  GetTextMetrics(hDC, &metrics);

  // Now that I have the metrics clean up (but don't delete the Parent's HFONT)
  SelectObject(hDC, hFont);
  ReleaseDC(hwnd, hDC);

  LONG lHeight = metrics.tmHeight + metrics.tmExternalLeading;
  if(lpMeasureItem->itemHeight < (UINT) lHeight)
    lpMeasureItem->itemHeight = lHeight;
}

#pragma endregion Drawing

#pragma region create destroy

/// @brief Initialize and register the checked listbox class.
///
/// @param hInstance Handle of application instance.
///
/// @returns ATOM If the function succeeds, the atom that uniquely identifies
///                the class being registered, otherwise 0.
ATOM InitCheckedListBox(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;

  // Get standard listbox information
  wcex.cbSize = sizeof(wcex);
  if (!GetClassInfoEx(NULL, WC_LISTBOX, &wcex))
    return 0;

  // Add our own stuff
  wcex.lpfnWndProc = (WNDPROC)Control_Proc;
  wcex.hInstance = hInstance;
  wcex.lpszClassName = g_szClassName;

  // Register our new class
  return RegisterClassEx(&wcex);
}

/// @brief Create an new instance of the checked listbox.
///
/// @param hParent Handle of the control's parent.
/// @param dwStyle The window style.
/// @param dwExStyle The extended window style.
/// @param dwID The ID for this control.
/// @param x The horizontal position of control.
/// @param y The vertical position of control.
/// @param nWidth The control width.
/// @param nHeight The control height.
///
/// @par Note:
///       The following Style parameters will be ignored by this control:
///         CBS_SIMPLE, CBS_DROPDOWN, and CBS_OWNERDRAWVARIABLE.
///
/// @returns HWND If the function succeeds, the checked listbox handle, otherwise NULL.
HWND New_CheckedListBox(HWND hParent, DWORD dwStyle, DWORD dwExStyle, DWORD dwID, INT x, INT y, INT nWidth, INT nHeight)
{
  static ATOM aCheckedListBox = 0;
  static HWND hCheckedListBox;

  HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(hParent, GWLP_HINSTANCE);

  //Only need to register the CheckedListBox once
  if (!aCheckedListBox)
    aCheckedListBox = InitCheckedListBox(hinst);

  hCheckedListBox = CreateWindowEx(dwExStyle, g_szClassName, NULL,
    dwStyle, x, y, nWidth, nHeight,
    hParent, (HMENU)dwID, hinst, NULL);

  return hCheckedListBox;
}

/// @brief Handles WM_CREATE message.
///
/// @param hwnd Handle of control.
/// @param lpCreateStruct Pointer to a structure with creation data.
///
/// @returns BOOL If an application processes this message,
///                it should return TRUE to continue creation of the window.
BOOL Control_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
  HWND hList;

  // Remove LBS_OWNERDRAWVARIABLE if defined and add the bits we need
  lpCreateStruct->style &= ~((DWORD)LBS_OWNERDRAWVARIABLE);

  // Use default strings. We need the itemdata to store checkmarks
  lpCreateStruct->style |= (LBS_OWNERDRAWFIXED | LBS_HASSTRINGS);

  hList = CreateWindowEx(lpCreateStruct->dwExStyle, WC_LISTBOX, NULL,
    lpCreateStruct->style, 0, 0,
    lpCreateStruct->cx, lpCreateStruct->cy,
    hwnd, (HMENU)ID_LISTBOX, lpCreateStruct->hInstance, NULL);

  if (!hList)
    return FALSE;

  SendMessage(hList, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);

  // Subclass listbox and save the old proc
  SetProp(hList, WPROC, (HANDLE)GetWindowLongPtr(hList, GWLP_WNDPROC));
  SubclassWindow(hList, ListBox_Proc);

  // Configure the parent window to be invisible,
  //  listbox child to determine appearance.
  SetWindowLongPtr(hwnd, GWL_STYLE, WS_CHILD |
    (WS_TABSTOP & GetWindowLongPtr(hwnd, GWL_STYLE) ? WS_TABSTOP : 0));

  SetWindowLongPtr(hwnd, GWL_EXSTYLE, 0l);

  // Certain window data is cached, so changes you make using SetWindowLongPtr
  //  will not take effect until you call the SetWindowPos() function.  SWP_FRAMECHANGED
  //  causes the window to recalculate the non client area and, in our case,
  //  remove scroll bar and border.
  SetWindowPos(hwnd, NULL, lpCreateStruct->x, lpCreateStruct->y,
    lpCreateStruct->cx, lpCreateStruct->cy,
    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

  return TRUE;
}

/// @brief Handles WM_DESTROY message.
///
/// @param hwnd Handle of Control.
///
/// @returns VOID.
VOID Control_OnDestroy(HWND hwnd)
{
  //Do something
}

#pragma endregion create destroy

#pragma region event handlers

/// @brief Handles WM_SIZE message.
///
/// @param hwnd  Handle of control.
/// @param state Specifies the type of resizing requested.
/// @param cx The width of client area.
/// @param cy The height of client area.
///
/// @returns VOID.
VOID Control_OnSize(HWND hwnd, UINT state, INT cx, INT cy)
{
  HWND hList = GetDlgItem(hwnd, ID_LISTBOX);

  //Attempt to size listBox component to fill parent container
  SetWindowPos(hList, NULL, 0, 0, cx, cy,
    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

  //Resize parent container to listBox new integral height
  RECT rc = { 0 };
  GetWindowRect(hList, &rc);

  SetWindowPos(hwnd, NULL, 0, 0, cx, rc.bottom - rc.top,
    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

#pragma endregion event handlers

/// @brief Window procedure for the listbox.
///
/// @param hList Handle of listbox.
/// @param msg Which message?
/// @param wParam Message parameter.
/// @param lParam Message parameter.
///
/// @returns LRESULT depends on message.
static LRESULT CALLBACK ListBox_Proc(HWND hList, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    HANDLE_MSG(hList, WM_CHAR, ListBox_OnChar);
    HANDLE_MSG(hList, WM_LBUTTONDOWN, ListBox_OnLButtonDown);
    HANDLE_MSG(hList, WM_LBUTTONDBLCLK, ListBox_OnLButtonDown);
    HANDLE_MSG(hList, WM_RBUTTONDOWN, ListBox_OnRButtonDown);

  case LB_SETITEMDATA:    // send notification upon completion of this message
    {
      DWORD dwItem = (DWORD)CallWindowProc((WNDPROC)GetProp(hList, WPROC),
        hList, msg, wParam, lParam);
      if (LB_ERR != dwItem)
        FORWARD_WM_COMMAND(GetParent(hList), GetDlgCtrlID(hList), hList,
        LBCN_ITEMCHECK, SNDMSG);

      return dwItem;
    }
  case WM_DESTROY:    //Unsubclass the ListBox
    {
      WNDPROC wp = (WNDPROC)GetProp(hList, WPROC);
      if (NULL != wp)
      {
        SetWindowLongPtr(hList, GWLP_WNDPROC, (DWORD)wp);
        RemoveProp(hList, WPROC);
        return CallWindowProc(wp, hList, msg, wParam, lParam);
      }
    }
  default:
    return CallWindowProc((WNDPROC)GetProp(hList, WPROC), hList,
      msg, wParam, lParam);
  }
}

/// @brief Procedure to route unhandled messages to custom control or designated child.
///
/// @param hwnd Handle of control.
/// @param hChild Handle of designated child.
/// @param msg Which message?
/// @param wParam Message parameter.
/// @param lParam Message parameter.
///
/// @returns LRESULT depends on message.
static LRESULT DefaultHandler(HWND hwnd, HWND hChild, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    // These messages to be handled by this window
  case WM_DRAWITEM:
  case WM_MEASUREITEM:
  case WM_CREATE:
  case WM_DESTROY:
  case WM_SIZE:

    // Sending these to child will cause stack overflow
  case WM_CTLCOLORMSGBOX:
  case WM_CTLCOLOREDIT:
  case WM_CTLCOLORLISTBOX:
  case WM_CTLCOLORBTN:
  case WM_CTLCOLORDLG:
  case WM_CTLCOLORSCROLLBAR:
  case WM_CTLCOLORSTATIC:
  case WM_MOUSEACTIVATE:

    // Sending these to child will cause improper sizing / positioning
  case WM_WINDOWPOSCHANGING:
  case WM_WINDOWPOSCHANGED:
  case WM_NCCALCSIZE:

    // Sending this to child will mess up child paint
  case WM_PAINT:
    break; //<- End Fallthrough

    // Pass child notifications to parent
  case WM_COMMAND:
    FORWARD_WM_COMMAND(GetParent(hwnd), GetDlgCtrlID(hwnd), hwnd,
      HIWORD(wParam), SNDMSG);
    return 0;
  case WM_NOTIFY:
    ((LPNMHDR)lParam)->hwndFrom = hwnd;
    ((LPNMHDR)lParam)->idFrom = GetDlgCtrlID(hwnd);
    return FORWARD_WM_NOTIFY(GetParent(hwnd), ((LPNMHDR)lParam)->idFrom,
      (LPNMHDR)lParam, SNDMSG);

  default: // The rest of the messages passed to child (if it exists)
    {
      if(NULL != hChild)
        return SNDMSG(hChild, msg, wParam, lParam);
    }
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

/// @brief Window procedure and public interface for the checked list control.
///
/// @param hwnd Handle of control.
/// @param msg Which message?
/// @param wParam Message parameter.
/// @param lParam Message parameter.
///
/// @returns LRESULT depends on message.
static LRESULT CALLBACK Control_Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    HANDLE_MSG(hwnd, WM_DRAWITEM, Control_OnDrawItem);
    HANDLE_MSG(hwnd, WM_MEASUREITEM, Control_OnMeasureItem);
    HANDLE_MSG(hwnd, WM_CREATE, Control_OnCreate);
    HANDLE_MSG(hwnd, WM_DESTROY, Control_OnDestroy);
    HANDLE_MSG(hwnd, WM_SIZE, Control_OnSize);
  case WM_CTLCOLORLISTBOX:
    return SNDMSG(GetParent(hwnd), msg, wParam, lParam);
  case LBCM_FLATCHECKS:
    {
      DWORD dwUserData = (DWORD)GetWindowLongPtr(GetDlgItem(hwnd,
        ID_LISTBOX), GWLP_USERDATA);
      if (FALSE != (BOOL)wParam)
        dwUserData |= FLATCHECKS;
      else
        dwUserData &= ~FLATCHECKS;

      return SetWindowLongPtr(GetDlgItem(hwnd, ID_LISTBOX), GWLP_USERDATA,
        (LONG_PTR)dwUserData);
    }
  case LBCM_CHECKALL:
    {
      DWORD dwUserData = (DWORD)GetWindowLongPtr(GetDlgItem(hwnd,
        ID_LISTBOX), GWLP_USERDATA);
      if (FALSE != (BOOL)wParam)
        dwUserData |= CHECKALL;
      else
        dwUserData &= ~CHECKALL;

      return SetWindowLongPtr(GetDlgItem(hwnd, ID_LISTBOX), GWLP_USERDATA,
        (LONG_PTR)dwUserData);
    }
  default:
    return DefaultHandler(hwnd, GetDlgItem(hwnd, ID_LISTBOX), msg, wParam, lParam);
  }
}
