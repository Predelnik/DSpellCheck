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

#include "UrlHelpers.h"
#include "utils/string_utils.h"

namespace UrlHelpers {
bool is_ftp_url(std::wstring s) {
  trim_inplace(s);
  to_lower_inplace(s);
  return starts_with(s, L"ftp://");
}

bool is_github_url(std::wstring url) {
  trim_inplace(url);
  to_lower_inplace(url);
  auto view = std::wstring_view(url);
  view = remove_prefix_equals(view, L"https://");
  view = remove_prefix_equals(view, L"www.");
  return starts_with(view, L"github.com");
}

namespace {
std::wstring to_github_base_url(std::wstring github_url) {
  std::wstring_view vw = github_url;
  vw = remove_prefix_equals(vw, L"https://");
  vw = remove_prefix_equals(vw, L"www.");
  github_url = vw;
  github_url.insert(L"github.com/"sv.length(), L"repos/");
  return L"https://api." + github_url;
}
}



std::wstring github_url_to_api_recursive_tree_url(std::wstring github_url) {
  return to_github_base_url(std::move(github_url)) + L"/git/trees/master?recursive=1";
}

std::wstring github_url_to_contents_url(std::wstring github_url) { return to_github_base_url(std::move(github_url)) + L"/contents/"; }

std::wstring github_file_url_to_download_url(std::wstring github_url) {
  std::wstring_view vw = github_url;
  vw = remove_prefix_equals(vw, L"https://");
  vw = remove_prefix_equals(vw, L"www.");
  vw = remove_prefix_equals(vw, L"github.com");
  github_url = vw;
  github_url.insert(0, L"https://raw.githubusercontent.com");
  return github_url + L"/master/";
}
} // namespace UrlHelpers
