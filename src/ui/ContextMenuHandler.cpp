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

#include "ContextMenuHandler.h"

#include "MenuItem.h"
#include "SuggestionsButton.h"
#include "core/SpellChecker.h"
#include "core/SpellCheckerHelpers.h"
#include "npp/EditorInterface.h"
#include "npp/NppInterface.h"
#include "plugin/MainDefs.h"
#include "plugin/Plugin.h"
#include "plugin/resource.h"
#include "plugin/Settings.h"
#include "spellers/LanguageInfo.h"
#include "spellers/SpellerContainer.h"
#include "spellers/SpellerInterface.h"

void ContextMenuHandler::do_plugin_menu_inclusion(bool invalidate) {
  MENUITEMINFO mif;
  HMENU dspellcheck_menu = get_this_plugin_menu();
  if (dspellcheck_menu == nullptr)
    return;
  HMENU langs_sub_menu = get_langs_sub_menu();
  if (langs_sub_menu != nullptr)
    DestroyMenu(langs_sub_menu);
  auto cur_lang = m_settings.get_active_language();
  HMENU new_menu = CreatePopupMenu();
  if (!invalidate) {
    auto langs = m_speller_container.get_available_languages();
    if (!langs.empty()) {
      int i = 0;
      for (auto &lang : langs) {
        int checked = (cur_lang == lang.orig_name)
                        ? (MFT_RADIOCHECK | MF_CHECKED)
                        : MF_UNCHECKED;
        bool res = AppendMenu(
            new_menu, MF_STRING | checked,
            get_use_allocated_ids()
              ? i + get_langs_menu_id_start()
              : MAKEWORD(i, menu_id::language_menu),
            m_settings.data.language_name_style != LanguageNameStyle::original
              ? lang.alias_name.c_str()
              : lang.orig_name.c_str());
        if (!res)
          return;
        ++i;
      }
      int checked = (cur_lang == multiple_language_alias)
                      ? (MFT_RADIOCHECK | MF_CHECKED)
                      : MF_UNCHECKED;
      AppendMenu(new_menu, MF_STRING | checked,
                 get_use_allocated_ids()
                   ? menu_id::multiple_languages + get_langs_menu_id_start()
                   : MAKEWORD(menu_id::multiple_languages, menu_id::language_menu),
                 rc_str(IDS_MULTIPLE_LANGUAGES).c_str());
      AppendMenu(new_menu, MF_SEPARATOR, 0, nullptr);
      AppendMenu(new_menu, MF_STRING,
                 get_use_allocated_ids()
                   ? menu_id::customize_multiple_languages + get_langs_menu_id_start()
                   : MAKEWORD(menu_id::customize_multiple_languages, menu_id::language_menu),
                 rc_str(IDS_SET_MULTIPLE_LANG).c_str());
      if (m_settings.data.active_speller_lib_id ==
          SpellerId::hunspell) // Only Hunspell supported
      {
        AppendMenu(new_menu, MF_STRING,
                   get_use_allocated_ids()
                     ? menu_id::download_dictionaries + get_langs_menu_id_start()
                     : MAKEWORD(menu_id::download_dictionaries, menu_id::language_menu),
                   rc_str(IDS_DOWNLOAD_LANGS).c_str());
        AppendMenu(new_menu, MF_STRING,
                   get_use_allocated_ids()
                     ? menu_id::remove_dictionaries + get_langs_menu_id_start()
                     : MAKEWORD(menu_id::remove_dictionaries, menu_id::language_menu),
                   rc_str(IDS_REMOVE_LANG).c_str());
      }
    } else if (m_settings.data.active_speller_lib_id == SpellerId::hunspell)
      AppendMenu(new_menu, MF_STRING,
                 get_use_allocated_ids()
                   ? menu_id::download_dictionaries + get_langs_menu_id_start()
                   : MAKEWORD(menu_id::download_dictionaries, menu_id::language_menu),
                 rc_str(IDS_DOWNLOAD_LANG).c_str());
  }

  mif.fMask = MIIM_SUBMENU | MIIM_STATE;
  mif.cbSize = sizeof(MENUITEMINFO);
  mif.hSubMenu = new_menu;
  mif.fState = MFS_ENABLED;

  SetMenuItemInfo(dspellcheck_menu, action_index[Action::quick_language_change], TRUE, &mif);
}

