/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "AspellInterface.h"
#include "HunspellInterface.h"
#include "NativeSpellerInterface.h"
#include "SpellerInterface.h"

#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "LanguageInfo.h"
#include "MainDef.h"
#include "Plugin.h"
#include "PluginInterface.h"
#include "SciUtils.h"
#include "Scintilla.h"
#include "Settings.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "SpellerContainer.h"
#include "SpellerId.h"
#include "SuggestionsButton.h"
#include "npp/EditorInterface.h"
#include "npp/NppInterface.h"
#include "utils/string_utils.h"

SpellChecker::SpellChecker(const Settings *settings, EditorInterface &editor,
                           SpellerContainer &speller_container)
    : m_settings(*settings), m_editor(editor),
      m_speller_container(speller_container) {
  m_current_position = 0;
  m_word_under_cursor_length = 0;
  m_word_under_cursor_pos = 0;
  m_word_under_cursor_is_correct = true;
  reset_hot_spot_cache();
  m_settings.settings_changed.connect([this] { on_settings_changed(); });
  auto npp = dynamic_cast<NppInterface *>(&m_editor);
  if (npp == nullptr)
    return;

  bool res = npp->is_allocate_cmdid_supported();

  if (res) {
    set_use_allocated_ids(true);
    auto id = npp->allocate_cmdid(350);
    set_context_menu_id_start(id);
    set_langs_menu_id_start(id + 103);
  }
}

SpellChecker::~SpellChecker() = default;

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

void SpellChecker::precalculate_menu() {
  std::vector<SuggestionsMenuItem> suggestion_menu_items;
  if (check_text_needed() &&
      m_settings.suggestions_mode == SuggestionMode::context_menu) {
    long pos, length;
    m_word_under_cursor_is_correct =
        is_word_under_cursor_correct(pos, length, true);
    if (!m_word_under_cursor_is_correct) {
      m_word_under_cursor_pos = pos;
      m_word_under_cursor_length = length;
      suggestion_menu_items = fill_suggestions_menu(nullptr);
    }
  }
  show_calculated_menu(std::move(suggestion_menu_items));
}

void SpellChecker::recheck_visible_both_views() {
  recheck_visible(EditorViewType::primary);
  recheck_visible(EditorViewType::secondary);
}

void SpellChecker::lang_change() { recheck_visible(m_editor.active_view()); }

void SpellChecker::do_plugin_menu_inclusion(bool invalidate) {
  MENUITEMINFO mif;
  HMENU dspellcheck_menu = get_dspellcheck_menu();
  if (dspellcheck_menu == nullptr)
    return;
  HMENU langs_sub_menu = get_langs_sub_menu(dspellcheck_menu);
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
            m_settings.use_language_name_aliases ? lang.alias_name.c_str()
                                                 : lang.orig_name.c_str());
        if (!res)
          return;
        ++i;
      }
      int checked = (cur_lang == L"<MULTIPLE>") ? (MFT_RADIOCHECK | MF_CHECKED)
                                                : MF_UNCHECKED;
      AppendMenu(new_menu, MF_STRING | checked,
                 get_use_allocated_ids()
                     ? MULTIPLE_LANGS + get_langs_menu_id_start()
                     : MAKEWORD(MULTIPLE_LANGS, LANGUAGE_MENU_ID),
                 L"Multiple Languages");
      AppendMenu(new_menu, MF_SEPARATOR, 0, nullptr);
      AppendMenu(new_menu, MF_STRING,
                 get_use_allocated_ids()
                     ? CUSTOMIZE_MULTIPLE_DICS + get_langs_menu_id_start()
                     : MAKEWORD(CUSTOMIZE_MULTIPLE_DICS, LANGUAGE_MENU_ID),
                 L"Set Multiple Languages...");
      if (m_settings.active_speller_lib_id ==
          SpellerId::hunspell) // Only Hunspell supported
      {
        AppendMenu(new_menu, MF_STRING,
                   get_use_allocated_ids()
                       ? DOWNLOAD_DICS + get_langs_menu_id_start()
                       : MAKEWORD(DOWNLOAD_DICS, LANGUAGE_MENU_ID),
                   L"Download More Languages...");
        AppendMenu(new_menu, MF_STRING,
                   get_use_allocated_ids()
                       ? REMOVE_DICS + get_langs_menu_id_start()
                       : MAKEWORD(REMOVE_DICS, LANGUAGE_MENU_ID),
                   L"Remove Unneeded Languages...");
      }
    } else if (m_settings.active_speller_lib_id == SpellerId::hunspell)
      AppendMenu(new_menu, MF_STRING,
                 get_use_allocated_ids()
                     ? DOWNLOAD_DICS + get_langs_menu_id_start()
                     : MAKEWORD(DOWNLOAD_DICS, LANGUAGE_MENU_ID),
                 L"Download Languages...");
  }

  mif.fMask = MIIM_SUBMENU | MIIM_STATE;
  mif.cbSize = sizeof(MENUITEMINFO);
  mif.hSubMenu = new_menu;
  mif.fState = MFS_ENABLED;

  SetMenuItemInfo(dspellcheck_menu, QUICK_LANG_CHANGE_ITEM, 1, &mif);
}

