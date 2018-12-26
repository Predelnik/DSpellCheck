#include "WinApiControls.h"
#include <cassert>
#include "winapi.h"

namespace WinApi
{
WinBase::~WinBase() = default;

void WinBase::set_enabled(bool enabled) { EnableWindow(m_hwnd, enabled ? TRUE : FALSE); }

bool WinBase::is_enabled() const {
  return IsWindowEnabled (m_hwnd) != FALSE;
}

void WinBase::init(HWND hwnd) {
  m_hwnd = hwnd;
  check_hwnd();
  init_impl();
}

WinBase::operator bool() const {
  return m_hwnd;
}

void ComboBox::check_hwnd() { assert(get_class_name(m_hwnd) == L"ComboBox"); }

int ComboBox::current_index() const { return static_cast<int>(ComboBox_GetCurSel(m_hwnd)); }

std::wstring ComboBox::current_text() const {
  auto length = ComboBox_GetTextLength(m_hwnd);
  std::vector<wchar_t> buf(length + 1);
  ComboBox_GetText(m_hwnd, buf.data(), static_cast<int>(buf.size()));
  return buf.data();
}

int ComboBox::current_data() const { return static_cast<int>(ComboBox_GetItemData(m_hwnd, current_index())); }

void ComboBox::set_current_index(int index) {
  ComboBox_SetCurSel(m_hwnd, index);
}

std::optional<int> ComboBox::find_by_data(int data) {
  for (int i = 0; i < count (); ++i)
    if (ComboBox_GetItemData(m_hwnd, i) == data)
      return i;
  return std::nullopt;
}

void ComboBox::clear() { ComboBox_ResetContent(m_hwnd); }

int ComboBox::count() const {
  return ComboBox_GetCount(m_hwnd);
}
}
