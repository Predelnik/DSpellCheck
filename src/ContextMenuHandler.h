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
#include "CommonFunctions.h"
#include "MappedWString.h"

class Settings;
class SpellerContainer;
class EditorInterface;
class SuggestionsButton;
class SpellChecker;

class ContextMenuHandler {
public:
  ContextMenuHandler(const Settings &settings,
                     const SpellerContainer &speller_container,
                     EditorInterface &editor,
                     const SpellChecker &spell_checker);
  [[nodiscard]] std::vector<MenuItem> get_suggestion_menu_items();
  void process_menu_result(WPARAM menu_id);
  void update_word_under_cursor_data();
  void precalculate_menu();
  void init_suggestions_box(SuggestionsButton &suggestion_button);

private:
  void do_plugin_menu_inclusion(bool invalidate = false);

private:
  const Settings &m_settings;
  const SpellerContainer &m_speller_container;
  EditorInterface &m_editor;
  MappedWstring m_selected_word;
  TextPosition m_word_under_cursor_length = 0;
  TextPosition m_word_under_cursor_pos = 0;
  std::vector<std::wstring> m_last_suggestions;
  const SpellChecker &m_spell_checker;
  bool m_word_under_cursor_is_correct = true;
};