void ContextMenuHandler::process_menu_result(WPARAM menu_id) {
  if ((!get_use_allocated_ids() && HIBYTE(menu_id) != menu_id::plguin_menu &&
       HIBYTE(menu_id) != get_func_item()[action_index[Action::quick_language_change]].cmd_id) ||
      (get_use_allocated_ids() &&
       (static_cast<int>(menu_id) < get_context_menu_id_start() ||
        static_cast<int>(menu_id) >= get_context_menu_id_start() + requested_menu_count)))
    return;
  int used_menu_id;
  if (get_use_allocated_ids()) {
    used_menu_id = (static_cast<int>(menu_id) < get_langs_menu_id_start()
                      ? menu_id::plguin_menu
                      : menu_id::language_menu);
  } else {
    used_menu_id = HIBYTE(menu_id);
  }
  ACTIVE_VIEW_BLOCK(m_editor);

  switch (used_menu_id) {
  case menu_id::plguin_menu: {
    WPARAM result;
    if (!get_use_allocated_ids())
      result = LOBYTE(menu_id);
    else
      result = menu_id - get_context_menu_id_start();

    if (result != 0) {
      if (result == menu_id::ignore_all) {
        auto mut = m_speller_container.modify();
        mut->ignore_word(m_selected_word.str);
        m_word_under_cursor_length =
            static_cast<TextPosition>(m_selected_word.str.length());
        m_editor.set_cursor_pos(m_word_under_cursor_pos +
                                m_word_under_cursor_length);
      } else if (result == menu_id::add_to_dictionary) {
        auto mut = m_speller_container.modify();
        mut->add_to_dictionary(m_selected_word.str);
        m_word_under_cursor_length =
            static_cast<TextPosition>(m_selected_word.str.length());
        m_editor.set_cursor_pos(m_word_under_cursor_pos +
                                m_word_under_cursor_length);
      } else if (result <= m_last_suggestions.size()) {
        std::string encoded_str;
        if (m_editor.get_encoding() == EditorCodepage::ansi)
          encoded_str = to_string(m_last_suggestions[result - 1].c_str());
        else
          encoded_str = to_utf8_string(m_last_suggestions[result - 1]);

        m_editor.replace_selection(encoded_str.c_str());
      } else if (result <= menu_id::replace_all_start + m_last_suggestions.size()) {
        auto misspelled_text = m_editor.selected_text();
        auto &suggestion = m_last_suggestions[result - menu_id::replace_all_start - 1];
        UNDO_BLOCK(m_editor);
        // not replacing originally selected word is unexpected behaviour, so we replace it with the exact suggestion
        m_editor.replace_selection(m_editor.to_editor_encoding(suggestion).c_str());

        bool is_proper_name = true;
        if (!suggestion.empty()) {
          auto suggestion_copy = suggestion;
          to_lower_inplace(suggestion_copy);
          // if lowercase version is incorrect - the word is not a proper name
          if (m_speller_container.active_speller().check_word({suggestion_copy}))
            is_proper_name = false;
        }

        SpellCheckerHelpers::replace_all_tokens(m_editor, m_settings, misspelled_text.c_str(), suggestion, is_proper_name);
      }
    }
  }
  break;
  case menu_id::language_menu: {
    WPARAM result;
    if (!get_use_allocated_ids())
      result = LOBYTE(menu_id);
    else
      result = menu_id - get_langs_menu_id_start();

    std::wstring lang_string;
    if (result == menu_id::multiple_languages) {
      lang_string = multiple_language_alias;
    } else if (result == menu_id::customize_multiple_languages || result == menu_id::download_dictionaries ||
               result == menu_id::remove_dictionaries) {
      // All actions are done in GUI thread in that case
      return;
    } else
      lang_string =
          m_speller_container.get_available_languages()[result].orig_name;
    do_plugin_menu_inclusion(true);

    auto mut_settings = m_settings.modify(SettingsModificationStyle::ignore_file_errors);
    mut_settings->get_active_language() = lang_string;
    break;
  }
  default:
    break;
  }
}

void ContextMenuHandler::update_word_under_cursor_data() {
  TextPosition pos, length;
  m_word_under_cursor_is_correct =
      m_spell_checker.is_word_under_cursor_correct(pos, length, true);
  if (!m_word_under_cursor_is_correct) {
    m_word_under_cursor_pos = pos;
    m_word_under_cursor_length = length;
  }
}

void ContextMenuHandler::precalculate_menu() {
  std::vector<MenuItem> suggestion_menu_items;
  if (SpellCheckerHelpers::is_spell_checking_needed_for_file(m_editor, m_settings) &&
      m_settings.data.suggestions_mode == SuggestionMode::context_menu) {
    update_word_under_cursor_data();
    if (!m_word_under_cursor_is_correct) {
      suggestion_menu_items = get_suggestion_menu_items();
      suggestion_menu_items.emplace_back(MenuItem::Separator{});
    }
  }
  show_calculated_menu(std::move(suggestion_menu_items));
}

