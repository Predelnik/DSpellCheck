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
            L"https://www.github.com/predelnik/DSpellCheck") ==
        L"https://api.github.com/repos/predelnik/DSpellCheck/git/trees/"
        L"master?recursive=1");
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
