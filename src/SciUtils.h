#pragma once

namespace SciUtils
{
    enum class StyleCategory
    {
      text,       // should be spell-checked
      comment,
      string,
      identifier,
      unknown,    // Should not be spell-checked

      COUNT,
    };

    StyleCategory get_style_category(LRESULT lexer, LRESULT style);
} // namespace SciUtils