void SpellChecker::check_file_name() {
  m_check_text_enabled = !m_settings.check_those;
  auto full_path = m_editor.get_full_current_path();
  for (auto token : make_delimiter_tokenizer(m_settings.file_types, LR"(;)")
                        .get_all_tokens()) {
    if (m_settings.check_those) {
      m_check_text_enabled =
          m_check_text_enabled ||
          PathMatchSpec(full_path.c_str(), std::wstring(token).c_str());
      if (m_check_text_enabled)
        break;
    } else {
      m_check_text_enabled &=
          m_check_text_enabled &&
          (!PathMatchSpec(full_path.c_str(), std::wstring(token).c_str()));
      if (!m_check_text_enabled)
        break;
    }
  }
}

bool SpellChecker::check_text_needed() {
  return m_check_text_enabled && m_settings.auto_check_text;
}

void SpellChecker::find_next_mistake() {
  auto view = m_editor.active_view();
  m_current_position = m_editor.get_current_pos(view);
  auto cur_line = m_editor.line_from_position(view, m_current_position);
  auto line_start_pos = m_editor.get_line_start_position(view, cur_line);
  auto doc_length = m_editor.get_active_document_length(view);
  auto iterator_pos = line_start_pos;
  bool full_check = false;

  while (true) {
    auto from = static_cast<long>(iterator_pos);
    auto to = static_cast<long>(iterator_pos +
                                m_settings.find_next_buffer_size * 1024);
    int ignore_offsetting = 0;
    if (to > doc_length) {
      ignore_offsetting = 1;
      to = static_cast<long>(doc_length);
    }
    if (from < to) {
      auto text = to_mapped_wstring(
          view, m_editor.get_text_range(view, from, to).c_str());
      auto index =
          prev_token_begin(text.str, static_cast<long>(text.str.size()) - 1)
              .value_or(text.str.size() - 1);
      text.str.erase(index, text.str.size() - index);
      m_editor.force_style_update(view, from, to);
      SCNotification scn;
      scn.nmhdr.code = SCN_SCROLLED;
      bool result = check_text(view, text, static_cast<long>(iterator_pos),
                               CheckTextMode::find_first) != 0;
      if (result)
        break;

      iterator_pos += text.to_original_index(index);
    }

    if (to == doc_length) {
      if (!full_check) {
        m_current_position = 0;
        iterator_pos = 0;
        full_check = true;
      } else
        break;

      if (full_check && iterator_pos > m_current_position)
        break; // So nothing was found TODO: Message probably
    }
  }
}

