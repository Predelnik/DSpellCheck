#pragma once

template <typename IsDelimiterType>
std::vector<std::basic_string_view<wchar_t>> tokenize_by_condition(std::basic_string_view<wchar_t> target,
                                                                   IsDelimiterType is_delimiter,
                                                                   bool split_camel_case = false) {
    int token_begin = 0, token_end = -1;
    std::vector<std::basic_string_view<wchar_t>> ret;

    auto finalize_word = [&]()
    {
        if (token_begin < token_end) {
            ret.push_back(target.substr(token_begin, token_end - token_begin));
            token_end = -1;
        }
    };

    for (int i = 0; i < static_cast<int>(target.size()); ++i) {
        if (!is_delimiter(target[i])) {
            if (split_camel_case && i > token_begin && IsCharUpper(target [i]) && IsCharLower(target [i - 1])) {
                token_end = i;
                finalize_word ();
                token_begin = i;
            }
            else
              token_end = i + 1;
        }
        else {
            finalize_word();
            token_begin = i + 1;
        }
    }
    finalize_word();
    return ret;
}

std::vector<std::basic_string_view<wchar_t>> tokenize_by_delimiters(std::basic_string_view<wchar_t> target,
                                                                    std::basic_string_view<wchar_t> delimiters);
