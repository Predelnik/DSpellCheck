#pragma once

class Settings {
public:
    bool auto_check_text = false;
    std::wstring aspell_path;
    std::wstring hunspell_path;
    std::wstring additional_hunspell_path;
    std::wstring aspell_multi_languages;
    std::wstring hunspell_multi_languages;
    int suggestions_mode;
    std::wstring aspell_language;
    std::wstring hunspell_language;
    std::string delim_utf8;
    int suggestion_count;
    bool ignore_yo;
    bool convert_single_quotes;
    bool remove_boundary_apostrophes;
    bool check_those;
    std::wstring file_types;
    bool check_only_comments_and_strings;
    int underline_color;
    int underline_style;
    bool ignore_containing_digit;
    bool ignore_starting_with_capital;
    bool ignore_having_a_capital;
    bool ignore_all_capital;
    bool ignore_one_letter;
    bool ignore_having_underscore;

};