void ContextMenuHandler::init_suggestions_box(
    SuggestionsButton &suggestion_button) {
  if (m_settings.data.suggestions_mode != SuggestionMode::button)
    return;
  if (!m_speller_container.active_speller().is_working())
    return;
  POINT p;
  if (!SpellCheckerHelpers::is_spell_checking_needed_for_file(
      m_editor, m_settings)) // If there's no red underline let's do nothing
  {
    suggestion_button.display(false);
    return;
  }

  TextPosition pos, length;
  if (m_spell_checker.is_word_under_cursor_correct(pos, length)) {
    return;
  }
  m_word_under_cursor_length = length;
  m_word_under_cursor_pos = pos;
  ACTIVE_VIEW_BLOCK(m_editor);
  auto line = m_editor.line_from_position(m_word_under_cursor_pos);
  auto text_height = m_editor.get_text_height(line);
  auto x_pos =
      m_editor.get_point_x_from_position(m_word_under_cursor_pos);
  auto y_pos =
      m_editor.get_point_y_from_position(m_word_under_cursor_pos);
  p.x = static_cast<LONG>(x_pos);
  p.y = static_cast<LONG>(y_pos);
  auto npp = dynamic_cast<NppInterface *>(&m_editor);
  if (npp == nullptr)
    return;
  RECT r;
  auto hwnd = npp->get_view_hwnd();
  GetWindowRect(hwnd, &r);

  ClientToScreen(hwnd, &p);

  if (p.y + text_height - 3 + m_settings.data.suggestion_button_size >= r.bottom) {
    if (p.x > m_settings.data.suggestion_button_size) {
      p.y -= m_settings.data.suggestion_button_size;
      p.x -= m_settings.data.suggestion_button_size;
    } else
      p.y -= (text_height + m_settings.data.suggestion_button_size);
  }

  if (p.x + m_settings.data.suggestion_button_size > r.right)
    p.x -= m_settings.data.suggestion_button_size;

  if (r.top > p.y + text_height - 3 || r.left > p.x)
    return;

  MoveWindow(suggestion_button.getHSelf(), p.x,
             p.y + static_cast<int>(text_height) - 3,
             m_settings.data.suggestion_button_size,
             m_settings.data.suggestion_button_size, 1);
  suggestion_button.display(true, false);
}

std::vector<MenuItem>
ContextMenuHandler::get_suggestion_menu_items() {
  if (!m_speller_container.active_speller().is_working())
    return {}; // Word is already off-screen

  auto pos = m_word_under_cursor_pos;
  ACTIVE_VIEW_BLOCK(m_editor);
  m_editor.set_selection(pos, pos + m_word_under_cursor_length);
  std::vector<MenuItem> suggestion_menu_items;
  m_selected_word = m_editor.get_mapped_wstring_range(m_word_under_cursor_pos, m_word_under_cursor_pos + static_cast<TextPosition>(m_word_under_cursor_length));
  SpellCheckerHelpers::apply_word_conversions(m_settings, m_selected_word.str);

  m_last_suggestions = m_speller_container.active_speller().get_suggestions(
      m_selected_word.str.c_str());

  for (int i = 0; i < static_cast<int>(m_last_suggestions.size()); i++) {
    if (i >= m_settings.data.suggestion_count)
      break;

    auto item = m_last_suggestions[i].c_str();
    suggestion_menu_items.emplace_back(item, static_cast<BYTE>(i + 1));
  }

  if (!m_last_suggestions.empty()) {
    MenuItem replace_all_item(wstring_printf(rc_str(IDS_REPLACE_ALL_PS).c_str(), m_selected_word.str.c_str()).c_str(), -1);
    replace_all_item.children = suggestion_menu_items;
    suggestion_menu_items.emplace_back(MenuItem::Separator{});
    std::for_each(replace_all_item.children.begin(), replace_all_item.children.end(), [](auto &item) { item.id += menu_id::replace_all_start; });
    suggestion_menu_items.push_back(std::move(replace_all_item));
  }

  SpellCheckerHelpers::apply_word_conversions(m_settings, m_selected_word.str);
  auto menu_string = wstring_printf(rc_str(IDS_IGNORE_PS_FOR_CURRENT_SESSION).c_str(),
                                    m_selected_word.str.c_str());
  suggestion_menu_items.emplace_back(menu_string.c_str(), menu_id::ignore_all);
  menu_string =
      wstring_printf(rc_str(IDS_ADD_PS_TO_DICTIONARY).c_str(), m_selected_word.str.c_str());;
  suggestion_menu_items.emplace_back(menu_string.c_str(),
                                     menu_id::add_to_dictionary);

  return suggestion_menu_items;
}

ContextMenuHandler::ContextMenuHandler(
    const Settings &settings, const SpellerContainer &speller_container,
    EditorInterface &editor, const SpellChecker &spell_checker)
  : m_settings(settings), m_speller_container(speller_container),
    m_editor(editor), m_spell_checker(spell_checker) {
  m_speller_container.speller_status_changed.connect(
      [this]() { do_plugin_menu_inclusion(); });
}
