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

// helper class which allows for arbitrary type T to have semantics similar to
// unique_ptr<> i.e. move_only<T> is move only, obviously a = std::move (b),
// causes b to have value of default constructed T{} useful with bool flags and
// possibly pointers/handles In case of pointers/handles unique_ptr with custom
// deleter could be used instead though.

#include <utility>

template <typename T> class move_only {
  using self_t = move_only;

public:
  move_only() = default;

  move_only(const T &arg)
    : value(arg) {
  }

  move_only(T &&arg)
    : value(std::move(arg)) {
  }

  move_only(const self_t &) = delete;
  move_only &operator=(const self_t &) = delete;

  move_only(self_t &&other) noexcept
    : value(std::move(other.value)) {
    other.value = T{};
  }

  move_only &operator=(self_t &&other) noexcept {
    value = std::move(other.value);
    other.value = T{};
    return *this;
  }

  ~move_only() = default;
  const T &get() const { return value; }
  T &get() { return value; }

private:
  T value = {};
};
