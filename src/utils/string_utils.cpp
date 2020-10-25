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

#include "string_utils.h"

#include <algorithm>
#include <cassert>
#include <cctype>

void to_lower_inplace(std::wstring &s) {
  std::transform(s.begin(), s.end(), s.begin(), &towlower);
}

void to_upper_inplace(std::wstring &s) { std::transform(s.begin(), s.end(), s.begin(), &towupper); }

void ltrim_inplace(std::wstring &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch)
  {
    return iswspace(ch) == FALSE;
  }));
}

void rtrim_inplace(std::wstring &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch)
  {
    return iswspace(ch) == FALSE;
  }).base(), s.end());
}

void trim_inplace(std::wstring &s) {
  ltrim_inplace(s);
  rtrim_inplace(s);
}

size_t find_case_insensitive(const std::string_view &str_haystack, const std::string_view &str_needle) {
  // courtesy of https://stackoverflow.com/a/19839371/1269661
  auto it = std::search(
                        str_haystack.begin(), str_haystack.end(),
                        str_needle.begin(), str_needle.end(),
                        [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
                       );
  if (it == str_haystack.end ())
    return std::string_view::npos;
  return it - str_haystack.begin ();
}

string_case_type get_string_case_type(const std::wstring_view &sv) {
  if (sv.empty ())
    return string_case_type::mixed;
  size_t lower_count = std::count_if (sv.begin (), sv.end (), &IsCharLower);
  if (lower_count == 0)
    return string_case_type::upper;
  if (lower_count == sv.length ())
    return string_case_type::lower;
  if (lower_count == sv.length () - 1 && IsCharUpper (sv.front ()))
    return string_case_type::title;

  return string_case_type::mixed;
}

void apply_case_type(std::wstring& str, string_case_type type) {
  if (str.empty ())
    return;
  std::transform(str.begin (), str.end (), str.begin (), [&](wchar_t wc) -> wchar_t {
    switch (type)
    {
    case string_case_type::lower:
    case string_case_type::title:
      return make_lower (wc);
    case string_case_type::upper:
      return make_upper (wc);
    case string_case_type::mixed: throw std::invalid_argument ("apply_case_type: inapplicable for mixed case"); break;
    }
    throw std::invalid_argument ("apply_case_type: corrupted enum value for type");
  }
  );
  if (type == string_case_type::title)
    str.front () = make_upper(str.front());
}

wchar_t make_upper(wchar_t c) {
  return static_cast<wchar_t> (reinterpret_cast<std::size_t> (CharUpper (reinterpret_cast<LPWSTR>(c))));
}

wchar_t make_lower(wchar_t c) {
  return static_cast<wchar_t> (reinterpret_cast<std::size_t> (CharLower (reinterpret_cast<LPWSTR>(c))));
}

bool is_upper(wchar_t c) {
  return IsCharUpper (c);
}

bool is_lower(wchar_t c) {
  return IsCharLower (c);
}
