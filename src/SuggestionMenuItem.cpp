#include "SuggestionMenuItem.h"
#include "MainDef.h"

SuggestionsMenuItem::SuggestionsMenuItem(const wchar_t *text_arg, int id_arg,
                                         bool separator_arg /*= false*/) {
  text = text_arg;
  id = static_cast<BYTE>(id_arg);
  separator = separator_arg;
}

void insert_sugg_menu_item(HMENU menu, const wchar_t *text, BYTE id,
                           int insert_pos, bool separator) {
  MENUITEMINFO mi;
  memset(&mi, 0, sizeof(mi));
  mi.cbSize = sizeof(MENUITEMINFO);
  if (separator) {
    mi.fType = MFT_SEPARATOR;
  } else {
    mi.fType = MFT_STRING;
    mi.fMask = MIIM_ID | MIIM_TYPE;
    if (!get_use_allocated_ids())
      mi.wID = MAKEWORD(id, DSPELLCHECK_MENU_ID);
    else
      mi.wID = get_context_menu_id_start() + id;

    mi.dwTypeData = const_cast<wchar_t *>(text);
    mi.cch = static_cast<int>(wcslen(text)) + 1;
  }
  if (insert_pos == -1)
    InsertMenuItem(menu, GetMenuItemCount(menu), 1, &mi);
  else
    InsertMenuItem(menu, insert_pos, 1, &mi);
}