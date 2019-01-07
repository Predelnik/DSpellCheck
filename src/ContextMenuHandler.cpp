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

#include "ContextMenuHandler.h"
#include "LanguageInfo.h"
#include "MainDef.h"
#include "Plugin.h"
#include "SpellChecker.h"
#include "SpellCheckerHelpers.h"
#include "SpellerContainer.h"
#include "SpellerInterface.h"
#include "SuggestionsButton.h"
#include "npp/EditorInterface.h"
#include "npp/NppInterface.h"
#include "resource.h"
#include "MenuItem.h"
#include "Settings.h"

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
            get_use_allocated_ids() ? i + get_langs_menu_id_start()
                                    : MAKEWORD(i, LANGUAGE_MENU_ID),
            m_settings.language_name_style != LanguageNameStyle::original ? lang.alias_name.c_str()
                                                 : lang.orig_name.c_str());
        if (!res)
          return;
        ++i;
      }
      int checked = (cur_lang == multiple_language_alias) ? (MFT_RADIOCHECK | MF_CHECKED)
                                                : MF_UNCHECKED;
      AppendMenu(new_menu, MF_STRING | checked,
                 get_use_allocated_ids()
                     ? MULTIPLE_LANGS + get_langs_menu_id_start()
                     : MAKEWORD(MULTIPLE_LANGS, LANGUAGE_MENU_ID),
                 rc_str(IDS_MULTIPLE_LANGUAGES).c_str ());
      AppendMenu(new_menu, MF_SEPARATOR, 0, nullptr);
      AppendMenu(new_menu, MF_STRING,
                 get_use_allocated_ids()
                     ? CUSTOMIZE_MULTIPLE_DICS + get_langs_menu_id_start()
                     : MAKEWORD(CUSTOMIZE_MULTIPLE_DICS, LANGUAGE_MENU_ID),
                 rc_str(IDS_SET_MULTIPLE_LANG).c_str ());
      if (m_settings.active_speller_lib_id ==
          SpellerId::hunspell) // Only Hunspell supported
      {
        AppendMenu(new_menu, MF_STRING,
                   get_use_allocated_ids()
                       ? DOWNLOAD_DICS + get_langs_menu_id_start()
                       : MAKEWORD(DOWNLOAD_DICS, LANGUAGE_MENU_ID),
                   rc_str(IDS_DOWNLOAD_LANGS).c_str ());
        AppendMenu(new_menu, MF_STRING,
                   get_use_allocated_ids()
                       ? REMOVE_DICS + get_langs_menu_id_start()
                       : MAKEWORD(REMOVE_DICS, LANGUAGE_MENU_ID),
                   rc_str(IDS_REMOVE_LANG).c_str ());
      }
    } else if (m_settings.active_speller_lib_id == SpellerId::hunspell)
      AppendMenu(new_menu, MF_STRING,
                 get_use_allocated_ids()
                     ? DOWNLOAD_DICS + get_langs_menu_id_start()
                     : MAKEWORD(DOWNLOAD_DICS, LANGUAGE_MENU_ID),
                 rc_str(IDS_DOWNLOAD_LANG).c_str ());
  }

  mif.fMask = MIIM_SUBMENU | MIIM_STATE;
  mif.cbSize = sizeof(MENUITEMINFO);
  mif.hSubMenu = new_menu;
  mif.fState = MFS_ENABLED;

  SetMenuItemInfo(dspellcheck_menu, action_index[Action::quick_language_change], TRUE, &mif);
}

