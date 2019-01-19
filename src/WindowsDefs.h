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

constexpr auto COLOR_OK = RGB(0, 144, 0);
constexpr auto COLOR_FAIL = RGB(225, 0, 0);
constexpr auto COLOR_WARN = RGB(144, 144, 0);
constexpr auto COLOR_NEUTRAL = RGB(0, 0, 0);

// Custom WMs (Only for our windows and threads)
constexpr auto WM_SHOWANDRECREATEMENU = WM_USER + 1000;
