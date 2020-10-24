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

#include "utils/string_utils.h"

#include "UrlHelpers.h"

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
            L"https://www.github.com/predelnik/DSpellCheck", L"master") ==
        L"https://api.github.com/repos/predelnik/DSpellCheck/git/trees/"
        L"master?recursive=1");

  CHECK (L"https://raw.githubusercontent.com/test/test_repo/master/" ==
    UrlHelpers::github_file_url_to_download_url (L"https://www.github.com/test/test_repo", L"master"));
  CHECK (L"https://raw.githubusercontent.com/show/me/the/money/main/" ==
    UrlHelpers::github_file_url_to_download_url (L"github.com/show/me/the/money", L"main"));
}

TEST_CASE ("string_case") {
  CHECK(get_string_case_type(L"asda") == string_case_type::lower);
  CHECK(get_string_case_type(L"Asda") == string_case_type::title);
  CHECK(get_string_case_type(L"ASDA") == string_case_type::upper);
  CHECK(get_string_case_type(L"AsDA") == string_case_type::mixed);
  using namespace std::string_literals;
  auto str = L"ArArAr"s;
  apply_case_type (str, string_case_type::lower);
  CHECK (str == L"ararar");
  apply_case_type (str, string_case_type::upper);
  CHECK (str == L"ARARAR");
  apply_case_type (str, string_case_type::title);
  CHECK (str == L"Ararar");
}

TEST_CASE ("CamelCase") {
  std::wstring s = L"TestCamelCase";
  //                 0   4    9   13
  auto t = [&](){ return make_delimiter_tokenizer (s, L" ", true); };
  CHECK(t().get_all_tokens() == std::vector<std::wstring_view>{L"Test", L"Camel", L"Case"});
  auto test_1 = [&](){
  CHECK(t().prev_token_begin(2) == 0);
  CHECK(t().next_token_end(2) == 4);
  CHECK(t().prev_token_begin(6) == 4);
  CHECK(t().next_token_end(6) == 9);
  CHECK(t().prev_token_begin(10) == 9);
  CHECK(t().next_token_end(10) == 13);
  };
  test_1 ();
  s = L"TestCamelCaseAndABBREVIATION";
  //    0   4    9   13 16         28
  CHECK(t().get_all_tokens() == std::vector<std::wstring_view>{L"Test", L"Camel", L"Case", L"And", L"ABBREVIATION"});
  auto test_2 = [&](){
    CHECK(t().prev_token_begin(14) == 13);
    CHECK(t().next_token_end(14) == 16);
    CHECK(t().prev_token_begin(23) == 16);
    CHECK(t().next_token_end(23) == 28);
  };
  test_2 ();
  s = L"TestCamelCaseAndABBREVIATIONAndSomeMoreCamelCase";
  //    0   4    9   13 16         28  31  35  39   44  48
  auto test_3 = [&]()
  {
    CHECK(t().prev_token_begin(30) == 28);
    CHECK(t().next_token_end(30) == 31);
    CHECK(t().prev_token_begin(33) == 31);
    CHECK(t().next_token_end(33) == 35);
    CHECK(t().prev_token_begin(37) == 35);
    CHECK(t().next_token_end(37) == 39);
    CHECK(t().prev_token_begin(41) == 39);
    CHECK(t().next_token_end(41) == 44);
    CHECK(t().prev_token_begin(46) == 44);
    CHECK(t().next_token_end(46) == 48);
  };

  CHECK(t().get_all_tokens() == std::vector<std::wstring_view>{L"Test", L"Camel", L"Case", L"And", L"ABBREVIATION", L"And", L"Some", L"More", L"Camel", L"Case"});
  test_1 ();
  test_2 ();
  test_3 ();
}
