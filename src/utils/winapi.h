#pragma once

// various winapi helpers

static std::wstring get_edit_text (HWND edit) {
  auto length = Edit_GetTextLength (edit);
  std::vector<wchar_t> buf (length + 1);
  Edit_GetText(edit, buf.data (), static_cast<int> (buf.size ()));
  return buf.data ();
}