void ContextMenuHandler::process_menu_result(WPARAM menu_id) {
  if ((!get_use_allocated_ids() && HIBYTE(menu_id) != DSPELLCHECK_MENU_ID &&
       HIBYTE(menu_id) != get_func_item()[action_index[Action::quick_language_change]].cmd_id) ||
      (get_use_allocated_ids() &&
       (static_cast<int>(menu_id) < get_context_menu_id_start() ||
        static_cast<int>(menu_id) > get_context_menu_id_start() + 350)))
    return;
  int used_menu_id;
  if (get_use_allocated_ids()) {
    used_menu_id = (static_cast<int>(menu_id) < get_langs_menu_id_start()
                        ? DSPELLCHECK_MENU_ID
                        : LANGUAGE_MENU_ID);
  } else {
    used_menu_id = HIBYTE(menu_id);
  }
  auto view = m_editor.active_view();

  switch (used_menu_id) {
  case DSPELLCHECK_MENU_ID: {
    WPARAM result;
    if (!get_use_allocated_ids())
      result = LOBYTE(menu_id);
    else
      result = menu_id - get_context_menu_id_start();

    if (result != 0) {
      if (result == MID_IGNOREALL) {
        SpellCheckerHelpers::apply_word_conversions(m_settings,
                                                    m_selected_word.str);
        auto mut = m_speller_container.modify();
        mut->active_speller().ignore_all(m_selected_word.str.c_str());
        m_word_under_cursor_length =
            static_cast<long>(m_selected_word.str.length());
        m_editor.set_cursor_pos(view, m_word_under_cursor_pos +
                                          m_word_under_cursor_length);
      } else if (result == MID_ADDTODICTIONARY) {
        SpellCheckerHelpers::apply_word_conversions(m_settings,
                                                    m_selected_word.str);
        auto mut = m_speller_container.modify();
        mut->active_speller().add_to_dictionary(m_selected_word.str.c_str());
        m_word_under_cursor_length =
            static_cast<long>(m_selected_word.str.length());
        m_editor.set_cursor_pos(view, m_word_under_cursor_pos +
                                          m_word_under_cursor_length);
      } else if (result <= m_last_suggestions.size()) {
        std::string encoded_str;
        if (m_editor.get_encoding(view) == EditorCodepage::ansi)
          encoded_str = to_string(m_last_suggestions[result - 1].c_str());
        else
          encoded_str = to_utf8_string(m_last_suggestions[result - 1]);

        m_editor.replace_selection(view, encoded_str.c_str());
      } else if (result <= MID_REPLACE_ALL_START + m_last_suggestions.size()) {
        std::string encoded_suggestion;
        if (m_editor.get_encoding(view) == EditorCodepage::ansi)
          encoded_suggestion = to_string(m_last_suggestions[result - MID_REPLACE_ALL_START - 1].c_str());
        else
          encoded_suggestion = to_utf8_string(m_last_suggestions[result - MID_REPLACE_ALL_START - 1]);
        long pos = 0;
        auto misspelled_text = m_editor.selected_text(view);
        m_editor.begin_undo_action(view);
        while (true) {
          pos = m_editor.find_next(view, pos, misspelled_text.c_str ());
          if (pos >= 0) {
            auto start_pos = m_editor.get_prev_valid_begin_pos(view, pos);
            auto end_pos = m_editor.get_next_valid_end_pos(view, static_cast<long> (pos + misspelled_text.length()));
            auto rng = m_editor.get_text_range(view, start_pos, end_pos);
            auto mapped_wstr = SpellCheckerHelpers::to_mapped_wstring (m_editor, view, rng);
            if (!m_settings.do_with_tokenizer(mapped_wstr.str, [&](const auto &tokenizer)
            {
              return (start_pos == pos || tokenizer.prev_token_begin (static_cast<long> (mapped_wstr.str.length ()) - 2) == 1) &&
                     (end_pos == pos + misspelled_text.length() || tokenizer.next_token_end (1) == static_cast<long> (mapped_wstr.str.length ()) - 1);
            })) {
              pos = pos + static_cast<long> (misspelled_text.length());
              continue;
            }
            m_editor.replace_text(view, pos, static_cast<long> (pos + misspelled_text.length()), encoded_suggestion);
            pos = pos + static_cast<long> (encoded_suggestion.length()) - static_cast<long> (misspelled_text.length());
          } else
            break;
        }
        m_editor.end_undo_action(view);
      }
    }
  } break;
  case LANGUAGE_MENU_ID: {
    WPARAM result;
    if (!get_use_allocated_ids())
      result = LOBYTE(menu_id);
    else
      result = menu_id - get_langs_menu_id_start();

    std::wstring lang_string;
    if (result == MULTIPLE_LANGS) {
      lang_string = multiple_language_alias;
    } else if (result == CUSTOMIZE_MULTIPLE_DICS || result == DOWNLOAD_DICS ||
               result == REMOVE_DICS) {
      // All actions are done in GUI thread in that case
      return;
    } else
      lang_string =
          m_speller_container.get_available_languages()[result].orig_name;
    do_plugin_menu_inclusion(true);

    auto mut_settings = m_settings.modify();
    mut_settings->get_active_language() = lang_string;
    break;
  }
  default:
    break;
  }
}

void ContextMenuHandler::update_word_under_cursor_data () {
  long pos, length;
    m_word_under_cursor_is_correct =
        m_spell_checker.is_word_under_cursor_correct(pos, length, true);
    if (!m_word_under_cursor_is_correct) {
      m_word_under_cursor_pos = pos;
      m_word_under_cursor_length = length;
    }
}

void ContextMenuHandler::precalculate_menu() {
  std::vector<MenuItem> suggestion_menu_items;
  if (SpellCheckerHelpers::is_spell_checking_needed (m_editor, m_settings) &&
      m_settings.suggestions_mode == SuggestionMode::context_menu) {
      update_word_under_cursor_data ();
      suggestion_menu_items = get_suggestion_menu_items();
    }
  show_calculated_menu(std::move(suggestion_menu_items));
}

