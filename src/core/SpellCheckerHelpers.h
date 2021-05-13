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

#pragma once

class Settings;
class EditorInterface;
enum class NppViewType;

#include "plugin/Constants.h"

namespace SpellCheckerHelpers {
bool is_spell_checking_needed_for_file(const EditorInterface &editor, const Settings &settings);
void apply_word_conversions(const Settings &settings, std::wstring &word);
void cut_apostrophes(const Settings &settings, std::wstring_view &word);
// Replace all tokens equal `from` to `to`. Settings are required for tokenization style.
// If to is proper name or abbreviation it should be capitalized correctly otherwise it should be all lower case
void replace_all_tokens(EditorInterface &editor, const Settings &settings, const char *from, std::wstring_view to, bool
                        is_proper_name);
bool is_word_spell_checking_needed(const Settings &settings, const EditorInterface &editor, std::wstring_view word, TextPosition word_start);
} // namespace SpellCheckerHelpers
