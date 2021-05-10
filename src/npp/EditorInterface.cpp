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

#include "EditorInterface.h"

#include "TextUtils.h"
#include "common/utf8.h"
#include "common/Utility.h"
#include <cassert>

POINT EditorInterface::get_point_from_position(TextPosition position) const {
  return {get_point_x_from_position(position),
          get_point_y_from_position(position)};
}

TextPosition EditorInterface::get_prev_valid_begin_pos(TextPosition pos) const {
  if (pos == 0)
    return 0;
  if (get_encoding() == EditorCodepage::ansi)
    return pos - 1;

  auto worst_prev_pos = std::max(0_sz, pos - static_cast<TextPosition>(max_utf8_char_length));
  auto rng = get_text_range(worst_prev_pos, pos);
  auto it = std::find_if(rng.rbegin(), rng.rend(), &utf8_is_lead);
  assert(it != rng.rend ());
  return worst_prev_pos + (it.base() - rng.begin()) - 1;
}

TextPosition EditorInterface::get_next_valid_end_pos(TextPosition pos) const {
  auto doc_len = get_active_document_length();
  if (pos >= doc_len)
    return doc_len;

  if (get_encoding() == EditorCodepage::ansi)
    return pos + 1;

  auto text = get_text_range(pos, pos + 1);
  return pos + utf8_symbol_len(*text.begin());
}

std::string EditorInterface::to_editor_encoding(std::wstring_view str) const {
  switch (get_encoding()) {
  case EditorCodepage::ansi:
    return to_string(str);
  case EditorCodepage::utf8:
    return to_utf8_string(str);
  case EditorCodepage::COUNT:
    break;
  }
  throw std::runtime_error("Unsupported encoding");
}

MappedWstring EditorInterface::to_mapped_wstring(const std::string &str) {
  if (get_encoding() == EditorCodepage::utf8)
    return utf8_to_mapped_wstring(str);

  return ::to_mapped_wstring(str);
}

MappedWstring EditorInterface::get_mapped_wstring_range(TextPosition from, TextPosition to) {
  auto result = to_mapped_wstring(get_text_range(from, to));
  for (auto &val : result.mapping)
    val += from;
  return result;
}

MappedWstring EditorInterface::get_mapped_wstring_line(TextPosition line) {
  auto result = to_mapped_wstring(get_line(line));;
  auto line_start = get_line_start_position(line);
  for (auto &val : result.mapping)
    val += line_start;
  return result;
}
