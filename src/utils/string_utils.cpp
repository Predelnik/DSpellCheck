#include "string_utils.h"
#include "utf8.h"

std::vector<std::string_view> tokenize_utf8(std::string_view target, std::string_view delim) {
    int token_begin = 0, token_end = -1;
    std::vector<std::string_view> ret;
    int i = 0;

    auto on_delim = [&]()
    {
        if (token_begin < token_end) {
            ret.push_back(target.substr(token_begin, token_end - token_begin));
            token_end = -1;
        }
    };

    for (; i < static_cast<int>(target.size()); i += utf8_symbol_len(target[i])) {
        if (utf8_chr(delim.data(), target.data() + i) == nullptr) {
            token_end = i + utf8_symbol_len(target[i]);
        }
        else {
            on_delim();
            token_begin = i + utf8_symbol_len(target[i]);
        }
    }
    on_delim();
    return ret;
}
