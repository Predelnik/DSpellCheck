#include "MainDef.h"
#include "MockEditorInterface.h"
#include "MockSpeller.h"
#include "Settings.h"
#include "SpellChecker.h"
#include "SpellerContainer.h"
#include "catch.hpp"
#include "utils/TemporaryAcessor.h"

void setup_speller(MockSpeller &speller) {
  speller.set_inner_dict({{L"English",
                           {L"This", L"is", L"test", L"document", L"Please",
                            L"bear", L"with", L"me"}}});
}

TEST_CASE("Simple") {
  auto speller = std::make_unique<MockSpeller>();
  setup_speller(*speller);
  Settings settings;
  settings.speller_language[SpellerId::aspell] = L"English";
  MockEditorInterface editor;
  auto v = EditorViewType::primary;
  editor.open_virtual_document(v, L"test.txt", LR"(This is test document.
Please bear with me.
adadsd.)");
  auto speller_ptr = speller.get();
  SpellerContainer sp_container(&settings, std::move(speller));
  SpellChecker sc(&settings, editor, sp_container);
  sc.find_next_mistake();
  CHECK(editor.selected_text(v) == "adadsd");
  settings.modify()->tokenization_style = TokenizationStyle::by_delimiters;
  CHECK(editor.selected_text(v) == "adadsd");
  editor.set_active_document_text(v, LR"(This is test document)");
  sc.find_next_mistake();
  CHECK(editor.selected_text(v).empty());
  editor.set_active_document_text(v, LR"(
wrongword
This is test document
badword)");
  sc.find_next_mistake();
  CHECK(editor.selected_text(v) == "wrongword");
  sc.find_next_mistake();
  CHECK(editor.selected_text(v) == "badword");
  sc.find_prev_mistake();
  CHECK(editor.selected_text(v) == "wrongword");
  sc.find_prev_mistake();
  CHECK(editor.selected_text(v) == "badword");
  CHECK(sc.get_all_misspellings_as_string() == LR"(badword
wrongword
)");
  editor.set_active_document_text(v, LR"(
wrongword
This is test document
badword
Badword)");
  CHECK(sc.get_all_misspellings_as_string() == LR"(badword
wrongword
)");
  editor.make_all_visible(v);
  sc.recheck_visible_both_views(); // technically it should happen automatically
                                   // if notificaitons were signals in
                                   // editorinterface
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id) ==
        std::vector<std::string>{"wrongword", "badword", "Badword"});
  {
    auto mut = settings.modify();
    mut->check_those = false;
    mut->file_types = L"*.txt";
  }
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id).empty());
  {
    auto mut = settings.modify();
    mut->check_those = true;
  }
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id) ==
        std::vector<std::string>{"wrongword", "badword", "Badword"});
  speller_ptr->set_working(false);
  sp_container.speller_status_changed();
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id).empty());

  speller_ptr->set_working(true);
  sp_container.speller_status_changed();

  long pos, length;
  editor.set_cursor_pos(v, 4);
  CHECK_FALSE(sc.is_word_under_cursor_correct(pos, length, true));
  editor.set_cursor_pos(v, 14);
  CHECK(sc.is_word_under_cursor_correct(pos, length, true));

  {
    auto mut = settings.modify();
    mut->word_minimum_length = 8;
  }
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id) ==
        std::vector<std::string>{"wrongword"});
  {
    auto mut = settings.modify();
    mut->word_minimum_length = 0;
    mut->ignore_starting_with_capital = true;
  }
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id) ==
        std::vector<std::string>{"wrongword", "badword"});
  {
    auto mut = settings.modify();
    mut->word_minimum_length = 0;
    mut->ignore_containing_digit = false;
    mut->ignore_having_a_capital = true;
    mut->ignore_all_capital = true;
  }
  editor.set_active_document_text(v, LR"(
  abaas123asd
  wEirdCasE
  SCREAMING
  )");
  sc.recheck_visible_both_views();
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id) ==
        std::vector<std::string>{"abaas123asd"});
  {
    auto mut = settings.modify();
    mut->ignore_starting_with_capital = false;
    mut->ignore_containing_digit = true;
    mut->split_camel_case = true;
    mut->ignore_all_capital = false;
    mut->ignore_having_a_capital = false;
  }
  CHECK(sc.get_all_misspellings_as_string() ==
        LR"(Cas
E
Eird
SCREAMING
w
)");
}