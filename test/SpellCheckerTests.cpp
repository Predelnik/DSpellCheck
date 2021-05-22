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

#include "MockEditorInterface.h"
#include "MockSpeller.h"
#include "TestCommon.h"
#include "core/SpellChecker.h"
#include "core/SpellCheckerHelpers.h"
#include "plugin/Constants.h"
#include "plugin/Settings.h"
#include "spellers/SpellerContainer.h"

#include <catch.hpp>

using namespace std::literals;
constexpr auto indicator_id = spell_check_indicator_id;

TEST_CASE("Speller") {
  Settings settings;
  settings.data.speller_language[SpellerId::aspell] = L"English";
  MockEditorInterface editor;
  TARGET_VIEW_BLOCK(editor, 0);
  editor.open_virtual_document(L"test.txt", LR"(This is test document.
Please bear with me.
adadsd.)");
  auto speller = std::make_unique<MockSpeller>(settings);
  setup_speller(*speller);
  auto speller_ptr = speller.get();
  SpellerContainer sp_container(&settings, std::move(speller));
  SpellChecker sc(&settings, editor, sp_container);

  SECTION("Find/prev next mistake / Underlines") {
    sc.find_next_mistake();
    CHECK(editor.selected_text() == "adadsd");
    settings.modify()->data.tokenization_style = TokenizationStyle::by_delimiters;
    CHECK(editor.selected_text() == "adadsd");
    editor.set_active_document_text(LR"(This is test document)");
    sc.find_next_mistake();
    CHECK(editor.selected_text().empty());
    sc.find_prev_mistake();
    CHECK(editor.selected_text().empty());
    std::wstring s;
    for (int i = 0; i < 5000; ++i) {
      if (i == 3333)
        s += L"adadsd\n";
      if (i == 4888)
        s += L"abirvalg\n";
      s += L"This is test document\n";
    }
    editor.set_active_document_text(s);
    sc.find_next_mistake();
    CHECK(editor.selected_text() == "adadsd");
    sc.find_next_mistake();
    CHECK(editor.selected_text() == "abirvalg");
    sc.find_prev_mistake();
    CHECK(editor.selected_text() == "adadsd");

    editor.set_active_document_text(LR"(
wrongword
This is test document
badword)");
    sc.find_next_mistake();
    CHECK(editor.selected_text() == "wrongword");
    sc.find_next_mistake();
    CHECK(editor.selected_text() == "badword");
    sc.find_prev_mistake();
    CHECK(editor.selected_text() == "wrongword");
    sc.find_prev_mistake();
    CHECK(editor.selected_text() == "badword");
    CHECK(sc.get_all_misspellings_as_string() == LR"(badword
wrongword
)");
    sc.erase_all_misspellings();
    CHECK(editor.get_active_document_text() == R"(

This is test document
)");
    editor.set_active_document_text(LR"(
нехорошееслово
И ещё немного слов
ошибочноеслово)");
    sc.erase_all_misspellings();
    CHECK(editor.get_active_document_text() == R"(

И ещё немного слов
)");
    editor.undo(); // check that undo restore both removed words
    CHECK(editor.get_active_document_text() == R"(
нехорошееслово
И ещё немного слов
ошибочноеслово)");
    editor.set_active_document_text(LR"(
wrongword
This is test document
badword
Badword)");
    CHECK(sc.get_all_misspellings_as_string() == LR"(badword
wrongword
)");
    editor.make_all_visible();
    sc.recheck_visible_both_views(); // technically it should happen automatically
    // if notifications were signals in
    // EditorInterface
    CHECK(editor.get_underlined_words(indicator_id) ==
          std::vector<std::string>{"wrongword", "badword", "Badword"});
    {
      auto mut = settings.modify();
      mut->data.check_those = false;
      mut->data.file_types = L"*.txt";
    }
    CHECK(editor.get_underlined_words(indicator_id).empty());
    {
      auto mut = settings.modify();
      mut->data.check_those = true;
    }
    CHECK(editor.get_underlined_words(indicator_id) ==
          std::vector<std::string>{"wrongword", "badword", "Badword"});
    speller_ptr->set_working(false);
    sp_container.speller_status_changed();
    CHECK(editor.get_underlined_words(indicator_id).empty());

    speller_ptr->set_working(true);
    sp_container.speller_status_changed();

    TextPosition pos, length;
    editor.set_cursor_pos(4);
    CHECK_FALSE(sc.is_word_under_cursor_correct(pos, length, true));
    speller_ptr->set_working(false);
    CHECK(sc.is_word_under_cursor_correct(pos, length, true));
    speller_ptr->set_working(true);
    editor.set_cursor_pos(14);
    CHECK(sc.is_word_under_cursor_correct(pos, length, true));

    {
      auto mut = settings.modify();
      mut->data.word_minimum_length = 8;
    }
    CHECK(editor.get_underlined_words(indicator_id) ==
        std::vector<std::string>{"wrongword"});
    {
      auto mut = settings.modify();
      mut->data.word_minimum_length = 0;
      mut->data.ignore_starting_with_capital = true;
    }
    CHECK(editor.get_underlined_words(indicator_id) ==
          std::vector<std::string>{"wrongword", "badword"});

    editor.set_active_document_text(LR"(test test test wrongword test test test)");
    sc.recheck_visible_on_active_view();
    CHECK(editor.get_underlined_words(indicator_id) == std::vector ({"wrongword"s}));

    {
      auto mut = settings.modify();
      mut->data.word_minimum_length = 0;
      mut->data.ignore_containing_digit = false;
      mut->data.ignore_having_a_capital = true;
      mut->data.ignore_all_capital = true;
    }
    editor.set_active_document_text(LR"(
  abaas123asd
  wEirdCasE
  ALLCAPITAL
  )");
    sc.recheck_visible_both_views();
    CHECK(editor.get_underlined_words(indicator_id) ==
        std::vector<std::string>{"abaas123asd"});
    {
      auto mut = settings.modify();
      mut->data.ignore_starting_with_capital = false;
      mut->data.ignore_containing_digit = true;
      mut->data.split_camel_case = true;
      mut->data.ignore_having_a_capital = false;
    }
    CHECK(sc.get_all_misspellings_as_string() == LR"(Cas
Eird
w
)");
    {
      auto mut = settings.modify();
      mut->data.ignore_one_letter = true;
      mut->data.ignore_all_capital = true;
      mut->data.ignore_starting_or_ending_with_apostrophe = true;
    }
    CHECK(sc.get_all_misspellings_as_string() == LR"(Cas
Eird
)");
    editor.set_active_document_text(LR"(
test
'ignoreme
abg
)");
    sc.recheck_visible_both_views();
    CHECK(editor.get_underlined_words(indicator_id) ==
        std::vector<std::string>{"abg"});
    editor.set_active_document_text(LR"(
test
test_test
)");
    {
      auto mut = settings.modify();
      mut->data.ignore_having_underscore = false;
    }
    CHECK(editor.get_underlined_words(indicator_id) ==
        std::vector{"test_test"s});
    {
      auto mut = settings.modify();
      mut->data.ignore_having_underscore = true;
    }
    CHECK(editor.get_underlined_words(indicator_id).empty ());

    {
      std::wstring text = L"nurserymaid ";
      std::wstring more_text;
      std::wstring_view arg = L"test ";
      for (int i = 0; i < 16; ++i)
        more_text.insert(more_text.end(), arg.begin(), arg.end());
      more_text.pop_back();
      for (int i = 0; i < 60; ++i) {
        text.insert(text.end(), more_text.begin(), more_text.end());
        text.push_back('\n');
      }
      text.pop_back();
      editor.set_active_document_text(text);
      sc.find_prev_mistake();
      CHECK(editor.selected_text() == "nurserymaid");
    }

    {
      editor.set_active_document_text(LR"(siudhsaudhasduihasiudhasuiasidhasiudhsaudhasduihasiudhasui)");
      editor.set_selection(1, 1);
      sc.find_prev_mistake();
    }

    {
      std::wstring text = L"nurserymaid  ";
      std::wstring more_text;
      std::wstring_view arg = L"test ";
      for (int i = 0; i < 12; ++i)
        text.insert(text.end(), arg.begin(), arg.end());
      for (int i = 0; i < 16; ++i)
        more_text.insert(more_text.end(), arg.begin(), arg.end());
      more_text.pop_back();
      for (int i = 0; i < 60; ++i) {
        text.insert(text.end(), more_text.begin(), more_text.end());
        text.push_back('\n');
      }
      text.pop_back();
      editor.set_active_document_text(text);
      sc.find_next_mistake();
      CHECK(editor.selected_text() == "nurserymaid");
      sc.find_next_mistake();
      CHECK(editor.selected_text() == "nurserymaid");
    }
    {
      editor.set_active_document_text(L"abcdef");
      sc.recheck_visible_both_views();
      CHECK(editor.get_underlined_words(indicator_id) ==
          std::vector<std::string>{"abcdef"});
      {
        auto mut = settings.modify();
        mut->data.tokenization_style = TokenizationStyle::by_delimiters;
        mut->data.delimiters += L"abcdef";
      }
      CHECK(editor.get_underlined_words(indicator_id).empty());
    }
  }
  SECTION("Is word under cursor correct") {
    TextPosition pos, length;
    {
      {
        auto mut = settings.modify();
        mut->data.tokenization_style = TokenizationStyle::by_non_alphabetic;
      }
      editor.set_active_document_text(
          L"これはテストです"); // each one is delimiter
      CHECK(sc.is_word_under_cursor_correct(pos, length, true));
      CHECK(length == 0);
    }
    {
      // check that right clicking when cursor after the word works
      editor.set_active_document_text(L"abcdef test");
      editor.set_cursor_pos(6);
      CHECK_FALSE(sc.is_word_under_cursor_correct(pos, length, true));
    }
    {
      // check that right clicking when cursor after the word works
      editor.set_active_document_text(L"abcdef test");
      editor.set_cursor_pos(11);
      CHECK(sc.is_word_under_cursor_correct(pos, length, true));
      editor.set_selection(0, 6);
      CHECK(!sc.is_word_under_cursor_correct(pos, length, true));
      editor.set_selection(0, 5);
      CHECK(sc.is_word_under_cursor_correct(pos, length, true));
    }
    {
      // check for wrong utf-8 characters
      editor.set_active_document_text_raw("abcdef\xBE\xE3");
      sc.recheck_visible_both_views();
    }
    {
      editor.set_active_document_text(L"");
      sc.recheck_visible_both_views();
      editor.set_cursor_pos(0);
      CHECK(sc.is_word_under_cursor_correct(pos, length, true));
    }
    {
      editor.set_codepage(EditorCodepage::ansi);
      sc.recheck_visible_both_views();
      editor.set_cursor_pos(0);
      CHECK(sc.is_word_under_cursor_correct(pos, length, true));
    }
  }

  SECTION("Mouse cursor pos") {
    TextPosition pos, length;
    {
      // check that right clicking when cursor after the word works
      editor.set_active_document_text(L"abcdef test");
      CHECK(sc.is_word_under_cursor_correct(pos, length, false));
      editor.set_mouse_cursor_pos(POINT{-500, -500});
      CHECK(sc.is_word_under_cursor_correct(pos, length, false));
      editor.set_mouse_cursor_pos(POINT{MockEditorInterface::char_width * 3, 0});
      CHECK_FALSE(sc.is_word_under_cursor_correct(pos, length, false));
    }
    {
      // check that right clicking when cursor after the word works
      editor.set_active_document_text(L"abcdef test");
      editor.set_mouse_cursor_pos(POINT{MockEditorInterface::char_width * 10, MockEditorInterface::char_height / 2});
      CHECK(sc.is_word_under_cursor_correct(pos, length, false));
    }
  }

  SECTION("Editor rect") {
    // \r is used to emulate word wrap
    // \n separates real lines, \r word-wrapped ones
    editor.set_editor_rect(0, MockEditorInterface::char_height, 1000, MockEditorInterface::char_height * 3 - 1);
    editor.set_active_document_text(L"Thes\rtist\rhes\rinvesible\rwards\n");
    editor.set_visible_lines(0, 0);
    sc.recheck_visible_both_views();
    CHECK(editor.get_underlined_words(indicator_id) ==
          std::vector<std::string>{"tist", "hes"});
    editor.set_active_document_text(L"Thes osome und gret tist realy mbe hes invesible wards");
    editor.make_all_visible();
    editor.set_editor_rect(MockEditorInterface::char_height * 5, 0, MockEditorInterface::char_height * (5 + 5 + 1 + 3) - 1,
                           MockEditorInterface::char_height * 2 - 1);
    sc.recheck_visible_both_views();
    CHECK(editor.get_underlined_words(indicator_id) ==
          std::vector<std::string>{"osome", "und"});
  }

  SECTION("Replace all tokens") {
    editor.set_active_document_text(L"token token token nottoken token token");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "token", L"bar", false);
    CHECK(editor.get_active_document_text() == "bar bar bar nottoken bar bar");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "bar", L"foobuzz", false);
    CHECK(editor.get_active_document_text() == "foobuzz foobuzz foobuzz nottoken foobuzz foobuzz");
    {
      auto m = settings.modify();
      m->data.split_camel_case = true;
    }
    editor.set_active_document_text(L"token token stillToken notoken andOneMoreToken tokenno");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "token", L"foobar", false);
    CHECK(editor.get_active_document_text() == "foobar foobar stillFoobar notoken andOneMoreFoobar tokenno");

    editor.set_active_document_text(L"token⸺token⸺token⸺nottoken⸺token⸺token");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "token", L"bar", false);
    CHECK(editor.get_active_document_text() == "bar⸺bar⸺bar⸺nottoken⸺bar⸺bar");

    SpellCheckerHelpers::replace_all_tokens(editor, settings, "", L"bar", false);
    CHECK(editor.get_active_document_text() == "bar⸺bar⸺bar⸺nottoken⸺bar⸺bar");

    {
      auto m = settings.modify();
      m->data.split_camel_case = false;
      m->data.ignore_all_capital = false;
      m->data.ignore_having_a_capital = false;
      m->data.ignore_one_letter = false;
    }

    editor.set_active_document_text(L"token token stillToken notoken andOneMoreToken TOKEN ToKeN");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "token", L"foobar", false);
    CHECK(editor.get_active_document_text() == "foobar foobar stillToken notoken andOneMoreToken FOOBAR foobar");

    editor.set_active_document_text(L"a a a a a A a a A b b c c a a A");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "a", L"z", false);
    CHECK(editor.get_active_document_text() == "z z z z z Z z z Z b b c c z z Z");

    {
      auto m = settings.modify();
      m->data.ignore_having_a_capital = true;
    }
    editor.set_active_document_text(L"token Token");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "token", L"foobar", false);
    CHECK(editor.get_active_document_text() == "foobar Foobar");

    {
      auto m = settings.modify();
      m->data.ignore_starting_with_capital = true;
    }
    editor.set_active_document_text(L"token Token");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "token", L"foobar", false);
    CHECK(editor.get_active_document_text() == "foobar Token");

    {
      auto m = settings.modify();
      m->data.ignore_starting_with_capital = false;
      m->data.ignore_all_capital = false;
      m->data.ignore_having_a_capital = false;
    }

    // replacing not proper name
    editor.set_active_document_text(L"please PlEaSe Token Please PLEASE");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "please", L"foobar", false);
    CHECK(editor.get_active_document_text() == "foobar foobar Token Foobar FOOBAR");

    // replacing proper name
    editor.set_active_document_text(L"parise PaRiSe Token Parise PARISE");
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "parise", L"Paris", true);
    CHECK(editor.get_active_document_text() == "Paris Paris Token Paris Paris");

    editor.set_active_document_text(L"token token token nottoken token token");
    editor.set_codepage(EditorCodepage::ansi);
    SpellCheckerHelpers::replace_all_tokens(editor, settings, "token", L"bar", false);
    CHECK(editor.get_active_document_text() == "bar bar bar nottoken bar bar");

    editor.set_codepage (static_cast<EditorCodepage> (-1));
    REQUIRE_THROWS_AS (editor.to_editor_encoding(L""), std::runtime_error);
  }

  SECTION("Bookmarks") {
    editor.set_active_document_text(L"abcdef\ntest\ntest\nkolli");
    sc.mark_lines_with_misspelling();
    CHECK(editor.get_bookmarked_lines() == std::set<size_t> {0, 3});
  }

  SECTION("Leftover settings") {
    editor.set_active_document_text(L"нёмного Ёё");
    sc.recheck_visible_both_views();
    CHECK (editor.get_underlined_words(indicator_id) == std::vector{"нёмного"s, "Ёё"s});
    {
      auto mut = settings.modify();
      mut->data.ignore_yo = true;
      mut->data.ignore_having_a_capital = false;
    }
    sc.recheck_visible_both_views();
    CHECK (editor.get_underlined_words(indicator_id) == std::vector{"Ёё"s});
    editor.set_active_document_text(L"D’Artagnan");
    sc.recheck_visible_both_views();
    CHECK (editor.get_underlined_words(indicator_id).empty ());
    {
      auto mut = settings.modify();
      mut->data.convert_single_quotes = false;
      mut->data.remove_boundary_apostrophes = false;
    }
    sc.recheck_visible_both_views();
    CHECK (editor.get_underlined_words(indicator_id) == std::vector{"D’Artagnan"s});
    editor.set_active_document_text(L"’’’test’’’");
    sc.recheck_visible_both_views();
    CHECK (editor.get_underlined_words(indicator_id) == std::vector{"’’’test’’’"s});
    {
      auto mut = settings.modify();
      mut->data.remove_boundary_apostrophes = true;
      mut->data.convert_single_quotes = true;
    }
    sc.recheck_visible_both_views();
    CHECK (editor.get_underlined_words(indicator_id).empty ());
  }
  SECTION("Ignore regexp") {
    {
      auto mut = settings.modify();
      mut->data.ignore_regexp_str = L"[A-Z]{1,5}";
    }
    editor.set_active_document_text(L"ABBA ALEXANDER abba V");
    sc.recheck_visible_both_views();
    CHECK (editor.get_underlined_words(indicator_id) == std::vector{"ALEXANDER"s, "abba"s});
    sc.recheck_visible_both_views();
  }
  SECTION("Not called normally") {
    CHECK_FALSE (SpellCheckerHelpers::is_word_spell_checking_needed(settings, editor, L"", 0));
  }
}
