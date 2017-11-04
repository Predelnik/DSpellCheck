#pragma once

template <typename IsDelimiterType>
std::vector<std::basic_string_view<wchar_t>> tokenize_by_condition(std::basic_string_view<wchar_t> target,
                                                          IsDelimiterType is_delimiter) {
    int token_begin = 0, token_end = -1;
    std::vector<std::basic_string_view<wchar_t>> ret;
    int i = 0;

    auto on_delim = [&]()
    {
        if (token_begin < token_end) {
            ret.push_back(target.substr(token_begin, token_end - token_begin));
            token_end = -1;
        }
        token_begin = i + 1;
    };

    for (; i < static_cast<int>(target.size()); ++i) {
        if (!is_delimiter(target[i])) {
            token_end = i + 1;
        }
        else {
            on_delim();
        }
    }
    on_delim();
    return ret;
}

std::vector<std::basic_string_view<wchar_t>> tokenize_by_delimiters(std::basic_string_view<wchar_t> target,
                                                                    std::basic_string_view<wchar_t> delimiters);
