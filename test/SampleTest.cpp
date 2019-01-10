// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA.

#include "GithubFileListProvider.h"
#include "MainDef.h"
#include "MockEditorInterface.h"
#include "MockSpeller.h"
#include "SciLexer.h"
#include "Settings.h"
#include "SpellChecker.h"
#include "SpellerContainer.h"
#include "UrlHelpers.h"
#include "catch.hpp"
#include "utils/TemporaryAcessor.h"
#include "SpellCheckerHelpers.h"

void setup_speller(MockSpeller &speller) {
  speller.set_inner_dict({{L"English",
                           {L"This", L"is", L"test", L"document", L"Please",
                            L"bear", L"with", L"me", L"И", L"ещё", L"немного", L"слов"}}});
}

TEST_CASE("Simple") {
  Settings settings;
  auto speller = std::make_unique<MockSpeller>(settings);
  setup_speller(*speller);
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
  sc.erase_all_misspellings();
  CHECK(editor.get_active_document_text(v) == R"(

This is test document
)");
  editor.set_active_document_text(v, LR"(
нехорошееслово
И ещё немного слов
ошибочноеслово)");
  sc.erase_all_misspellings();
  CHECK(editor.get_active_document_text(v) == R"(

И ещё немного слов
)");
  editor.undo (v); // check that undo restore both removed words
  CHECK(editor.get_active_document_text(v) == R"(
нехорошееслово
И ещё немного слов
ошибочноеслово)");
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
                                   // if notifications were signals in
                                   // EditorInterface
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
  CHECK(sc.get_all_misspellings_as_string() == LR"(Cas
E
Eird
w
)");
  {
    auto mut = settings.modify();
    mut->ignore_one_letter = true;
    mut->ignore_all_capital = true;
    mut->ignore_starting_or_ending_with_apostrophe = true;
  }
  CHECK(sc.get_all_misspellings_as_string() == LR"(Cas
Eird
)");
  editor.set_active_document_text(v, LR"(
test
'ignoreme
abg
)");
  sc.recheck_visible_both_views();
  CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id) ==
        std::vector<std::string>{"abg"});

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
    editor.set_active_document_text(v, text);
    sc.find_prev_mistake();
    CHECK(editor.selected_text(v) == "nurserymaid");
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
    editor.set_active_document_text(v, text);
    sc.find_next_mistake();
    CHECK(editor.selected_text(v) == "nurserymaid");
    sc.find_next_mistake();
    CHECK(editor.selected_text(v) == "nurserymaid");
  }
  {
    editor.set_active_document_text(v, L"abcdef");
    sc.recheck_visible_both_views();
    CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id) ==
          std::vector<std::string>{"abcdef"});
    {
      auto mut = settings.modify();
      mut->tokenization_style = TokenizationStyle::by_delimiters;
      mut->delimiters += L"abcdef";
    }
    CHECK(editor.get_underlined_words(v, dspellchecker_indicator_id).empty());
  }
  {
    {
      auto mut = settings.modify();
      mut->tokenization_style = TokenizationStyle::by_non_alphabetic;
    }
    editor.set_active_document_text(
        v, L"これはテストです"); // each one is delimiter
    CHECK(sc.is_word_under_cursor_correct(pos, length, true));
    CHECK(length == 0);
  }
  {
    // check that right clicking when cursor after the word works
    editor.set_active_document_text(v, L"abcdef test");
    editor.set_cursor_pos(v, 6);
    CHECK_FALSE(sc.is_word_under_cursor_correct(pos, length, true));
  }
  {
    // check for wrong utf-8 characters
    editor.set_active_document_text_raw(v, "abcdef\xBE\xE3");
    sc.recheck_visible_both_views();
  }
  {
    editor.set_active_document_text(v, L"token token token nottoken token token");
    SpellCheckerHelpers::replace_all_tokens(editor, v, settings, "token", "bar");
    CHECK (editor.get_active_document_text(v) == "bar bar bar nottoken bar bar");
    SpellCheckerHelpers::replace_all_tokens(editor, v, settings, "bar", "foobuzz");
    CHECK (editor.get_active_document_text(v) == "foobuzz foobuzz foobuzz nottoken foobuzz foobuzz");
    {
      auto m = settings.modify();
      m->split_camel_case = true;
    }
    editor.set_active_document_text(v, L"token token stillToken notoken andOneMoreToken");
    SpellCheckerHelpers::replace_all_tokens(editor, v, settings, "token", "foobar");
    CHECK (editor.get_active_document_text(v) == "foobar foobar stillfoobar notoken andOneMorefoobar");
  }
}

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
    CHECK(editor.get_next_valid_end_pos(v, 55) == 56);
    // and a bit further
    CHECK(editor.get_prev_valid_begin_pos(v, 27) == 25);
    CHECK(editor.get_next_valid_end_pos(v, 56) == 57);
  }
}

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
    CHECK(!editor.get_underlined_words(v, dspellchecker_indicator_id).empty());

    editor.set_lexer(v, SCLEX_YAML);
    editor.set_whole_text_style(v, SCE_YAML_DEFAULT);
    sc.recheck_visible_both_views();
    CHECK(!editor.get_underlined_words(v, dspellchecker_indicator_id).empty());
  }
}

TEST_CASE("GitHub") {
  CHECK(UrlHelpers::is_github_url(L"github.com/predelnik/DSpellCheck"));
  CHECK(UrlHelpers::is_github_url(L"https://gitHub.com/predelnik/DSpellCheck"));
  CHECK_FALSE(UrlHelpers::is_github_url(
      L"http://www.gitHub.com/preDeEnik/DSpellCheck"));
  CHECK_FALSE(UrlHelpers::is_github_url(L"asiodaosidaasoida"));
  CHECK_FALSE(
      UrlHelpers::is_github_url(L"ftp://github.com/predelnik/DSpellCheck"));
  CHECK(UrlHelpers::is_ftp_url(L"ftp://lol.com"));
  CHECK_FALSE(UrlHelpers::is_ftp_url(L"aftp://lol.com"));
  CHECK_FALSE(UrlHelpers::is_ftp_url(L"asdasdoaishdo"));
  CHECK(UrlHelpers::github_url_to_api_recursive_tree_url(
            L"https://www.github.com/predelnik/DSpellCheck") ==
        L"https://api.github.com/repos/predelnik/DSpellCheck/git/trees/"
        L"master?recursive=1");
}