void SpellChecker::find_prev_mistake() {
  auto view = m_editor.active_view();
  m_current_position = m_editor.get_current_pos(view);
  auto cur_line = m_editor.line_from_position(view, m_current_position);
  auto doc_length = m_editor.get_active_document_length(view);
  auto line_end_pos = m_editor.get_line_end_position(view, cur_line);

  auto iterator_pos = line_end_pos;
  bool full_check = false;

  while (true) {
    auto from = static_cast<long>(iterator_pos -
                                  m_settings.find_next_buffer_size * 1024);
    auto to = static_cast<long>(iterator_pos);
    int ignore_offsetting = 0;
    if (from < 0) {
      from = 0;
      ignore_offsetting = 1;
    }

    if (from < to) {
      auto text =
          to_mapped_wstring(view, m_editor.get_text_range(view, from, to));
      auto offset = next_token_end(text.str, 0).value_or(0);
      m_editor.force_style_update(view, from + offset, to);
      SCNotification scn;
      scn.nmhdr.code = SCN_SCROLLED;
      bool result = check_text(view, text, from, CheckTextMode::find_last) != 0;
      if (result)
        break;

      iterator_pos -= (m_settings.find_next_buffer_size * 1024 -
                       text.to_original_index(offset));
    } else
      --iterator_pos;

    if (iterator_pos < 0) {
      if (!full_check) {
        m_current_position = doc_length + 1;
        iterator_pos = doc_length;
        full_check = true;
      } else
        break;

      if (full_check && iterator_pos < m_current_position - 1)
        break; // So nothing was found TODO: Message probably
    }
  }
}

WordForSpeller SpellChecker::to_word_for_speller(std::wstring_view word) const {
  WordForSpeller res;
  res.data.ends_with_dot = *(word.data() + word.length()) == '.';
  res.str = {word};
  apply_conversions(res.str);
  return res;
}

bool SpellChecker::is_word_under_cursor_correct(long &pos, long &length,
                                                bool use_text_cursor) {
  POINT p;
  long init_char_pos;
  LRESULT selection_start = 0;
  LRESULT selection_end = 0;
  auto view = m_editor.active_view();

  if (!use_text_cursor) {
    if (GetCursorPos(&p) == 0)
      return true;

    auto mb_pos = m_editor.char_position_from_point(view, p.x, p.y);
    if (!mb_pos)
      return true;
    init_char_pos = *mb_pos;
  } else {
    selection_start = m_editor.get_selection_start(view);
    selection_end = m_editor.get_selection_end(view);
    init_char_pos = m_editor.get_current_pos(view);
  }

  if (init_char_pos == -1)
    return true;
  auto line = m_editor.line_from_position(view, init_char_pos);
  auto offset = m_editor.get_line_start_position(view, line);
  auto mapped_str =
      to_mapped_wstring(view, m_editor.get_line(view, line).data());
  if (mapped_str.str.empty())
    return true;
  auto word = get_word_at(static_cast<long>(init_char_pos), mapped_str,
                          static_cast<long>(offset));
  if (word.empty())
    return true;
  cut_apostrophes(word);
  pos = static_cast<long>(mapped_str.to_original_index(static_cast<long>(
                              word.data() - mapped_str.str.data())) +
                          offset);
  long pos_end = mapped_str.to_original_index(static_cast<long>(
                     word.data() + word.length() - mapped_str.str.data())) +
                 offset;
  long word_len = pos_end - pos;
  if (selection_start != selection_end &&
      (selection_start != pos || selection_end != pos + word_len))
    return true;
  if (check_word(view, word, pos)) {
    return true;
  }
  length = word_len;
  return false;
}

std::wstring_view SpellChecker::get_word_at(long char_pos,
                                            const MappedWstring &text,
                                            long offset) const {
  auto index = text.from_original_index(char_pos - offset);
  if (index >= static_cast<long>(text.str.length()))
    index = static_cast<long>(text.str.length()) - 1;
  auto begin = prev_token_begin(text.str, index);
  if (!begin)
    return {};
  auto end = next_token_end(text.str, *begin);
  if (!end)
    return {};
  return std::wstring_view(text.str).substr(*begin, *end - *begin);
}

