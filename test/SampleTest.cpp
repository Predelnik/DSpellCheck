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
  SpellerContainer container(&settings, std::move(speller));
  SpellChecker sc(&settings, editor, container);
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
  editor.make_all_visible (v);
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
}