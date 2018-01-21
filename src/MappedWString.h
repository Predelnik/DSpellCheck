// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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

class MappedWstring {
public:
    long to_original_index(long cur_index) const { return !mapping.empty() ? mapping[cur_index] : cur_index; }
    long from_original_index(long cur_index) const { return !mapping.empty() ?
        static_cast<long> (std::lower_bound(mapping.begin (), mapping.end (), cur_index) - mapping.begin ()) : cur_index; }
    long original_length () const { return !mapping.empty () ? mapping.back () : static_cast<long> (str.size ());}
public:
    std::wstring str;
    std::vector<long> mapping; // should have size str.length () or empty (if empty mapping is identity a<->a)
    // indices should correspond to offsets string `str` had in original encoding
};