void SpellChecker::init_suggestions_box(SuggestionsButton &suggestion_button) {
  if (m_settings.suggestions_mode != SuggestionMode::button)
    return;
  if (!m_speller_container.active_speller().is_working())
    return;
  POINT p;
  if (!check_text_needed()) // If there's no red underline let's do nothing
  {
    suggestion_button.display(false);
    return;
  }

  long pos, length;
  if (is_word_under_cursor_correct(pos, length)) {
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
  if (r.top > p.y + text_height - 3 || r.left > p.x ||
      r.bottom < p.y + text_height - 3 + m_settings.suggestion_button_size ||
      r.right < p.x + m_settings.suggestion_button_size)
    return;
  MoveWindow(suggestion_button.getHSelf(), p.x,
             p.y + static_cast<int>(text_height) - 3,
             m_settings.suggestion_button_size,
             m_settings.suggestion_button_size, 1);
  suggestion_button.display(true, false);
}

void SpellChecker::process_menu_result(WPARAM menu_id)  {
  if ((!get_use_allocated_ids() && HIBYTE(menu_id) != DSPELLCHECK_MENU_ID &&
       HIBYTE(menu_id) != LANGUAGE_MENU_ID) ||
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
        apply_conversions(m_selected_word.str);
        m_speller_container.active_speller().ignore_all(
            m_selected_word.str.c_str());
        m_word_under_cursor_length =
            static_cast<long>(m_selected_word.str.length());
        m_editor.set_cursor_pos(view, m_word_under_cursor_pos +
                                          m_word_under_cursor_length);
        recheck_visible_both_views();
      } else if (result == MID_ADDTODICTIONARY) {
        apply_conversions(m_selected_word.str);
        m_speller_container.active_speller().add_to_dictionary(
            m_selected_word.str.c_str());
        m_word_under_cursor_length =
            static_cast<long>(m_selected_word.str.length());
        m_editor.set_cursor_pos(view, m_word_under_cursor_pos +
                                          m_word_under_cursor_length);
        recheck_visible_both_views();
      } else if (result <= m_last_suggestions.size()) {
        std::string encoded_str;
        if (m_editor.get_encoding(view) == EditorCodepage::ansi)
          encoded_str = to_string(m_last_suggestions[result - 1].c_str());
        else
          encoded_str = to_utf8_string(m_last_suggestions[result - 1]);

        m_editor.replace_selection(view, encoded_str.c_str());
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
      lang_string = L"<MULTIPLE>";
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

std::vector<SuggestionsMenuItem>
SpellChecker::fill_suggestions_menu(HMENU menu) {
  if (!m_speller_container.active_speller().is_working())
    return {}; // Word is already off-screen

  int pos = m_word_under_cursor_pos;
  auto view = m_editor.active_view();
  m_editor.set_selection(view, pos, pos + m_word_under_cursor_length);
  std::vector<SuggestionsMenuItem> suggestion_menu_items;
  auto text = m_editor.get_text_range( 
      view, m_word_under_cursor_pos,
      m_word_under_cursor_pos + static_cast<long>(m_word_under_cursor_length));

  m_selected_word = to_mapped_wstring(view, text.data());
  apply_conversions(m_selected_word.str);

  m_last_suggestions = m_speller_container.active_speller().get_suggestions(
      m_selected_word.str.c_str());

  for (int i = 0; i < static_cast<int>(m_last_suggestions.size()); i++) {
    if (i >= m_settings.suggestion_count)
      break;

    auto item = m_last_suggestions[i].c_str();
    if (m_settings.suggestions_mode == SuggestionMode::button)
      insert_sugg_menu_item(menu, item, static_cast<BYTE>(i + 1), -1);
    else
      suggestion_menu_items.emplace_back(item, static_cast<BYTE>(i + 1));
  }

  if (!m_last_suggestions.empty()) {
    if (m_settings.suggestions_mode == SuggestionMode::button)
      insert_sugg_menu_item(menu, L"", 0, 103, true);
    else
      suggestion_menu_items.emplace_back(L"", 0, true);
  }

  apply_conversions(m_selected_word.str);
  auto menu_string = wstring_printf(L"Ignore \"%s\" for Current Session",
                                    m_selected_word.str.c_str());
  if (m_settings.suggestions_mode == SuggestionMode::button)
    insert_sugg_menu_item(menu, menu_string.c_str(), MID_IGNOREALL, -1);
  else
    suggestion_menu_items.emplace_back(menu_string.c_str(), MID_IGNOREALL);
  menu_string =
      wstring_printf(L"Add \"%s\" to Dictionary", m_selected_word.str.c_str());
  ;
  if (m_settings.suggestions_mode == SuggestionMode::button)
    insert_sugg_menu_item(menu, menu_string.c_str(), MID_ADDTODICTIONARY, -1);
  else
    suggestion_menu_items.emplace_back(menu_string.c_str(),
                                       MID_ADDTODICTIONARY);

  if (m_settings.suggestions_mode == SuggestionMode::context_menu)
    suggestion_menu_items.emplace_back(L"", 0, true);

  return suggestion_menu_items;
}

void SpellChecker::refresh_underline_style() {
  for (auto view : enum_range<EditorViewType>()) {
    m_editor.set_indicator_style(view, SCE_ERROR_UNDERLINE,
                                 m_settings.underline_style);
    m_editor.set_indicator_foreground(view, SCE_ERROR_UNDERLINE,
                                      m_settings.underline_color);
  }
}

void SpellChecker::on_settings_changed() {
  {
    auto npp = dynamic_cast<NppInterface *>(&m_editor);
    if (npp != nullptr)
      npp->set_menu_item_check(get_func_item()[0].cmd_id,
                               m_settings.auto_check_text);
  }
  refresh_underline_style();
  check_file_name();
  refresh_underline_style();
  recheck_visible_both_views();
  do_plugin_menu_inclusion();
  update_delimiters();
  get_download_dics()->update_list_box();
}

void SpellChecker::create_word_underline(EditorViewType view, long start,
                                         long end) {
  m_editor.set_current_indicator(view, SCE_ERROR_UNDERLINE);
  m_editor.indicator_fill_range(view, start, end);
}

void SpellChecker::remove_underline(EditorViewType view, long start, long end) {
  if (end < start)
    return;
  m_editor.set_current_indicator(view, SCE_ERROR_UNDERLINE);
  m_editor.indicator_clear_range(view, start, end);
}

void SpellChecker::get_visible_limits(EditorViewType view, long &start,
                                      long &finish) {
  auto top = m_editor.get_first_visible_line(view);
  auto bottom = top + m_editor.get_lines_on_screen(view);
  top = m_editor.get_document_line_from_visible(view, top);
  bottom = m_editor.get_document_line_from_visible(view, bottom);
  auto line_count = m_editor.get_document_line_count(view);
  start = m_editor.get_line_start_position(view, top);
  // Not using end of line position cause utf-8 symbols could be more than one
  // char
  // So we use next line start as the end of our visible text
  if (bottom + 1 < line_count) {
    finish = m_editor.get_line_start_position(view, bottom + 1);
  } else {
    finish = m_editor.get_active_document_length(view);
  }
}

MappedWstring SpellChecker::get_visible_text(EditorViewType view, long *offset,
                                             bool /*not_intersection_only*/) {
  long from, to;
  get_visible_limits(view, from, to);

  if (to < 0 || from > to)
    return {};

  *offset = from;
  return to_mapped_wstring(view, m_editor.get_text_range(view, from, to));
}

void SpellChecker::add_periods(const std::wstring_view &parent_string_view,
                               std::wstring_view &target) {
  ptrdiff_t start_offset = target.data() - parent_string_view.data();
  ptrdiff_t end_offset = start_offset + target.length();
  while (end_offset < static_cast<ptrdiff_t>(parent_string_view.length()) &&
         parent_string_view[end_offset] == '.')
    ++end_offset;
  target = parent_string_view.substr(start_offset, end_offset - start_offset);
}

void SpellChecker::clear_all_underlines(EditorViewType view) {
  auto length = m_editor.get_active_document_length(view);
  if (length > 0) {
    m_editor.set_current_indicator(view, SCE_ERROR_UNDERLINE);
    m_editor.indicator_clear_range(view, 0, length - 1);
  }
}

void SpellChecker::apply_conversions(std::wstring &word) const {
  for (auto &c : word) {
    if (m_settings.ignore_yo) {
      if (c == L'ё')
        c = L'е';
      if (c == L'Ё')
        c = L'Е';
    }
    if (m_settings.convert_single_quotes) {
      if (c == L'’')
        c = L'\'';
    }
  }
}

void SpellChecker::reset_hot_spot_cache() {
  memset(m_hot_spot_cache, -1, sizeof(m_hot_spot_cache));
}

bool SpellChecker::is_spellchecking_needed(EditorViewType view,
                                           std::wstring_view word,
                                           long word_start) {
  if (!m_speller_container.active_speller().is_working() || word.empty()) {
    return false;
  }
  // Well Numbers have same codes for ANSI and Unicode I guess, so
  // If word contains number then it's probably just a number or some crazy name
  auto style = m_editor.get_style_at(view, word_start);
  auto lexer = m_editor.get_lexer(view);
  auto category = SciUtils::get_style_category(lexer, style);
  if (category == SciUtils::StyleCategory::unknown) {
    return false;
  }

  if (category != SciUtils::StyleCategory::text &&
      !((category == SciUtils::StyleCategory::comment &&
         m_settings.check_comments) ||
        (category == SciUtils::StyleCategory::string &&
         m_settings.check_strings) ||
        (category == SciUtils::StyleCategory::idenitifier &&
         m_settings.check_variable_functions))) {
    return false;
  }

  if (m_editor.is_style_hotspot(view, style)) {
    return false;
  }

  if (m_settings.ignore_one_letter && word.length() == 1)
    return false;

  if (m_settings.ignore_containing_digit &&
      word.find_first_of(L"0123456789") != std::wstring_view::npos) {
    return false;
  }

  if (m_settings.ignore_starting_with_capital && IsCharUpper(word.front())) {
    return false;
  }

  if (m_settings.ignore_having_a_capital || m_settings.ignore_all_capital) {
    if (m_settings.ignore_having_a_capital || m_settings.ignore_all_capital) {
      bool all_upper = IsCharUpper(word.front()), any_upper = false;
      for (auto c : std::wstring_view(word).substr(1)) {
        if (IsCharUpper(c)) {
          any_upper = true;
        } else
          all_upper = false;
      }

      if (!all_upper && any_upper && m_settings.ignore_having_a_capital)
        return false;

      if (all_upper && m_settings.ignore_all_capital)
        return false;
    }
  }

  if (m_settings.ignore_having_underscore &&
      word.find(L'_') != std::wstring_view::npos)
    return false;

  if (m_settings.ignore_starting_or_ending_with_apostrophe) {
    if (word.front() == '\'' || word.back() == '\'')
      return false;
  }

  return true;
}

bool SpellChecker::check_word(EditorViewType view, std::wstring_view word,
                              long word_start) {
  if (!is_spellchecking_needed(view, word, word_start))
    return true;

  return m_speller_container.active_speller().check_word(
      to_word_for_speller(word));
}

void SpellChecker::cut_apostrophes(std::wstring_view &word) {
  if (m_settings.remove_boundary_apostrophes) {
    while (!word.empty() && word.front() == L'\'')
      word.remove_prefix(1);

    while (!word.empty() && word.back() == L'\'')
      word.remove_suffix(1);
  }
}

auto SpellChecker::non_alphabetic_tokenizer(std::wstring_view target) const {
  return make_condition_tokenizer(target,
                                  [this](wchar_t c) {
                                    return !IsCharAlphaNumeric(c) &&
                                           m_settings.delimiter_exclusions.find(
                                               c) == std ::wstring_view::npos;
                                  },
                                  m_settings.split_camel_case);
}

auto SpellChecker::delimiter_tokenizer(std::wstring_view target) const {
  return make_delimiter_tokenizer(target, m_settings.delimiters,
                                  m_settings.split_camel_case);
}

std::optional<long> SpellChecker::next_token_end(std::wstring_view target,
                                                 long index) const {
  switch (m_settings.tokenization_style) {
  case TokenizationStyle::by_non_alphabetic:
    return non_alphabetic_tokenizer(target).next_token_end(index);
  case TokenizationStyle::by_delimiters:
    return delimiter_tokenizer(target).next_token_end(index);
  case TokenizationStyle::COUNT:
    break;
  }
  assert(false);
  return std::nullopt;
}

std::optional<long> SpellChecker::prev_token_begin(std::wstring_view target,
                                                   long index) const {
  switch (m_settings.tokenization_style) {
  case TokenizationStyle::by_non_alphabetic:
    return non_alphabetic_tokenizer(target).prev_token_begin(index);
  case TokenizationStyle::by_delimiters:
    return delimiter_tokenizer(target).prev_token_begin(index);
  case TokenizationStyle::COUNT:
    break;
  }
  assert(false);
  return std::nullopt;
}

namespace {
struct WordData {
  std::wstring_view token;
  WordForSpeller word_for_speller;
  long word_start;
  long word_end;
  bool is_correct;
};
} // namespace

int SpellChecker::check_text(EditorViewType view,
                             const MappedWstring &text_to_check, long offset,
                             CheckTextMode mode) {
  if (text_to_check.str.empty())
    return 0;

  bool stop = false;
  long resulting_word_end = -1, resulting_word_start = -1;
  auto text_len = text_to_check.mapping.back();
  std::vector<long> underline_buffer;

  auto sv = std::wstring_view(text_to_check.str);
  std::vector<std::wstring_view> tokens;
  switch (m_settings.tokenization_style) {
  case TokenizationStyle::by_non_alphabetic:
    tokens = non_alphabetic_tokenizer(sv).get_all_tokens();
    break;
  case TokenizationStyle::by_delimiters:
    tokens = delimiter_tokenizer(sv).get_all_tokens();
    break;
  case TokenizationStyle::COUNT:
    break;
  }

  std::vector<bool> results(tokens.size());
  std::vector<WordData> words_to_check;
  words_to_check.clear();
  std::vector<WordForSpeller> words_for_speller;
  for (auto token : tokens) {
    cut_apostrophes(token);
    auto word_start = static_cast<long>(
        offset + text_to_check.to_original_index(static_cast<long>(
                     token.data() - text_to_check.str.data())));
    auto word_end = static_cast<long>(
        offset +
        text_to_check.to_original_index(static_cast<long>(
            token.data() - text_to_check.str.data() + token.length())));
    if (is_spellchecking_needed(view, token, word_start)) {
      words_to_check.emplace_back();
      auto &w = words_to_check.back();
      w.word_for_speller = to_word_for_speller(token);
      w.word_start = word_start;
      w.word_end = word_end;
      w.token = token;
    }
  }
  words_for_speller.resize(words_to_check.size());
  std::transform(words_to_check.begin(), words_to_check.end(),
                 words_for_speller.begin(), [](auto &word) -> auto && {
                   return std::move(word.word_for_speller);
                 });
  auto spellcheck_result =
      m_speller_container.active_speller().check_words(words_for_speller);
  if (!spellcheck_result.empty()) {
    for (int i = 0; i < static_cast<int>(words_for_speller.size()); ++i)
      words_to_check[i].is_correct = spellcheck_result[i];
  } else
    for (auto &w : words_to_check)
      w.is_correct = true;

  for (auto &result : words_to_check) {
    if (!result.is_correct) {
      auto word_start = result.word_start;
      auto word_end = result.word_end;
      switch (mode) {
      case CheckTextMode::underline_errors:
        underline_buffer.push_back(word_start);
        underline_buffer.push_back(word_end);
        break;
      case CheckTextMode::find_first:
        if (word_end > m_current_position) {
          m_editor.set_selection(view, word_start, word_end);
          stop = true;
        }
        break;
      case CheckTextMode::find_last: {
        if (word_end >= m_current_position) {
          stop = true;
          break;
        }
        resulting_word_start = word_start;
        resulting_word_end = word_end;
      } break;
      case CheckTextMode::find_all:
        m_misspellings.push_back(result.token);
        break;
      }
      if (stop)
        break;
    }
  }

  if (mode == CheckTextMode::underline_errors) {
    long prev_pos = offset;
    for (long i = 0; i < static_cast<long>(underline_buffer.size()) - 1;
         i += 2) {
      remove_underline(view, prev_pos, underline_buffer[i] - 1);
      create_word_underline(view, underline_buffer[i],
                            underline_buffer[i + 1] - 1);
      prev_pos = underline_buffer[i + 1];
    }
    remove_underline(view, prev_pos, offset + static_cast<long>(text_len) - 1);
  }
  switch (mode) {
  case CheckTextMode::underline_errors:
    return 1;
  case CheckTextMode::find_first:
    return static_cast<int>(stop);
  case CheckTextMode::find_all:
    return 1;
  case CheckTextMode::find_last:
    if (resulting_word_start == -1)
      return 0;
    else {
      m_editor.set_selection(view, resulting_word_start, resulting_word_end);
      return 1;
    }
  };
  return 0;
}

void SpellChecker::check_visible(EditorViewType view,
                                 bool not_intersection_only) {
  long offset;
  auto text = get_visible_text(view, &offset, not_intersection_only);
  check_text(view, text, offset, CheckTextMode::underline_errors);
}

MappedWstring SpellChecker::to_mapped_wstring(EditorViewType view,
                                              std::string_view str) const {
  if (m_editor.get_encoding(view) == EditorCodepage::utf8)
    return utf8_to_mapped_wstring(str);

  return ::to_mapped_wstring(str);
}

void SpellChecker::recheck_visible(EditorViewType view,
                                   bool not_intersection_only) {
  if (!m_speller_container.active_speller().is_working()) {
    clear_all_underlines(view);
    return;
  }

  // to utf-8 or no
  if (check_text_needed())
    check_visible(view, not_intersection_only);
  else
    clear_all_underlines(view);
}

void SpellChecker::copy_misspellings_to_clipboard() {
  auto view = m_editor.active_view();
  auto buf = m_editor.get_active_document_text(view);
  auto mapped_str = to_mapped_wstring(view, buf.data());
  m_misspellings.clear();
  check_text(view, mapped_str, 0, CheckTextMode::find_all);
  std::sort(m_misspellings.begin(), m_misspellings.end());
  auto it = std::unique(m_misspellings.begin(), m_misspellings.end());
  m_misspellings.erase(it, m_misspellings.end());
  std::wstring str;
  for (auto &s : m_misspellings)
    str += std::wstring{s} + L'\n';
  m_misspellings.clear();
  const size_t len = (str.length() + 1) * 2;
  HGLOBAL h_mem = GlobalAlloc(GMEM_MOVEABLE, len);
  memcpy(GlobalLock(h_mem), str.c_str(), len);
  GlobalUnlock(h_mem);
  OpenClipboard(nullptr);
  EmptyClipboard();
  SetClipboardData(CF_UNICODETEXT, h_mem);
  CloseClipboard();
}

void SpellChecker::update_delimiters() {
  m_delimiters = L" \n\r\t\v" + parse_string(m_settings.delimiters.c_str());
}

SuggestionsMenuItem::SuggestionsMenuItem(const wchar_t *text_arg, int id_arg,
                                         bool separator_arg /*= false*/) {
  text = text_arg;
  id = static_cast<BYTE>(id_arg);
  separator = separator_arg;
}
