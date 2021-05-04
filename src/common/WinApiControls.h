// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
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

#pragma once
#include "enum_range.h"
#include "lsignal.h"

enum class DlgProcResult {
  processed,
  pass_through
};

namespace WinApi {
class WinBase {
public:
  virtual ~WinBase();
  void set_enabled(bool enabled);
  bool is_enabled() const;
  void init(HWND hwnd);
  virtual DlgProcResult dlg_proc(UINT message, WPARAM w_param, LPARAM l_param); // should be called for possibility of making signals in windows work
  explicit operator bool () const;
  bool was_inited () const;

private:
  virtual void check_hwnd() = 0;
  virtual void init_impl() {}

protected:
  HWND m_hwnd = nullptr;
  int m_id = -1;
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

  void add_item(const wchar_t *text, int data = -1) {
    ComboBox_AddString(m_hwnd, text);
    ComboBox_SetItemData(m_hwnd, ComboBox_GetCount(m_hwnd) - 1, data);
  }

  int current_index() const;
  std::wstring current_text () const;
  int current_data() const;
  void set_current_index(int index);

  std::optional<int> find_by_data(int data);
  void clear();
  int count() const;
};

class Button : public WinBase {
  void check_hwnd() override;
public:
  DlgProcResult dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) override;
  lsignal::signal<void()> button_pressed;
};

template <typename EnumType> class EnumComboBox : protected ComboBox {
  using Parent = ComboBox;

public:
  using Parent::init;
  using Parent::set_enabled;
  using Parent::clear;
  using Parent::add_item;

  EnumType current_data() const { return static_cast<EnumType>(Parent::current_data()); }

  void set_current(EnumType value) {
    if (!was_inited ())
      return;
    set_current_index(*find_by_data(static_cast<std::underlying_type_t<EnumType>>(value)));
  }

private:
  void init_impl() override { this->add_items<EnumType>(); }
};

class Timer
{
public:
  Timer(HWND hwnd);
  ~Timer ();
  void set_resolution (std::chrono::milliseconds resolution);
  void stop_timer ();
  bool is_set () const;

  lsignal::signal<void()> on_timer_tick;

private:
  void do_register();
  void unregister();
  void kill_timer();
  void generate_id();
  void initialize ();

private:
  HWND m_hwnd = nullptr;
  UINT_PTR m_id = 0;
  std::optional<std::chrono::milliseconds> current_resolution;
};
} // namespace WinApi
