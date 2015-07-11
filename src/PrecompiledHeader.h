/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <functional>
#include <hash_set>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include "utils/enum_range.h"
#include "utils/enum_vector.h"
#include "utils/Uncopiable.h"

#include <cstring>
#include <tchar.h>
#include <time.h>
#include <shlwapi.h>
#include <string.h>
#include <windowsx.h>
#include <wchar.h>

#include <Shlobj.h>

/*
#if (_MSC_VER >= 1700)
#define _CRTDBG_MAP_ALLOC
#ifdef _DEBUG
#ifndef DEBUG_NEW
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#endif
#include <stdlib.h>
#include <crtdbg.h>/
#endif //_MSC_VER >= 1700
*/