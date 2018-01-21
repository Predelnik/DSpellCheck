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
} // namespace SpellCheckerHelpers
