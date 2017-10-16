#pragma once

class IniWorker {
public:
    enum class Action {
        save,
        load,
    };

    explicit IniWorker(const std::wstring& app_name, const std::wstring& file_name, Action action);
    bool process(const std::wstring& name, std::wstring& value) const;
    bool process(const std::wstring& name, int& value) const;

    bool process_utf8(const std::wstring& name, std::string& value, bool in_quotes) const;

    bool process(const std::wstring& name, bool& value) const;

private:
    std::wstring m_app_name;
    std::wstring m_file_name;
    Action m_action;
};
