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

#include "IniWorker.h"

#include "Utility.h"

IniWorker::IniWorker(std::wstring_view app_name, std::wstring_view file_name, Action action)
  : m_app_name{app_name},
    m_file_name{
        file_name
    },
    m_action{action} {
}

constexpr auto initial_buf_size = 256;
constexpr auto max_buf_size = 100 * 1024;

void IniWorker::process(const wchar_t *name, std::wstring &value, std::wstring_view default_value,
                        bool in_quotes) const {
  switch (m_action) {
  case Action::save: {
    if (value == default_value)
      return;
    std::wstring str = value;
    if (in_quotes)
      str = L'"' + str + L'"';
    WritePrivateProfileString(m_app_name.data(), name, str.data(), m_file_name.data());
  }
    return;
  case Action::load: {
    int buf_size = initial_buf_size;
    std::wstring str(default_value);
    while (true) {
      std::vector<wchar_t> buf(buf_size);
      auto ret = GetPrivateProfileString(m_app_name.data(), name, str.data(), buf.data(),
                                         static_cast<DWORD>(buf.size()),
                                         m_file_name.data());
      if (ret == 0) {
        str = default_value;
        break;
      }

      if (static_cast<int>(ret) < buf_size - 1) {
        str = buf.data();
        break;
      }
      if (buf_size > max_buf_size) {
        str = default_value;
        break;
      }
      buf_size *= 2;
    }

    if (in_quotes) {
      auto str_v = std::wstring_view(str);
      if (str_v.front() != L'"' || str_v.back() != L'"') {
        value = default_value;
        return;
      }

      if (!str_v.empty())
        str_v.remove_prefix(1);
      if (!str_v.empty())
        str_v.remove_suffix(1);
      str = str_v;
    }
    value = str;
  }
  }
}

void IniWorker::process(const wchar_t *name, int &value, int default_value) const {
  switch (m_action) {
  case Action::save: {
    auto str = std::to_wstring(value);
    return process(name, str, std::to_wstring(default_value));
  }
  case Action::load: {
    std::wstring str;
    process(name, str, std::to_wstring(default_value));
    size_t idx;
    try {
      auto new_val = std::stoi(str, &idx, 10);
      value = new_val;
    } catch (...) {
      value = default_value;
    }
    break;
  }
  }
}

void IniWorker::process_utf8(const wchar_t *name, std::string &value, const char *default_value,
                             bool in_quotes) const {
  switch (m_action) {
  case Action::save: {
    auto str = utf8_to_wstring(value.c_str());
    process(name, str, utf8_to_wstring(default_value), in_quotes);
  }
    return;
  case Action::load: {
    std::wstring str;
    process(name, str, utf8_to_wstring(default_value), in_quotes);
    value = to_utf8_string(str);
  }
  }
}

void IniWorker::process(const wchar_t *name, bool &value, bool default_value) const {
  auto int_value = value ? 1 : 0;
  process(name, int_value, default_value ? 1 : 0);
  value = int_value != 0;
}
