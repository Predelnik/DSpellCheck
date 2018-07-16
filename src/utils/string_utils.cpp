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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA.

#include "string_utils.h"

#include <algorithm>

void to_lower_inplace(std::wstring& s)
{
	std::transform(s.begin(), s.end (), s.begin (), &towlower);
}

void to_upper_inplace(std::wstring& s) { std::transform(s.begin(), s.end(), s.begin(), &towupper); }

void ltrim_inplace(std::wstring& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch)
	{
		return iswspace(ch) == FALSE;
	}));
}

void rtrim_inplace(std::wstring& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch)
	{
		return iswspace(ch) == FALSE;
	}).base(), s.end());
}

void trim_inplace(std::wstring& s)
{
	ltrim_inplace(s);
	rtrim_inplace(s);
}
