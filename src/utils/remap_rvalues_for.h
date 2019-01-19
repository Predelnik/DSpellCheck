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

#include <utility>

// useful for chaining functions. Just write original chaining function ref
// qualified Then write REMAP_RVALUES_FOR (function name) it will add
// appropriate overload for ref-ref-qualified case. Preserving reference
// category of return type.
#define REMAP_RVALUES_FOR(MEMBER_FUNC_NAME)                                    \
  template <typename... ArgTypes>                                              \
  auto MEMBER_FUNC_NAME(ArgTypes &&... args) && {                              \
    return std::move(                                                          \
        static_cast<std::remove_pointer_t<decltype(this)> &>(*this)            \
            .MEMBER_FUNC_NAME(std::forward<ArgTypes>(args)...));               \
  }
