#include "IniWorker.h"
#include "CommonFunctions.h"

IniWorker::IniWorker(const std::wstring& app_name, const std::wstring& file_name, Action action) : m_app_name{app_name},
                                                                                                   m_file_name{
                                                                                                       file_name
                                                                                                   },
                                                                                                   m_action{action} {
}

constexpr auto initial_buf_size = 256;
constexpr auto max_buf_size = 100 * 1024;

bool IniWorker::process(const std::wstring& name, std::wstring& value) const {
    switch (m_action) {
    case Action::save:
        return WritePrivateProfileString(m_app_name.data(), name.data(), value.data(), m_file_name.data()) != FALSE;
    case Action::load:
        {
            int buf_size = initial_buf_size;
            while (true) {
                std::vector<wchar_t> buf(buf_size);
                auto ret = GetPrivateProfileString(m_app_name.data(), name.data(), value.data(), buf.data(),
                                                   static_cast<DWORD>(buf.size()),
                                                   m_file_name.data());
                if (ret == 0)
                    return false;

                if (static_cast<int>(ret) < buf_size - 1) {
                    value = buf.data();
                    return true;
                }
                if (buf_size > max_buf_size)
                    return false;
                buf_size *= 2;
            }
        }
    }
    return false;
}

bool IniWorker::process(const std::wstring& name, int& value) const {
    switch (m_action) {
    case Action::save:
        {
            auto str = std::to_wstring(value);
            return process(name, str);
        }
    case Action::load:
        {
            std::wstring str;
            if (!process(name, str))
                return false;
            size_t idx;
            try {
                auto new_val = std::stoi(str, &idx, 10);
                value = new_val;
            }
            catch (...) {
                return false;
            }
            break;
        }
    }
    return false;
}

bool IniWorker::process_utf8(const std::wstring& name, std::string& value, bool in_quotes) const {
    switch (m_action) {
    case Action::save:
        {
            auto str = utf8_to_wstring(value.c_str());
            if (in_quotes)
                str = L'"' + str + L'"';
            process(name, str);
        }
        return true;
    case Action::load:
        {
            std::wstring str;
            if (process(name, str)) {
                auto str_v = std::wstring_view(str);
                if (in_quotes) {
                    if (str_v.front() != L'"' || str_v.back() != L'"')
                        return false;

                    if (!str_v.empty())
                        str_v.remove_prefix(1);
                    if (!str_v.empty())
                        str_v.remove_suffix(1);
                }
                value = to_utf8_string(str_v);
                return true;
            }
            return false;
        }
    }
    return false;
}

bool IniWorker::process(const std::wstring& name, bool& value) const {
    auto int_value = value ? 1 : 0;
    bool ret = process(name, int_value);
    value = int_value != 0;
    return ret;
}
