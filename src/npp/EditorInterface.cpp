#include "EditorInterface.h"

long EditorInterface::get_prev_valid_begin_pos(EditorViewType view, long pos) const {
  if (pos == 0)
    return 0;
  if (get_encoding(view) == EditorCodepage::ansi)
    return pos - 1;

  auto worst_prev_pos = std::max(0l, pos - static_cast<long>(max_utf8_char_length));
  auto rng = get_text_range(view, worst_prev_pos, pos);
  auto it = std::find_if(rng.rbegin(), rng.rend(), &utf8_is_lead);
  assert (it != rng.rend ());
  return worst_prev_pos + static_cast<long>(it.base() - rng.begin()) - 1;
}

long EditorInterface::get_next_valid_end_pos(EditorViewType view, long pos) const {
  auto doc_len = get_active_document_length(view);
  if (pos >= doc_len)
    return doc_len;

  if (get_encoding(view) == EditorCodepage::ansi)
    return pos + 1;

  auto text = get_text_range(view, pos, pos + 1);
  return pos + utf8_symbol_len(*text.begin());
}

std::string EditorInterface::to_editor_encoding(EditorViewType view, std::wstring_view str) const {
  switch (get_encoding(view)) {
  case EditorCodepage::ansi: return to_string (str);
  case EditorCodepage::utf8: return to_utf8_string(str);
  case EditorCodepage::COUNT: break;
  }
  std::abort ();
}