void ContextMenuHandler::init_suggestions_box(
    SuggestionsButton &suggestion_button) {
  if (m_settings.suggestions_mode != SuggestionMode::button)
    return;
  if (!m_speller_container.active_speller().is_working())
    return;
  POINT p;
  if (!SpellCheckerHelpers::is_spell_checking_needed(
          m_editor, m_settings)) // If there's no red underline let's do nothing
  {
    suggestion_button.display(false);
    return;
  }

  long pos, length;
  if (m_spell_checker.is_word_under_cursor_correct(pos, length)) {
    return;
  }
  m_word_under_cursor_length = length;
  m_word_under_cursor_pos = pos;
  auto view = m_editor.active_view();
  auto line = m_editor.line_from_position(view, m_word_under_cursor_pos);
  auto text_height = m_editor.get_text_height(view, line);
  auto x_pos =
      m_editor.get_point_x_from_position(view, m_word_under_cursor_pos);
  auto y_pos =
      m_editor.get_point_y_from_position(view, m_word_under_cursor_pos);
  p.x = static_cast<LONG>(x_pos);
  p.y = static_cast<LONG>(y_pos);
  auto npp = dynamic_cast<NppInterface *>(&m_editor);
  if (npp == nullptr)
    return;
  RECT r;
  auto hwnd = npp->get_scintilla_hwnd(view);
  GetWindowRect(hwnd, &r);

  ClientToScreen(hwnd, &p);

  if (p.y + text_height - 3 + m_settings.suggestion_button_size >= r.bottom)
    {
      if (p.x > m_settings.suggestion_button_size) {
        p.y -= m_settings.suggestion_button_size;
        p.x -= m_settings.suggestion_button_size;
      }
      else
        p.y -= (text_height + m_settings.suggestion_button_size);
    }

  if (p.x + m_settings.suggestion_button_size > r.right)
    p.x -= m_settings.suggestion_button_size;

  if (r.top > p.y + text_height - 3 || r.left > p.x)
    return;

  MoveWindow(suggestion_button.getHSelf(), p.x,
             p.y + static_cast<int>(text_height) - 3,
             m_settings.suggestion_button_size,
             m_settings.suggestion_button_size, 1);
  suggestion_button.display(true, false);
}

std::vector<MenuItem>
ContextMenuHandler::get_suggestion_menu_items() {
  if (!m_speller_container.active_speller().is_working())
    return {}; // Word is already off-screen

  int pos = m_word_under_cursor_pos;
  auto view = m_editor.active_view();
  m_editor.set_selection(view, pos, pos + m_word_under_cursor_length);
  std::vector<MenuItem> suggestion_menu_items;
  auto text = m_editor.get_text_range(
      view, m_word_under_cursor_pos,
      m_word_under_cursor_pos + static_cast<long>(m_word_under_cursor_length));

  m_selected_word =
      SpellCheckerHelpers::to_mapped_wstring(m_editor, view, text.data());
  SpellCheckerHelpers::apply_word_conversions(m_settings, m_selected_word.str);

  m_last_suggestions = m_speller_container.active_speller().get_suggestions(
      m_selected_word.str.c_str());

  for (int i = 0; i < static_cast<int>(m_last_suggestions.size()); i++) {
    if (i >= m_settings.suggestion_count)
      break;

    auto item = m_last_suggestions[i].c_str();
    suggestion_menu_items.emplace_back(item, static_cast<BYTE>(i + 1));
  }

  if (!m_last_suggestions.empty()) {
    MenuItem replace_all_item (L"<Placeholder>", -1);
    replace_all_item.children = suggestion_menu_items;
    std::for_each (replace_all_item.children.begin (), replace_all_item.children.end (), [](auto &item){ item.id += MID_REPLACE_ALL_START;});
    suggestion_menu_items.push_back (std::move (replace_all_item));
    suggestion_menu_items.emplace_back(MenuItem::Separator{});
  }

  SpellCheckerHelpers::apply_word_conversions(m_settings, m_selected_word.str);
  auto menu_string = wstring_printf(rc_str (IDS_IGNORE_PS_FOR_CURRENT_SESSION).c_str (),
                                    m_selected_word.str.c_str());
  suggestion_menu_items.emplace_back(menu_string.c_str(), MID_IGNOREALL);
  menu_string =
      wstring_printf(rc_str (IDS_ADD_PS_TO_DICTIONARY).c_str (), m_selected_word.str.c_str());
  ;
  suggestion_menu_items.emplace_back(menu_string.c_str(),
                                     MID_ADDTODICTIONARY);

  if (m_settings.suggestions_mode == SuggestionMode::context_menu)
    suggestion_menu_items.emplace_back(MenuItem::Separator{});

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
