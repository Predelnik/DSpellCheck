#include <catch.hpp>

#include "TestCommon.h"

#include "Settings.h"
#include "MockEditorInterface.h"
#include "MockSpeller.h"
#include "SpellerContainer.h"
#include "SpellChecker.h"
#include "MainDefs.h"

#include "SciLexer.h"

TEST_CASE("Language Styles") {
  {
    Settings settings;
    auto speller = std::make_unique<MockSpeller>(settings);
    setup_speller(*speller);
    settings.speller_language[SpellerId::aspell] = L"English";
    MockEditorInterface editor;
    auto v = EditorViewType::primary;
    editor.open_virtual_document(v, L"test.txt", L"abcdef test");
    SpellerContainer sp_container(&settings, std::move(speller));
    SpellChecker sc(&settings, editor, sp_container);

    editor.set_lexer(v, SCLEX_HTML);
    editor.set_whole_text_style(v, SCE_H_DEFAULT);
    editor.make_all_visible(v);
    sc.recheck_visible_both_views();
    CHECK(!editor.get_underlined_words(v, spell_check_indicator_id).empty());

    editor.set_lexer(v, SCLEX_YAML);
    editor.set_whole_text_style(v, SCE_YAML_DEFAULT);
    sc.recheck_visible_both_views();
    CHECK(!editor.get_underlined_words(v, spell_check_indicator_id).empty());
  }
}