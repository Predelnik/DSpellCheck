#pragma once

class IniWorker {
public:
    enum class Action {
        save,
        load,
    };

    explicit IniWorker(std::wstring_view app_name, std::wstring_view file_name, Action action);
    void process(const wchar_t* name, std::wstring& value, std::wstring_view default_value) const;
    void process(const wchar_t* name, int& value, int default_value) const;
    void process_utf8(const wchar_t* name, std::string& value, const char* default_value, bool in_quotes) const;

    void process(const wchar_t* name, bool& value, bool default_value) const;

private:
    std::wstring m_app_name;
    std::wstring m_file_name;
    Action m_action;
};
