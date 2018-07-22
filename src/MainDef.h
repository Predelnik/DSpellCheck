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

constexpr int dspellchecker_indicator_id = 19;
#define DEFAULT_BUF_SIZE 4096


#ifdef _DEBUG
#define ASSERT_RETURN(CONDITION,RET) do { if (!(CONDITION)) { assert (false); return RET; }} while (0)
#else // !_DEBUG
#define ASSERT_RETURN(CONDITION,RET) do { if (!(CONDITION)) return RET; } while (0)
#endif // !_DEBUG

enum class EncodingType { utf8 = 0, ansi };

enum class CustomGuiMessage {
  generic_callback,
  max,
};

const wchar_t *const custom_gui_messsages_names[] = {L"DSpellCheck_GenericCallback" };

// Global Menu ID
#define DSPELLCHECK_MENU_ID 193
#define LANGUAGE_MENU_ID 197

// Menu item IDs
#define MID_ADDTODICTIONARY 101
#define MID_IGNOREALL 102
#define MULTIPLE_LANGS 201
#define CUSTOMIZE_MULTIPLE_DICS 202
#define DOWNLOAD_DICS 203
#define REMOVE_DICS 204

#define USER_SERVER_CONST 100

constexpr auto COLOR_OK = RGB(0, 144, 0);
constexpr auto COLOR_FAIL = RGB(225, 0, 0);
constexpr auto COLOR_WARN = RGB(144, 144, 0);
constexpr auto COLOR_NEUTRAL = RGB(0, 0, 0);

constexpr auto static_plugin_name = L"DSpellCheck";
constexpr auto config_file_name = L"DSpellCheck.ini";

// Custom WMs (Only for our windows and threads)
#define WM_SHOWANDRECREATEMENU (WM_USER + 1000)
