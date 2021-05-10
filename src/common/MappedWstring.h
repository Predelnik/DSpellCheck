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

template <typename IndexType>
class MappedWstringGeneric {
public:
  IndexType to_original_index(IndexType cur_index) const { return !mapping.empty() ? mapping[cur_index] : cur_index; }

  IndexType from_original_index(IndexType cur_index) const {
    return !mapping.empty() ? static_cast<IndexType>(std::lower_bound(mapping.begin(), mapping.end(), cur_index) - mapping.begin()) : cur_index;
  }

  IndexType original_length() const { return !mapping.empty() ? mapping.back() : static_cast<IndexType>(str.size()); }

  void append(const MappedWstringGeneric<IndexType> &other) {
    if (!str.empty() && !other.str.empty())
      str.push_back(L'\n');
    str.insert(str.end(), other.str.begin(), other.str.end());
    mapping.insert(mapping.end(), other.mapping.begin(), other.mapping.end());
  }

public:
  std::wstring str;
  std::vector<IndexType> mapping; // should have size str.length () or empty (if empty mapping is identity a<->a)
  // indices should correspond to offsets string `str` had in original encoding
};
