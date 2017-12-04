#pragma once

namespace SciUtils
{
    enum class StyleCategory
    {
      text,     // should be spell-checked
      comment,  // 
      string,   // 
      variable, //  
      function, //
      unknown,  // Should not be spell-checked
      
      COUNT,
    };

    StyleCategory get_style_category(LRESULT lexer, LRESULT style);
}
