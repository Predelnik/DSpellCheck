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

#define NOMINMAX
#include <winsock2.h>
#include <windows.h>

#include <array>
#include <map>
#include <set>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <stack>
#include <cstring>
#include <string>
#include <string_view>
#include <ctime>
#include <shlwapi.h>
#include <windowsx.h>
#include <cwchar>
#include <vector>
#include <ppltasks.h>
#include <regex>
#pragma warning(push)
#pragma warning(disable: 4091)
#include "Shlobj.h"
#pragma warning(pop)

#include "Plugin.h"  // plugin should be available everywhere to provide acess to rc_str function
#include "resource.h" // resource should be available everywhere to have acess to string ids

using namespace std::literals;
