#include "MockEditorInterface.h"
#include "MockSpeller.h"
#include "Settings.h"
#include "SpellChecker.h"
#include "SpellerContainer.h"
#include "catch.hpp"

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
  editor.open_virtual_document(EditorViewType::primary, L"test.txt",
                               LR"(This is test document.
Please bear with me.
adadsd.)");
  SpellerContainer container(&settings, std::move(speller));
  SpellChecker sc(&settings, editor, container);
  sc.find_next_mistake();
  CHECK(editor.selected_text(EditorViewType::primary) == "adadsd");
}