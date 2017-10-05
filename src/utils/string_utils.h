#pragma once

template <typename CharType>
std::vector<std::basic_string_view<CharType>> tokenize(std::basic_string_view<CharType> target,
                                                       std::basic_string_view<CharType> delim) {
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
        if (std::find(delim.begin(), delim.end(), target[i]) == delim.end()) {
            token_end = i + 1;
        }
        else {
            on_delim();
        }
    }
    on_delim ();
    return ret;
}
