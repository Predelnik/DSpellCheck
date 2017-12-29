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

  SetMenuItemInfo(dspellcheck_menu, quick_lang_change_item_index, TRUE, &mif);
}

void ContextMenuHandler::process_menu_result(WPARAM menu_id) {
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

void ContextMenuHandler::precalculate_menu() {
  std::vector<SuggestionsMenuItem> suggestion_menu_items;
  if (SpellCheckerHelpers::is_spell_checking_needed (m_editor, m_settings) &&
      m_settings.suggestions_mode == SuggestionMode::context_menu) {
    long pos, length;
    m_word_under_cursor_is_correct =
        m_spell_checker.is_word_under_cursor_correct(pos, length, true);
    if (!m_word_under_cursor_is_correct) {
      m_word_under_cursor_pos = pos;
      m_word_under_cursor_length = length;
      suggestion_menu_items = fill_suggestions_menu(nullptr);
    }
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

std::vector<SuggestionsMenuItem>
ContextMenuHandler::fill_suggestions_menu(HMENU menu) {
  if (!m_speller_container.active_speller().is_working())
    return {}; // Word is already off-screen

  int pos = m_word_under_cursor_pos;
  auto view = m_editor.active_view();
  m_editor.set_selection(view, pos, pos + m_word_under_cursor_length);
  std::vector<SuggestionsMenuItem> suggestion_menu_items;
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

  SpellCheckerHelpers::apply_word_conversions(m_settings, m_selected_word.str);
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

ContextMenuHandler::ContextMenuHandler(
    const Settings &settings, const SpellerContainer &speller_container,
    EditorInterface &editor, const SpellChecker &spell_checker)
    : m_settings(settings), m_speller_container(speller_container),
      m_editor(editor), m_spell_checker(spell_checker) {
  m_speller_container.speller_status_changed.connect(
      [this]() { do_plugin_menu_inclusion(); });
}
