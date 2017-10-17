#include "IniWorker.h"
#include "CommonFunctions.h"

IniWorker::IniWorker(std::wstring_view app_name, std::wstring_view file_name, Action action) : m_app_name{app_name},
                                                                                               m_file_name{
                                                                                                   file_name
                                                                                               },
                                                                                               m_action{action} {
}

constexpr auto initial_buf_size = 256;
constexpr auto max_buf_size = 100 * 1024;

void IniWorker::process(const wchar_t* name, std::wstring& value, std::wstring_view default_value) const {
    switch (m_action) {
    case Action::save:
        WritePrivateProfileString(m_app_name.data(), name, value.data(), m_file_name.data());
        return;
    case Action::load:
        {
            int buf_size = initial_buf_size;
            while (true) {
                std::vector<wchar_t> buf(buf_size);
                auto ret = GetPrivateProfileString(m_app_name.data(), name, value.data(), buf.data(),
                                                   static_cast<DWORD>(buf.size()),
                                                   m_file_name.data());
                if (ret == 0) {
                    value = default_value;
                    return;
                }

                if (static_cast<int>(ret) < buf_size - 1) {
                    value = buf.data();
                    return;
                }
                if (buf_size > max_buf_size) {
                    value = default_value;
                    return;
                }
                buf_size *= 2;
            }
        }
    }
}

void IniWorker::process(const wchar_t* name, int& value, int default_value) const {
    switch (m_action) {
    case Action::save:
        {
            auto str = std::to_wstring(value);
            return process(name, str, std::to_wstring(default_value));
        }
    case Action::load:
        {
            std::wstring str;
            process(name, str, std::to_wstring(default_value));
            size_t idx;
            try {
                auto new_val = std::stoi(str, &idx, 10);
                value = new_val;
            }
            catch (...) {
                value = default_value;
            }
            break;
        }
    }
}

void IniWorker::process_utf8(const wchar_t* name, std::string& value, const char* default_value,
                             bool in_quotes = false) const {
    switch (m_action) {
    case Action::save:
        {
            auto str = utf8_to_wstring(value.c_str());
            if (in_quotes)
                str = L'"' + str + L'"';
            process(name, str, utf8_to_wstring(default_value));
        }
        return;
    case Action::load:
        {
            std::wstring str;
            process(name, str, utf8_to_wstring(default_value));
            {
                auto str_v = std::wstring_view(str);
                if (in_quotes) {
                    if (str_v.front() != L'"' || str_v.back() != L'"') {
                        value = default_value;
                        return;
                    }

                    if (!str_v.empty())
                        str_v.remove_prefix(1);
                    if (!str_v.empty())
                        str_v.remove_suffix(1);
                }
                value = to_utf8_string(str_v);
            }
        }
    }
}

void IniWorker::process(const wchar_t* name, bool& value, bool default_value) const {
    auto int_value = value ? 1 : 0;
    process(name, int_value, default_value ? 1 : 0);
    value = int_value != 0;
}
