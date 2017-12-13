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

private:
    std::wstring m_app_name;
    std::wstring m_file_name;
    Action m_action;
};
