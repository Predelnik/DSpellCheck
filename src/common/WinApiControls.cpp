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

#include "WinApiControls.h"

#include "winapi.h"

#include <cassert>
#include <chrono>

namespace WinApi {
WinBase::~WinBase() = default;

void WinBase::set_enabled(bool enabled) { EnableWindow(m_hwnd, enabled ? TRUE : FALSE); }

bool WinBase::is_enabled() const {
  return IsWindowEnabled(m_hwnd) != FALSE;
}

void WinBase::init(HWND hwnd) {
  m_hwnd = hwnd;
  assert(m_hwnd != nullptr);
  m_id = GetDlgCtrlID(m_hwnd);
  assert(m_id != -1);
  check_hwnd();
  init_impl();
}

DlgProcResult WinBase::dlg_proc([[maybe_unused]] UINT message, [[maybe_unused]] WPARAM w_param, [[maybe_unused]] LPARAM l_param) {
  return DlgProcResult::pass_through;
}

WinBase::operator bool() const {
  return m_hwnd;
}

bool WinBase::was_inited() const {
  return m_hwnd != nullptr;
}

void ComboBox::check_hwnd() {
  assert(get_class_name(m_hwnd) == L"ComboBox");
}

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
  for (int i = 0; i < count(); ++i)
    if (ComboBox_GetItemData(m_hwnd, i) == data)
      return i;
  return std::nullopt;
}

void ComboBox::clear() {
  ComboBox_ResetContent(m_hwnd);
}

int ComboBox::count() const {
  return ComboBox_GetCount(m_hwnd);
}

void Button::check_hwnd() {
  assert(get_class_name(m_hwnd) == L"Button");
}

DlgProcResult Button::dlg_proc(UINT message, WPARAM w_param, [[maybe_unused]] LPARAM l_param) {
  if (message == WM_COMMAND && LOWORD(w_param) == m_id && HIWORD(w_param) == BN_CLICKED) {
    button_pressed();
    return DlgProcResult::processed;
  }
  return DlgProcResult::pass_through;
}

namespace {
std::map<UINT_PTR, Timer *> global_timer_map;

void WINAPI timer_proc([[maybe_unused]] HWND hwnd, [[maybe_unused]] UINT msg, UINT_PTR id, [[maybe_unused]] DWORD time) {
  auto it = global_timer_map.find(id);
  if (it == global_timer_map.end()) {
    assert(!"Timer not registered. Unexpected state");
    return;
  }
  it->second->on_timer_tick();
}
}

Timer::Timer(HWND hwnd)
  : m_hwnd(hwnd) {
  initialize();
}

Timer::~Timer() {
  kill_timer();
  unregister();
}

void Timer::do_register() {
  global_timer_map.insert({m_id, this});
}

void Timer::unregister() {
  global_timer_map.erase(m_id);
}

void Timer::kill_timer() {
  KillTimer(0, m_id);
}

void Timer::generate_id() {
  m_id = SetTimer(0, 0, USER_TIMER_MAXIMUM, timer_proc);
}

void Timer::initialize() {
  generate_id();
  do_register();
}

void Timer::set_resolution(std::chrono::milliseconds resolution) {
  using namespace std::chrono;
  SetTimer(m_hwnd, m_id, static_cast<UINT>(resolution.count()), timer_proc);
  current_resolution = resolution;
}

void Timer::stop_timer() {
  SetTimer(m_hwnd, m_id, USER_TIMER_MAXIMUM, timer_proc);
  current_resolution = {};
}

bool Timer::is_set() const {
  return current_resolution.has_value();
}

}
