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

constexpr int spell_check_indicator_id = 19;

#ifdef _DEBUG
#define ASSERT_RETURN(CONDITION, RET)                                                                                                                          \
  do {                                                                                                                                                         \
    if (!(CONDITION)) {                                                                                                                                        \
      assert(false);                                                                                                                                           \
      return RET;                                                                                                                                              \
    }                                                                                                                                                          \
  } while (0)
#else // !_DEBUG
#define ASSERT_RETURN(CONDITION, RET)                                                                                                                          \
  do {                                                                                                                                                         \
    if (!(CONDITION))                                                                                                                                          \
      return RET;                                                                                                                                              \
  } while (0)
#endif // !_DEBUG

enum class EncodingType { utf8 = 0, ansi };

enum class CustomGuiMessage {
  generic_callback,
  max,
};

const wchar_t *const custom_gui_messages_names[] = {L"DSpellCheck_GenericCallback"};

namespace menu_id {
// Global Menu ID
constexpr auto plguin_menu = 193;
constexpr auto language_menu = 197;

// Menu item IDs
constexpr auto replace_all_start = 50;
constexpr auto add_to_dictionary = 101;
constexpr auto ignore_all = 102;
constexpr auto multiple_languages = 201;
constexpr auto customize_multiple_languages = 202;
constexpr auto download_dictionaries = 203;
constexpr auto remove_dictionaries = 204;
} // namespace menu_id

constexpr auto USER_SERVER_CONST = 100;

constexpr auto static_plugin_name = L"DSpellCheck";
constexpr auto config_file_name = L"DSpellCheck.ini";
constexpr auto multiple_language_alias = L"<MULTIPLE>";

using TextPosition = ptrdiff_t;

constexpr std::size_t operator "" _z(unsigned long long n)
{
  return n;
}

constexpr std::ptrdiff_t operator "" _sz(unsigned long long n)
{
  return n;
}
