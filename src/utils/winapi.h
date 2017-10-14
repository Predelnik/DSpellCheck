#pragma once

// various winapi helpers
std::wstring get_edit_text(HWND edit);
bool move_file_and_reset_security_descriptor (const wchar_t* from, const wchar_t* to);
