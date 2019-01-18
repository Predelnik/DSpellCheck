#pragma once

constexpr auto COLOR_OK = RGB(0, 144, 0);
constexpr auto COLOR_FAIL = RGB(225, 0, 0);
constexpr auto COLOR_WARN = RGB(144, 144, 0);
constexpr auto COLOR_NEUTRAL = RGB(0, 0, 0);

// Custom WMs (Only for our windows and threads)
#define WM_SHOWANDRECREATEMENU (WM_USER + 1000)
