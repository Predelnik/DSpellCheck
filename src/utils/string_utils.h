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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#pragma once

template <typename IsDelimiterType> class Tokenizer {
public:
  Tokenizer(const std::wstring_view &target,
            const IsDelimiterType &is_delimiter, bool split_camel_case)
      : m_target(target), m_is_delimiter(is_delimiter),
        m_split_camel_case(split_camel_case) {}

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
        if (m_split_camel_case && i > token_begin && IsCharUpper(m_target[i]) &&
            IsCharLower(m_target[i - 1])) {
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

  long prev_token_begin(long index) {
    if (index < 0)
        return 0;
    if (m_is_delimiter(m_target[index]))
      return index;
    while (index >= 0 && !m_is_delimiter(m_target[index])) {
      if (m_split_camel_case && index > 0 && IsCharLower(m_target[index - 1]) &&
          IsCharUpper(m_target[index]))
        return index;
      --index;
    }
    ++index;
    return index;
  }

  long next_token_end(long index) {
    if (index >= static_cast<long> (m_target.length ()))
      return static_cast<long> (m_target.length ());
    while (index < static_cast<long>(m_target.length()) &&
           !m_is_delimiter(m_target[index])) {
      if (m_split_camel_case &&
          index < static_cast<long>(m_target.length()) - 1 &&
          IsCharUpper(m_target[index + 1]) && IsCharLower(m_target[index]))
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

template <typename IsDelimiterType>
auto make_condition_tokenizer(std::wstring_view target,
                              IsDelimiterType is_delimiter,
                              bool split_camel_case = false) {
  return Tokenizer<IsDelimiterType>(target, is_delimiter, split_camel_case);
}

inline auto make_delimiter_tokenizer(std::wstring_view target,
                                     std::wstring_view delimiters,
                                     bool split_camel_case = false) {
  return make_condition_tokenizer(
      target,
      [=](wchar_t c) { return delimiters.find(c) != std::string_view::npos; },
      split_camel_case);
}
