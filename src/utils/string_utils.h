#pragma once

template <typename CharType, typename IsDelimiterType>
std::vector<std::basic_string_view<CharType>> tokenize_by_condition(std::basic_string_view<CharType> target,
                                                          IsDelimiterType is_delimiter) {
    int token_begin = 0, token_end = -1;
    std::vector<std::basic_string_view<CharType>> ret;
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

template <typename CharType>
std::vector<std::basic_string_view<CharType>> tokenize_by_delimiters(std::basic_string_view<CharType> target,
                                                             std::basic_string_view<CharType> delimiters) {
    return tokenize_by_condition(target, [&](wchar_t c) { return delimiters.find(c) != std::string_view::npos; });
}

std::vector<std::string_view> tokenize_utf8(std::string_view target,
                                            std::string_view delim);
