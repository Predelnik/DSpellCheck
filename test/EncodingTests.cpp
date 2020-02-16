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
  auto v = NppViewType::primary;
  editor.open_virtual_document(v, L"test.txt", L"abcd\nefgh");
  SpellerContainer sp_container(&settings, std::move(speller));
  SpellChecker sc(&settings, editor, sp_container);

  editor.set_codepage(v, EditorCodepage::ansi);
  sc.recheck_visible_both_views();
  CHECK(editor.get_underlined_words(v, spell_check_indicator_id) ==
        std::vector<std::string>{"abcd", "efgh"});
}

TEST_CASE("UTF-8 shenanigans") {
  {
    Settings settings;
    auto speller = std::make_unique<MockSpeller>(settings);
    setup_speller(*speller);
    settings.speller_language[SpellerId::aspell] = L"English";
    MockEditorInterface editor;
    auto v = NppViewType::primary;
    TARGET_VIEW_BLOCK(editor, static_cast<int> (v));
    editor.open_virtual_document(v, L"test.txt", L"тестирование с нетривиальными utf-8 символами");
    SpellerContainer sp_container(&settings, std::move(speller));
    SpellChecker sc(&settings, editor, sp_container);

    CHECK(editor.get_prev_valid_begin_pos(2) == 0);
    CHECK(editor.get_next_valid_end_pos(4) == 6);
    // moving around word 'нетривиальными'
    CHECK(editor.get_prev_valid_begin_pos(28) == 27);
    CHECK(editor.get_next_valid_end_pos(56) == 57);
    // and a bit further
    CHECK(editor.get_prev_valid_begin_pos(27) == 25);
    CHECK(editor.get_next_valid_end_pos(56) == 57);
  }
}
