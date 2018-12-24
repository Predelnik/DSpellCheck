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

class IniWorker {
public:
    enum class Action {
        save,
        load,
    };

    explicit IniWorker(std::wstring_view app_name, std::wstring_view file_name, Action action);
    void process(const wchar_t* name, std::wstring& value, std::wstring_view default_value, bool in_quotes = false) const;
    void process(const wchar_t* name, int& value, int default_value) const;
    // Enums are currently processed as ints. This is no good but supporting backwards compatibility would be required to upgrade this
    template <typename EnumT, std::enable_if_t<std::is_enum_v<EnumT>>...>
    void process(const wchar_t* name, EnumT& value, EnumT default_value) const
    {
        using UType = std::underlying_type_t<EnumT>;
        auto u_value = static_cast<UType> (value);
        process (name, u_value, static_cast<UType> (default_value));
        value = static_cast<EnumT> (u_value);
    }
    void process_utf8(const wchar_t* name, std::string& value, const char* default_value, bool in_quotes = false) const;

    void process(const wchar_t* name, bool& value, bool default_value) const;
    Action get_action() const { return m_action; }

private:
    std::wstring m_app_name;
    std::wstring m_file_name;
    Action m_action;
};
