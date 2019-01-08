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

#pragma once

class Settings;
class EditorInterface;
class MappedWstring;
enum class EditorViewType;

namespace SpellCheckerHelpers {
bool is_spell_checking_needed(const EditorInterface &editor,
                              const Settings &settings);
void apply_word_conversions(const Settings &settings, std::wstring &word);
void cut_apostrophes(const Settings &settings, std::wstring_view &word);
MappedWstring to_mapped_wstring(const EditorInterface &editor,
                                EditorViewType view, std::string_view str);
// Replace all tokens equal `from` to `to`. Settings are required for tokenization style.
void replace_all_tokens (EditorInterface& editor, EditorViewType view, const Settings &settings, const char* from, const char* to);
} // namespace SpellCheckerHelpers