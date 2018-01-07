#pragma once

struct SuggestionsMenuItem {
  std::wstring text;
  BYTE id;
  bool separator;
  SuggestionsMenuItem(const wchar_t *text_arg, int id_arg,
                      bool separator_arg = false);
};

void insert_sugg_menu_item(HMENU menu, const wchar_t *text, BYTE id,
                           int insert_pos, bool separator = false);