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
  std::vector<SuggestionsMenuItem> fill_suggestions_menu(HMENU menu);
  void process_menu_result(WPARAM menu_id);
  void precalculate_menu();
  void init_suggestions_box(SuggestionsButton &suggestion_button);

private:
  void do_plugin_menu_inclusion(bool invalidate = false);

private:
  const Settings &m_settings;
  const SpellerContainer &m_speller_container;
  EditorInterface &m_editor;
  MappedWstring m_selected_word;
  long m_word_under_cursor_length = 0;
  long m_word_under_cursor_pos = 0;
  std::vector<std::wstring> m_last_suggestions;
  const SpellChecker &m_spell_checker;
  bool m_word_under_cursor_is_correct = true;
};