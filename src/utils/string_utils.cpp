#include "string_utils.h"

std::vector<std::basic_string_view<wchar_t>> tokenize_by_delimiters(std::basic_string_view<wchar_t> target,
                                                                    std::basic_string_view<wchar_t> delimiters) {
    return tokenize_by_condition(target, [&](wchar_t c) { return delimiters.find(c) != std::string_view::npos; });
}
