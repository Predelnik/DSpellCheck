#include "TestCommon.h"

#include <catch.hpp>

#include "Settings.h"
#include "MockEditorInterface.h"
#include "MockSpeller.h"
#include "SpellerContainer.h"
#include "SpellChecker.h"
#include "MainDefs.h"

TEST_CASE("ANSI") {
  Settings settings;
  auto speller = std::make_unique<MockSpeller>(settings);
  setup_speller(*speller);
  settings.speller_language[SpellerId::aspell] = L"English";
  MockEditorInterface editor;
  auto v = EditorViewType::primary;
  editor.open_virtual_document(v, L"test.txt", L"abcd\nefgh");
  SpellerContainer sp_container(&settings, std::move(speller));
  SpellChecker sc(&settings, editor, sp_container);

  editor.set_codepage(v, EditorCodepage::ansi);
  sc.recheck_visible_both_views();
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id) ==
        std::vector<std::string>{"abcd", "efgh"});
}

TEST_CASE("UTF-8 shenanigans") {
  {
    Settings settings;
    auto speller = std::make_unique<MockSpeller>(settings);
    setup_speller(*speller);
    settings.speller_language[SpellerId::aspell] = L"English";
    MockEditorInterface editor;
    auto v = EditorViewType::primary;
    editor.open_virtual_document(v, L"test.txt", L"тестирование с нетривиальными utf-8 символами");
    SpellerContainer sp_container(&settings, std::move(speller));
    SpellChecker sc(&settings, editor, sp_container);

    CHECK(editor.get_prev_valid_begin_pos(v, 2) == 0);
    CHECK(editor.get_next_valid_end_pos(v, 4) == 6);
    // moving around word 'нетривиальными'
    CHECK(editor.get_prev_valid_begin_pos(v, 28) == 27);
    CHECK(editor.get_next_valid_end_pos(v, 56) == 57);
    // and a bit further
    CHECK(editor.get_prev_valid_begin_pos(v, 27) == 25);
    CHECK(editor.get_next_valid_end_pos(v, 56) == 57);
  }
}
