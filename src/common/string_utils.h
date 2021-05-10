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

#pragma once

#include "plugin/MainDefs.h"

wchar_t make_upper(wchar_t c);
wchar_t make_lower(wchar_t c);
bool is_upper(wchar_t c);
bool is_lower(wchar_t c);

template <typename IsDelimiterType> class Tokenizer {
public:
  Tokenizer(const std::wstring_view &target, const IsDelimiterType &is_delimiter, bool split_camel_case)
    : m_target(target), m_is_delimiter(is_delimiter), m_split_camel_case(split_camel_case) {
  }

  std::vector<std::wstring_view> get_all_tokens() const {
    int token_begin = 0, token_end = -1;
    std::vector<std::wstring_view> ret;

    auto finalize_word = [&]() {
      if (token_begin < token_end) {
        ret.push_back(m_target.substr(token_begin, token_end - token_begin));
        ++token_end;
      }
    };

    for (int i = 0; i < static_cast<int>(m_target.size()); ++i) {
      if (!m_is_delimiter(m_target[i])) {
        if (m_split_camel_case && i > token_begin && is_upper(m_target[i]) &&
            (is_lower(m_target[i - 1]) || (i < static_cast<TextPosition>(m_target.length()) - 1 && is_lower(m_target[i + 1])))) {
          token_end = i;
          finalize_word();
          token_begin = i;
        } else
          token_end = i + 1;
      } else {
        finalize_word();
        token_begin = i + 1;
      }
    }
    finalize_word();
    return ret;
  }

  TextPosition prev_token_begin(TextPosition index) const {
    if (index <= 0)
      return 0;
    if (m_is_delimiter(m_target[index])) {
      if (m_is_delimiter(m_target[index - 1]))
        return index;
      else
        --index;
    }
    while (index >= 0 && !m_is_delimiter(m_target[index])) {
      if (m_split_camel_case && index > 0 && is_upper(m_target[index]) &&
          ((index == static_cast<TextPosition>(m_target.length()) - 1 || is_lower(m_target[index + 1])) || is_lower(m_target[index - 1])))
        return index;
      --index;
    }
    ++index;
    return index;
  }

  TextPosition next_token_end(TextPosition index) const {
    if (index >= static_cast<TextPosition>(m_target.length()))
      return static_cast<TextPosition>(m_target.length());
    while (index < static_cast<TextPosition>(m_target.length()) && !m_is_delimiter(m_target[index])) {
      if (m_split_camel_case && index < static_cast<TextPosition>(m_target.length()) - 1 &&
          is_upper(m_target[index + 1]) && (is_lower(m_target[index]) || (
                                              index < static_cast<TextPosition>(m_target.length()) - 2 && is_lower(m_target[index + 2]))))
        return index + 1;
      ++index;
    }
    return index;
  }

private:
  std::wstring_view m_target;
  IsDelimiterType m_is_delimiter;
  bool m_split_camel_case;
};

inline auto make_delimiter_tokenizer(std::wstring_view target, std::wstring_view delimiters, bool split_camel_case = false) {
  return Tokenizer(target, [=](wchar_t c) { return delimiters.find(c) != std::string_view::npos; }, split_camel_case);
}

namespace detail {
template <typename CharType> bool ends_with_helper(std::basic_string_view<CharType> const &target, std::basic_string_view<CharType> const &suffix) {
  if (target.length() >= suffix.length()) {
    return (0 == target.compare(target.length() - suffix.length(), suffix.length(), suffix));
  } else {
    return false;
  }
}

template <typename CharType> bool starts_with_helper(std::basic_string_view<CharType> const &target, std::basic_string_view<CharType> const &prefix) {
  if (target.length() >= prefix.length()) {
    return (0 == target.compare(0, prefix.length(), prefix));
  } else {
    return false;
  }
}

template <typename CharType>
std::basic_string_view<CharType> remove_prefix_equals_helper(std::basic_string_view<CharType> target, const std::basic_string_view<CharType> &prefix) {
  if (starts_with_helper(target, prefix))
    target.remove_prefix(prefix.length());
  return target;
}
} // namespace detail
inline bool starts_with(const std::string_view &target, const std::string_view &prefix) { return detail::starts_with_helper(target, prefix); }
inline bool starts_with(const std::wstring_view &target, const std::wstring_view &prefix) { return detail::starts_with_helper(target, prefix); }
inline bool ends_with(const std::string_view &target, const std::string_view &suffix) { return detail::ends_with_helper(target, suffix); }
inline bool ends_with(const std::wstring_view &target, const std::wstring_view &suffix) { return detail::ends_with_helper(target, suffix); }

inline std::string_view remove_prefix_equals(std::string_view target, const std::string_view &prefix) {
  return detail::remove_prefix_equals_helper(target, prefix);
}

inline std::wstring_view remove_prefix_equals(std::wstring_view target, const std::wstring_view &prefix) {
  return detail::remove_prefix_equals_helper(target, prefix);
}

void to_lower_inplace(std::wstring &s);
void to_upper_inplace(std::wstring &s);

namespace detail {
inline bool is_space_helper(wchar_t c) { return iswspace(c); }

inline bool is_space_helper(char c) { return isspace(c); }
} // namespace detail

template <typename CharType> std::basic_string_view<CharType> trim(std::basic_string_view<CharType> sv) {
  while (detail::is_space_helper(sv.front()))
    sv.remove_prefix(1);
  while (detail::is_space_helper(sv.back()))
    sv.remove_suffix(1);
  return sv;
}

// trim from start (in place)
void ltrim_inplace(std::wstring &s);

// trim from end (in place)
void rtrim_inplace(std::wstring &s);

// trim from both ends (in place)
void trim_inplace(std::wstring &s);

namespace detail {
template <typename T> struct identity {
  using type = T;
};

template <typename T> using identity_t = typename identity<T>::type;
} // namespace detail

template <typename CharType>
void replace_all_inplace(std::basic_string<CharType> &str, const detail::identity_t<std::basic_string_view<CharType>> &from,
                         const detail::identity_t<std::basic_string_view<CharType>> &to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

size_t find_case_insensitive(const std::string_view &str_haystack, const std::string_view &str_needle);

enum class string_case_type {
  lower,
  upper,
  title,
  mixed,
};

string_case_type get_string_case_type(const std::wstring_view &sv);
void apply_case_type(std::wstring &str, string_case_type type);
