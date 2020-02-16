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
    auto v = NppViewType::primary;
    TARGET_VIEW_BLOCK (editor, static_cast<int> (v));
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

    {
      auto m = settings.modify();
      m->check_strings = true;
    }

    editor.set_lexer(v, SCLEX_CPP);
    editor.set_whole_text_style(v, SCE_C_VERBATIM);
    sc.recheck_visible_both_views();
    CHECK(!editor.get_underlined_words(v, spell_check_indicator_id).empty());

    editor.set_lexer(v, SCLEX_LUA);
    editor.set_whole_text_style(v, SCE_LUA_LITERALSTRING);
    sc.recheck_visible_both_views();
    CHECK(!editor.get_underlined_words(v, spell_check_indicator_id).empty());
  }
}
