#pragma once
#include "enum_range.h"

namespace WinApi {
class WinBase {
public:
  virtual ~WinBase();
  void set_enabled(bool enabled);
  void init(HWND hwnd);

private:
  virtual void check_hwnd() = 0;
  virtual void init_impl() {}

protected:
  HWND m_hwnd = nullptr;
};

class ComboBox : public WinBase {
  void check_hwnd() override;

public:
  template <typename EnumType> void add_item(EnumType val) {
    add_item(gui_string(val).c_str(), static_cast<int>(val));
  }

  template <typename EnumType> void add_items() {
    static_assert(std::is_enum_v<EnumType>);
    for (auto val : enum_range<EnumType>()) {
      add_item(gui_string(val).c_str(), static_cast<int>(val));
    }
  }

  void add_item(const wchar_t *text, int data) {
    ComboBox_AddString(m_hwnd, text);
    ComboBox_SetItemData(m_hwnd, ComboBox_GetCount(m_hwnd) - 1, data);
  }

  int current_index() const { return static_cast<int>(ComboBox_GetCurSel(m_hwnd)); }

  int current_data() const { return static_cast<int>(ComboBox_GetItemData(m_hwnd, current_index())); }

  void set_current_index(int index) { ComboBox_SetCurSel(m_hwnd, index); }

  std::optional<int> find_by_data(int data);
  void clear();
};

template <typename EnumType> class EnumComboBox : protected ComboBox {
  using Parent = ComboBox;

public:
  using Parent::init;
  using Parent::set_enabled;
  using Parent::clear;
  using Parent::add_item;

  EnumType current_data() const { return static_cast<EnumType>(Parent::current_data()); }

  void set_index(EnumType value) { set_current_index(*find_by_data(static_cast<std::underlying_type_t<EnumType>>(value))); }

private:
  void init_impl() override { this->add_items<EnumType>(); }
};
} // namespace WinApi
