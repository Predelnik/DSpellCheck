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

#include "SpellCheckerHelpers.h"
#include "CommonFunctions.h"
#include "MappedWString.h"
#include "Settings.h"
#include "npp/EditorInterface.h"
#include "utils/string_utils.h"

namespace SpellCheckerHelpers {
static bool check_text_enabled_for_active_file(const EditorInterface &editor,
                                               const Settings &settings) {
  auto ret = !settings.check_those;
  auto full_path = editor.get_full_current_path();
  for (auto token : make_delimiter_tokenizer(settings.file_types, LR"(;)")
                        .get_all_tokens()) {
    if (settings.check_those) {
      ret =
          ret || PathMatchSpec(full_path.c_str(), std::wstring(token).c_str());
      if (ret)
        break;
    } else {
      ret &= ret &&
             (!PathMatchSpec(full_path.c_str(), std::wstring(token).c_str()));
      if (!ret)
        break;
    }
  }
  return ret;
}

bool is_spell_checking_needed(const EditorInterface &editor,
                              const Settings &settings) {
  return check_text_enabled_for_active_file(editor, settings) &&
         settings.auto_check_text;
}

void apply_word_conversions(const Settings &settings, std::wstring &word) {
  for (auto &c : word) {
    if (settings.ignore_yo) {
      if (c == L'ё')
        c = L'е';
      if (c == L'Ё')
        c = L'Е';
    }
    if (settings.convert_single_quotes) {
      if (c == L'’')
        c = L'\'';
    }
  }
}

void cut_apostrophes(const Settings &settings, std::wstring_view &word) {
  if (settings.remove_boundary_apostrophes) {
    while (!word.empty() && word.front() == L'\'')
      word.remove_prefix(1);

    while (!word.empty() && word.back() == L'\'')
      word.remove_suffix(1);
  }
}

MappedWstring to_mapped_wstring(const EditorInterface &editor,
                                EditorViewType view,
                                std::string_view str) {
  if (editor.get_encoding(view) == EditorCodepage::utf8)
    return utf8_to_mapped_wstring(str);

  return ::to_mapped_wstring(str);
}

void replace_all_tokens(EditorInterface& editor, EditorViewType view, const Settings& settings, const char* from, std::wstring to) {
  long pos = 0;
  editor.begin_undo_action(view);
  auto from_len = static_cast<long> (strlen (from));
  if (from_len == 0)
    return;

  while (true) {
    pos = editor.find_next(view, pos, from);
    if (pos >= 0) {
      auto start_pos = editor.get_prev_valid_begin_pos(view, pos);
      auto end_pos = editor.get_next_valid_end_pos(view, static_cast<long> (pos + from_len));
      auto rng = editor.get_text_range(view, start_pos, end_pos);
      auto mapped_wstr = SpellCheckerHelpers::to_mapped_wstring (editor, view, rng);
      size_t word_start = 1;
      size_t word_end = mapped_wstr.str.length() - 1;
      if (!settings.do_with_tokenizer(mapped_wstr.str, [&](const auto &tokenizer)
      {
        if (start_pos != pos) {
          if (tokenizer.prev_token_begin (static_cast<long> (mapped_wstr.str.length ()) - 2) != 1)
            return false;
        } else
          --word_start;
        if (end_pos != pos + from_len) {
          if (tokenizer.next_token_end (1) != static_cast<long> (mapped_wstr.str.length ()) - 1)
            return false;
        } else
          ++word_end;
        return true;
      })) {
        pos = pos + static_cast<long> (from_len);
        continue;
      }
      auto src_case_type = get_string_case_type(std::wstring_view (mapped_wstr.str).substr(word_start, word_end - word_start));
      if (src_case_type == string_case_type::mixed) {
        pos = pos + static_cast<long> (from_len);
        continue;
      } else
        apply_case_type (to, src_case_type);
      auto encoded_to = editor.to_editor_encoding(view, to);
      editor.replace_text(view, pos, static_cast<long> (pos + from_len), encoded_to);
      pos = pos + static_cast<long> (encoded_to.length());
    } else
      break;
  }
  editor.end_undo_action(view);
}
} // namespace SpellCheckerHelpers
