#include "WinApiControls.h"
#include <cassert>
#include "winapi.h"

namespace WinApi
{
WinBase::~WinBase() = default;

void WinBase::set_enabled(bool enabled) { EnableWindow(m_hwnd, enabled ? TRUE : FALSE); }

void WinBase::init(HWND hwnd) {
  m_hwnd = hwnd;
  check_hwnd();
  init_impl();
}

void ComboBox::check_hwnd() { assert(get_class_name(m_hwnd) == L"ComboBox"); }

std::optional<int> ComboBox::find_by_data(int data) {
  for (int i = 0; i < ComboBox_GetCount(m_hwnd); ++i)
    if (ComboBox_GetItemData(m_hwnd, i) == data)
      return i;
  return std::nullopt;
}

void ComboBox::clear() { ComboBox_ResetContent(m_hwnd); }
}
