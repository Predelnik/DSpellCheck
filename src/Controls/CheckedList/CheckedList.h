//////////////////////////////////////////////////////////////////////////////
///
/// @file checkedList.h
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

#ifndef CHECKEDLIST_H
#define CHECKEDLIST_H

/****************************************************************************/
/// @name Checked listbox specific messages.
/// @{

#define LBCM_FLATCHECKS  WM_USER + 1 ///<Set flat style check boxes
#define LBCM_CHECKALL  WM_USER + 2   ///<Enable right mouse button select/deselect all

/// @}

/****************************************************************************/
/// @name Checked listbox specific notifications.
/// @{

#define LBCN_ITEMCHECK  6 ///<Notifies that an item's check changed

/// @}

/****************************************************************************/
/// @name Macroes
/// @{

/// @def CheckedListBox_SetCheckState(hList, iIndex, fCheck)
///
/// @brief Checks or unchecks an item in a checked listbox control.
///
/// @param hwndCtl The handle of a checked listbox.
/// @param iIndex The zero-based index of the item for which to set the check state.
/// @param fCheck A value that is set to TRUE to select the item, or FALSE to deselect it.
///
/// @returns The zero-based index of the item in the listbox. If an error occurs,
///           the return value is LB_ERR. 
#define CheckedListBox_SetCheckState(hwndCtl, iIndex, fCheck) \
    (0 > ListBox_SetItemData((hwndCtl), (iIndex), (fCheck)) ? CB_ERR : \
     (InvalidateRgn((hwndCtl), NULL, FALSE), (iIndex)))

/// @def CheckedListBox_GetCheckState(hList, iIndex, fCheck)
///
/// @brief Gets the checked state of an item in a checked listbox control.
///
/// @param hwndCtl The handle of a checked listbox.
/// @param iIndex The zero-based index of the item for which to set the check state.
///
/// @returns Nonzero if the given item is selected, or zero otherwise. 
#define CheckedListBox_GetCheckState(hwndCtl, iIndex) \
    (BOOL) ListBox_GetItemData(hwndCtl, iIndex)

/// @def CheckedListBox_SetFlatStyleChecks(hwndCtl, fFlat)
///
/// @brief Sets the appearance of the checkboxes.
///
/// @param hwndCtl The handle of a checked listbox.
/// @param fFlat TRUE for flat checkboxes, or FALSE for standard checkboxes.
///
/// @returns No return value.
#define CheckedListBox_SetFlatStyleChecks(hwndCtl, fFlat) \
     ((void)SendMessage((hwndCtl),LBCM_FLATCHECKS,(WPARAM)(BOOL) (fFlat),(LPARAM)0L))

/// @def CheckedListBox_EnableCheckAll(hwndCtl, fEnable)
///
/// @brief Sets the select/deselect all feature.
///
/// @param hwndCtl The handle of a checked listbox.
/// @param fEnable TRUE enables right mouse button select/deselect all feature, or FALSE disables feature.
///
/// @returns No return value.
#define CheckedListBox_EnableCheckAll(hwndCtl, fEnable) \
    ((void)SendMessage((hwndCtl),LBCM_CHECKALL,(WPARAM)(BOOL) (fEnable),(LPARAM)0L))

/// @}

/****************************************************************************/
// Exported function prototypes

ATOM InitCheckedListBox(HINSTANCE hInstance);
HWND New_CheckedListBox(HWND hParent, DWORD dwStyle, DWORD dwExStyle, DWORD dwID, INT x, INT y, INT nWidth, INT nHeight);

#endif //CHECKEDLIST_H
