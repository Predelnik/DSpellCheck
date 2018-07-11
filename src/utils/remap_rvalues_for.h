#pragma once

#include <utility>

// useful for chaining functions. Just write original chaining function ref
// qualified Then write REMAP_RVALUES_FOR (function name) it will add
// appropriate overload for ref-ref-qualified case. Preserving reference
// category of return type.
#define REMAP_RVALUES_FOR(MEMBER_FUNC_NAME)                                    \
  template <typename... ArgTypes>                                              \
  auto MEMBER_FUNC_NAME(ArgTypes &&... args) && {                              \
    return std::move(                                                          \
        static_cast<std::remove_pointer_t<decltype(this)> &>(*this)            \
            .MEMBER_FUNC_NAME(std::forward<ArgTypes>(args)...));               \
  }
