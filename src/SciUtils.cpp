// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "SciUtils.h"
#include "SciLexer.h"

namespace SciUtils
{
    StyleCategory get_style_category(LRESULT lexer, LRESULT style)
    {
        using s = StyleCategory;
        switch (lexer)
        {
        case SCLEX_CONTAINER:
        case SCLEX_NULL:
            return s::text;
            // Meaning unclear:
        case SCLEX_AS:
        case SCLEX_SREC:
        case SCLEX_IHEX:
        case SCLEX_TEHEX:
            return s::unknown;
        case SCLEX_USER:
            switch (style)
            {
            case SCE_USER_STYLE_COMMENT:
            case SCE_USER_STYLE_COMMENTLINE:
                return s::comment;
            case SCE_USER_STYLE_DELIMITER1:
            case SCE_USER_STYLE_DELIMITER2:
            case SCE_USER_STYLE_DELIMITER3:
            case SCE_USER_STYLE_DELIMITER4:
            case SCE_USER_STYLE_DELIMITER5:
            case SCE_USER_STYLE_DELIMITER6:
            case SCE_USER_STYLE_DELIMITER7:
            case SCE_USER_STYLE_DELIMITER8:
                return s::string;
            case SCE_USER_STYLE_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            };
        case SCLEX_PYTHON:
            switch (style)
            {
            case SCE_P_COMMENTLINE:
            case SCE_P_COMMENTBLOCK:
                return s::comment;
            case SCE_P_STRING:
            case SCE_P_TRIPLE:
            case SCE_P_TRIPLEDOUBLE:
            case SCE_P_CHARACTER:
                return s::string;
            case SCE_P_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            };
        case SCLEX_CPP:
        case SCLEX_OBJC:
        case SCLEX_BULLANT:
            switch (style)
            {
            case SCE_C_COMMENT:
            case SCE_C_COMMENTLINE:
            case SCE_C_COMMENTDOC:
            case SCE_C_COMMENTLINEDOC:
                return s::comment;
            case SCE_C_STRING:
                return s::string;
            case SCE_C_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            };
        case SCLEX_HTML:
        case SCLEX_XML:
            switch (style)
            {
            case SCE_H_DEFAULT:
                return s::text;
            case SCE_H_COMMENT:
            case SCE_H_XCCOMMENT:
            case SCE_H_SGML_COMMENT:
            case SCE_HJ_COMMENT:
            case SCE_HJ_COMMENTLINE:
            case SCE_HJ_COMMENTDOC:
            case SCE_HJA_COMMENT:
            case SCE_HJA_COMMENTLINE:
            case SCE_HJA_COMMENTDOC:
            case SCE_HB_COMMENTLINE:
            case SCE_HBA_COMMENTLINE:
            case SCE_HP_COMMENTLINE:
            case SCE_HPA_COMMENTLINE:
            case SCE_HPHP_COMMENT:
            case SCE_HPHP_COMMENTLINE:
                return s::comment;
            case SCE_H_DOUBLESTRING:
            case SCE_H_SINGLESTRING:
            case SCE_HJ_STRINGEOL:
            case SCE_HJA_DOUBLESTRING:
            case SCE_HJA_SINGLESTRING:
            case SCE_HJA_STRINGEOL:
            case SCE_HB_STRING:
            case SCE_HB_STRINGEOL:
            case SCE_HBA_STRING:
            case SCE_HP_STRING:
            case SCE_HPA_STRING:
            case SCE_HPHP_HSTRING:
            case SCE_HPHP_SIMPLESTRING:
                return s::string;
            case SCE_HP_IDENTIFIER:
            case SCE_HPA_IDENTIFIER:
            case SCE_HPHP_COMPLEX_VARIABLE:
            case SCE_HPHP_VARIABLE:
            case SCE_HPHP_HSTRING_VARIABLE:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_PERL:
            switch (style)
            {
            case SCE_PL_COMMENTLINE:
                return s::comment;
            case SCE_PL_STRING_Q:
            case SCE_PL_STRING_QQ:
            case SCE_PL_STRING_QX:
            case SCE_PL_STRING_QR:
            case SCE_PL_STRING_QW:
                return s::string;
            case SCE_PL_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            };
        case SCLEX_SQL:
            switch (style)
            {
            case SCE_SQL_STRING:
                return s::string;
            case SCE_SQL_COMMENT:
            case SCE_SQL_COMMENTLINE:
            case SCE_SQL_COMMENTDOC:
            case SCE_SQL_COMMENTLINEDOC:
                return s::comment;
            case SCE_SQL_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_PROPERTIES:
            switch (style)
            {
            case SCE_PROPS_COMMENT:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_ERRORLIST:
            return s::unknown;
        case SCLEX_MAKEFILE:
            switch (style)
            {
            case SCE_MAKE_COMMENT:
                return s::comment;
            case SCE_MAKE_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_BATCH:
            switch (style)
            {
            case SCE_BAT_COMMENT:
                return s::comment;
            case SCE_BAT_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_XCODE:
            return s::unknown;
        case SCLEX_LATEX:
            switch (style)
            {
            case SCE_L_DEFAULT:
                return s::text;
            case SCE_L_COMMENT:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_LUA:
            switch (style)
            {
            case SCE_LUA_COMMENT:
            case SCE_LUA_COMMENTLINE:
            case SCE_LUA_COMMENTDOC:
                return s::comment;
            case SCE_LUA_STRING:
                return s::string;
            case SCE_LUA_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_DIFF:
            switch (style)
            {
            case SCE_DIFF_COMMENT:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_CONF:
            switch (style)
            {
            case SCE_CONF_COMMENT:
                return s::comment;
            case SCE_CONF_STRING:
                return s::string;
            case SCE_CONF_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_PASCAL:
            switch (style)
            {
            case SCE_PAS_COMMENT:
            case SCE_PAS_COMMENT2:
            case SCE_PAS_COMMENTLINE:
                return s::comment;
            case SCE_PAS_STRING:
                return s::string;
            case SCE_PAS_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_AVE:
            switch (style)
            {
            case SCE_AVE_COMMENT:
                return s::comment;
            case SCE_AVE_STRING:
                return s::string;
            case SCE_AVE_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_ADA:
            switch (style)
            {
            case SCE_ADA_STRING:
                return s::string;
            case SCE_ADA_COMMENTLINE:
                return s::comment;
            case SCE_ADA_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_LISP:
            switch (style)
            {
            case SCE_LISP_COMMENT:
                return s::comment;
            case SCE_LISP_STRING:
                return s::string;
            case SCE_LISP_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_RUBY:
            switch (style)
            {
            case SCE_RB_COMMENTLINE:
                return s::comment;
            case SCE_RB_STRING:
            case SCE_RB_STRING_Q:
            case SCE_RB_STRING_QQ:
            case SCE_RB_STRING_QX:
            case SCE_RB_STRING_QR:
            case SCE_RB_STRING_QW:
                return s::string;
            case SCE_RB_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_EIFFEL:
        case SCLEX_EIFFELKW:
            switch (style)
            {
            case SCE_EIFFEL_COMMENTLINE:
                return s::comment;
            case SCE_EIFFEL_STRING:
                return s::string;
            case SCE_EIFFEL_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            };
        case SCLEX_TCL:
            switch (style)
            {
            case SCE_TCL_COMMENT:
            case SCE_TCL_COMMENTLINE:
            case SCE_TCL_BLOCK_COMMENT:
                return s::comment;
            case SCE_TCL_IN_QUOTE:
                return s::string;
            case SCE_TCL_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_NNCRONTAB:
            switch (style)
            {
            case SCE_NNCRONTAB_COMMENT:
                return s::comment;
            case SCE_NNCRONTAB_STRING:
                return s::string;
            case SCE_NNCRONTAB_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_BAAN:
            switch (style)
            {
            case SCE_BAAN_COMMENT:
            case SCE_BAAN_COMMENTDOC:
                return s::comment;
            case SCE_BAAN_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_MATLAB:
            switch (style)
            {
            case SCE_MATLAB_COMMENT:
                return s::comment;
            case SCE_MATLAB_STRING:
                return s::string;
            case SCE_MATLAB_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_SCRIPTOL:
            switch (style)
            {
            case SCE_SCRIPTOL_COMMENTLINE:
            case SCE_SCRIPTOL_COMMENTBLOCK:
                return s::comment;
            case SCE_SCRIPTOL_STRING:
                return s::string;
            case SCE_SCRIPTOL_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_ASM:
            switch (style)
            {
            case SCE_ASM_COMMENT:
            case SCE_ASM_COMMENTBLOCK:
            case SCE_ASM_COMMENTDIRECTIVE:
                return s::comment;
            case SCE_ASM_STRING:
            case SCE_ASM_STRINGEOL:
            case SCE_ASM_CHARACTER:
                return s::string;
            case SCE_ASM_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_CPPNOCASE:
        case SCLEX_FORTRAN:
        case SCLEX_F77:
            switch (style)
            {
            case SCE_F_COMMENT:
                return s::comment;
            case SCE_F_STRING1:
            case SCE_F_STRING2:
                return s::string;
            case SCE_F_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_CSS:
            switch (style)
            {
            case SCE_CSS_IDENTIFIER:
            case SCE_CSS_VARIABLE:
                return s::identifier;
            case SCE_CSS_COMMENT:
                return s::comment;
            case SCE_CSS_DOUBLESTRING:
            case SCE_CSS_SINGLESTRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_POV:
            switch (style)
            {
            case SCE_POV_IDENTIFIER:
                return s::identifier;
            case SCE_POV_COMMENT:
            case SCE_POV_COMMENTLINE:
                return s::comment;
            case SCE_POV_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_LOUT:
            switch (style)
            {
            case SCE_LOUT_IDENTIFIER:
                return s::identifier;
            case SCE_LOUT_COMMENT:
                return s::comment;
            case SCE_LOUT_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_ESCRIPT:
            switch (style)
            {
            case SCE_ESCRIPT_IDENTIFIER:
                return s::identifier;
            case SCE_ESCRIPT_COMMENT:
            case SCE_ESCRIPT_COMMENTLINE:
            case SCE_ESCRIPT_COMMENTDOC:
                return s::comment;
            case SCE_ESCRIPT_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_PS:
            switch (style)
            {
            case SCE_PS_COMMENT:
            case SCE_PS_DSC_COMMENT:
                return s::comment;
            case SCE_PS_TEXT:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_NSIS:
            switch (style)
            {
            case SCE_NSIS_COMMENT:
                return s::comment;
            case SCE_NSIS_STRINGDQ:
            case SCE_NSIS_STRINGLQ:
            case SCE_NSIS_STRINGRQ:
                return s::string;
            case SCE_NSIS_VARIABLE:
            case SCE_NSIS_FUNCTION:
            case SCE_NSIS_FUNCTIONDEF:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_MMIXAL:
            switch (style)
            {
            case SCE_MMIXAL_COMMENT:
                return s::comment;
            case SCE_MMIXAL_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_CLW:
            switch (style)
            {
            case SCE_CLW_USER_IDENTIFIER:
                return s::identifier;
            case SCE_CLW_COMMENT:
                return s::comment;
            case SCE_CLW_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_CLWNOCASE:
        case SCLEX_LOT:
            return s::unknown;
        case SCLEX_YAML:
            switch (style)
            {
            case SCE_YAML_IDENTIFIER:
                return s::identifier;
            case SCE_YAML_COMMENT:
                return s::comment;
            case SCE_YAML_TEXT:
                return s::text;
            default:
                return s::unknown;
            }
        case SCLEX_TEX:
            switch (style)
            {
            case SCE_TEX_TEXT:
                return s::text;
            default:
                return s::unknown;
            }
        case SCLEX_METAPOST:
            switch (style)
            {
            case SCE_METAPOST_TEXT:
            case SCE_METAPOST_DEFAULT:
                return s::text;
            default:
                return s::unknown;
            }
        case SCLEX_FORTH:
            switch (style)
            {
            case SCE_FORTH_IDENTIFIER:
                return s::identifier;
            case SCE_FORTH_COMMENT:
            case SCE_FORTH_COMMENT_ML:
                return s::comment;
            case SCE_FORTH_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_ERLANG:
            switch (style)
            {
            case SCE_ERLANG_STRING:
                return s::string;
            case SCE_ERLANG_COMMENT:
            case SCE_ERLANG_COMMENT_FUNCTION:
            case SCE_ERLANG_COMMENT_MODULE:
            case SCE_ERLANG_COMMENT_DOC:
            case SCE_ERLANG_COMMENT_DOC_MACRO:
                return s::comment;
            case SCE_ERLANG_FUNCTION_NAME:
            case SCE_ERLANG_VARIABLE:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_OCTAVE:
        case SCLEX_MSSQL:
            switch (style)
            {
            case SCE_MSSQL_IDENTIFIER:
            case SCE_MSSQL_VARIABLE:
            case SCE_MSSQL_FUNCTION:
                return s::identifier;
            case SCE_MSSQL_COMMENT:
            case SCE_MSSQL_LINE_COMMENT:
                return s::comment;
            case SCE_MSSQL_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_VERILOG:
            switch (style)
            {
            case SCE_V_IDENTIFIER:
                return s::identifier;
            case SCE_V_COMMENT:
            case SCE_V_COMMENTLINE:
            case SCE_V_COMMENTLINEBANG:
                return s::comment;
            case SCE_V_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_KIX:
            switch (style)
            {
            case SCE_KIX_IDENTIFIER:
            case SCE_KIX_FUNCTIONS:
            case SCE_KIX_VAR:
                return s::identifier;
            case SCE_KIX_COMMENT:
            case SCE_KIX_COMMENTSTREAM:
                return s::comment;
            case SCE_KIX_STRING1:
            case SCE_KIX_STRING2:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_GUI4CLI:
            switch (style)
            {
            case SCE_GC_COMMENTLINE:
            case SCE_GC_COMMENTBLOCK:
                return s::comment;
            case SCE_GC_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_SPECMAN:
            switch (style)
            {
            case SCE_SN_IDENTIFIER:
                return s::identifier;
            case SCE_SN_COMMENTLINE:
            case SCE_SN_COMMENTLINEBANG:
                return s::comment;
            case SCE_SN_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_AU3:
            switch (style)
            {
            case SCE_AU3_COMMENT:
            case SCE_AU3_COMMENTBLOCK:
                return s::comment;
            case SCE_AU3_STRING:
                return s::string;
            case SCE_AU3_VARIABLE:
            case SCE_AU3_FUNCTION:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_APDL:
            switch (style)
            {
            case SCE_APDL_COMMENT:
            case SCE_APDL_COMMENTBLOCK:
                return s::comment;
            case SCE_APDL_STRING:
                return s::string;
            case SCE_APDL_FUNCTION:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_BASH:
            switch (style)
            {
            case SCE_SH_IDENTIFIER:
                return s::identifier;
            case SCE_SH_COMMENTLINE:
                return s::comment;
            case SCE_SH_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_ASN1:
            switch (style)
            {
            case SCE_ASN1_IDENTIFIER:
                return s::identifier;
            case SCE_ASN1_COMMENT:
                return s::comment;
            case SCE_ASN1_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_VHDL:
            switch (style)
            {
            case SCE_VHDL_IDENTIFIER:
                return s::identifier;
            case SCE_VHDL_COMMENT:
            case SCE_VHDL_COMMENTLINEBANG:
                return s::comment;
            case SCE_VHDL_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_CAML:
            switch (style)
            {
            case SCE_CAML_IDENTIFIER:
                return s::identifier;
            case SCE_CAML_STRING:
                return s::string;
            case SCE_CAML_COMMENT:
            case SCE_CAML_COMMENT1:
            case SCE_CAML_COMMENT2:
            case SCE_CAML_COMMENT3:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_VB:
        case SCLEX_VBSCRIPT:
        case SCLEX_BLITZBASIC:
        case SCLEX_PUREBASIC:
        case SCLEX_FREEBASIC:
        case SCLEX_POWERBASIC:
            switch (style)
            {
            case SCE_B_IDENTIFIER:
                return s::identifier;
            case SCE_B_COMMENT:
                return s::comment;
            case SCE_B_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_HASKELL:
        case SCLEX_LITERATEHASKELL:
            switch (style)
            {
            case SCE_HA_STRING:
                return s::string;
            case SCE_HA_COMMENTLINE:
            case SCE_HA_COMMENTBLOCK:
            case SCE_HA_COMMENTBLOCK2:
            case SCE_HA_COMMENTBLOCK3:
                return s::comment;
            case SCE_HA_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_PHPSCRIPT:
        case SCLEX_TADS3:
            switch (style)
            {
            case SCE_T3_BLOCK_COMMENT:
            case SCE_T3_LINE_COMMENT:
                return s::comment;
            case SCE_T3_S_STRING:
            case SCE_T3_D_STRING:
            case SCE_T3_X_STRING:
                return s::string;
            case SCE_T3_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_REBOL:
            switch (style)
            {
            case SCE_REBOL_COMMENTLINE:
            case SCE_REBOL_COMMENTBLOCK:
                return s::comment;
            case SCE_REBOL_QUOTEDSTRING:
            case SCE_REBOL_BRACEDSTRING:
                return s::string;
            case SCE_REBOL_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_SMALLTALK:
            switch (style)
            {
            case SCE_ST_STRING:
            case SCE_ST_COMMENT:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_FLAGSHIP:
            switch (style)
            {
            case SCE_FS_COMMENT:
            case SCE_FS_COMMENTLINE:
            case SCE_FS_COMMENTDOC:
            case SCE_FS_COMMENTLINEDOC:
            case SCE_FS_COMMENTDOCKEYWORD:
            case SCE_FS_COMMENTDOCKEYWORDERROR:
            case SCE_FS_COMMENTDOC_C:
            case SCE_FS_COMMENTLINEDOC_C:
                return s::comment;
            case SCE_FS_STRING:
            case SCE_FS_STRING_C:
                return s::string;
            case SCE_FS_IDENTIFIER:
            case SCE_FS_IDENTIFIER_C:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_CSOUND:
            switch (style)
            {
            case SCE_CSOUND_COMMENT:
            case SCE_CSOUND_COMMENTBLOCK:
                return s::comment;
            case SCE_CSOUND_IDENTIFIER:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_INNOSETUP:
            switch (style)
            {
            case SCE_INNO_COMMENT:
            case SCE_INNO_COMMENT_PASCAL:
                return s::comment;
            case SCE_INNO_IDENTIFIER:
                return s::identifier;
            case SCE_INNO_STRING_DOUBLE:
            case SCE_INNO_STRING_SINGLE:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_OPAL:
            switch (style)
            {
            case SCE_OPAL_COMMENT_BLOCK:
            case SCE_OPAL_COMMENT_LINE:
                return s::comment;
            case SCE_OPAL_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_SPICE:
            switch (style)
            {
            case SCE_SPICE_IDENTIFIER:
                return s::identifier;
            case SCE_SPICE_COMMENTLINE:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_D:
            switch (style)
            {
            case SCE_D_IDENTIFIER:
                return s::identifier;
            case SCE_D_COMMENT:
            case SCE_D_COMMENTLINE:
            case SCE_D_COMMENTDOC:
            case SCE_D_COMMENTNESTED:
                return s::comment;
            case SCE_D_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_CMAKE:
            switch (style)
            {
            case SCE_CMAKE_STRINGDQ:
            case SCE_CMAKE_STRINGLQ:
            case SCE_CMAKE_STRINGRQ:
            case SCE_CMAKE_STRINGVAR:
                return s::string;
            case SCE_CMAKE_VARIABLE:
                return s::identifier;
            case SCE_CMAKE_COMMENT:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_GAP:
            switch (style)
            {
            case SCE_GAP_IDENTIFIER:
                return s::identifier;
            case SCE_GAP_COMMENT:
                return s::comment;
            case SCE_GAP_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_PLM:
            switch (style)
            {
            case SCE_PLM_IDENTIFIER:
                return s::identifier;
            case SCE_PLM_COMMENT:
                return s::comment;
            case SCE_PLM_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_PROGRESS:
            switch (style)
            {
            case SCE_4GL_STRING:
            case SCE_4GL_STRING_:
                return s::string;
            case SCE_4GL_COMMENT1:
            case SCE_4GL_COMMENT2:
            case SCE_4GL_COMMENT3:
            case SCE_4GL_COMMENT4:
            case SCE_4GL_COMMENT5:
            case SCE_4GL_COMMENT6:
            case SCE_4GL_COMMENT1_:
            case SCE_4GL_COMMENT2_:
            case SCE_4GL_COMMENT3_:
            case SCE_4GL_COMMENT4_:
            case SCE_4GL_COMMENT5_:
            case SCE_4GL_COMMENT6_:
                return s::comment;
            case SCE_4GL_IDENTIFIER:
            case SCE_4GL_IDENTIFIER_:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_ABAQUS:
            switch (style)
            {
            case SCE_ABAQUS_COMMENT:
            case SCE_ABAQUS_COMMENTBLOCK:
                return s::comment;
            case SCE_ABAQUS_STRING:
                return s::string;
            case SCE_ABAQUS_FUNCTION:
                return s::identifier;
            default:
                return s::unknown;
            }
        case SCLEX_ASYMPTOTE:
            switch (style)
            {
            case SCE_ASY_IDENTIFIER:
                return s::identifier;
            case SCE_ASY_COMMENT:
            case SCE_ASY_COMMENTLINE:
                return s::comment;
            case SCE_ASY_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_R:
            switch (style)
            {
            case SCE_R_IDENTIFIER:
                return s::identifier;
            case SCE_R_COMMENT:
                return s::comment;
            case SCE_R_STRING:
            case SCE_R_STRING2:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_MAGIK:
            switch (style)
            {
            case SCE_MAGIK_IDENTIFIER:
                return s::identifier;
            case SCE_MAGIK_COMMENT:
            case SCE_MAGIK_HYPER_COMMENT:
                return s::comment;
            case SCE_MAGIK_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_POWERSHELL:
            switch (style)
            {
            case SCE_POWERSHELL_IDENTIFIER:
            case SCE_POWERSHELL_VARIABLE:
            case SCE_POWERSHELL_FUNCTION:
                return s::identifier;
            case SCE_POWERSHELL_COMMENT:
                return s::comment;
            case SCE_POWERSHELL_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_MYSQL:
            switch (style)
            {
            case SCE_MYSQL_IDENTIFIER:
            case SCE_MYSQL_FUNCTION:
            case SCE_MYSQL_VARIABLE:
            case SCE_MYSQL_QUOTEDIDENTIFIER:
                return s::identifier;
            case SCE_MYSQL_COMMENT:
            case SCE_MYSQL_COMMENTLINE:
                return s::comment;
            case SCE_MYSQL_STRING:
            case SCE_MYSQL_SQSTRING:
            case SCE_MYSQL_DQSTRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_PO:
            switch (style)
            {
            case SCE_PO_COMMENT:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_TAL:
        case SCLEX_COBOL:
        case SCLEX_TACL:
            return s::unknown;
        case SCLEX_SORCUS:
            switch (style)
            {
            case SCE_SORCUS_IDENTIFIER:
                return s::identifier;
            case SCE_SORCUS_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_POWERPRO:
            switch (style)
            {
            case SCE_POWERPRO_IDENTIFIER:
            case SCE_POWERPRO_FUNCTION:
                return s::identifier;
            case SCE_POWERPRO_COMMENTBLOCK:
            case SCE_POWERPRO_COMMENTLINE:
                return s::comment;
            case SCE_POWERPRO_DOUBLEQUOTEDSTRING:
            case SCE_POWERPRO_SINGLEQUOTEDSTRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_NIMROD:
        case SCLEX_SML:
            switch (style)
            {
            case SCE_SML_IDENTIFIER:
                return s::identifier;
            case SCE_SML_STRING:
                return s::string;
            case SCE_SML_COMMENT:
            case SCE_SML_COMMENT1:
            case SCE_SML_COMMENT2:
            case SCE_SML_COMMENT3:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_MARKDOWN: // TODO: check markdown. There is some text there definetily
            return s::unknown;
        case SCLEX_TXT2TAGS:
            switch (style)
            {
            case SCE_TXT2TAGS_COMMENT:
                return s::comment;
            default:
                return s::unknown;
            }
        case SCLEX_A68K:
            switch (style)
            {
            case SCE_A68K_IDENTIFIER:
                return s::identifier;
            case SCE_A68K_COMMENT:
                return s::comment;
            case SCE_A68K_STRING1:
            case SCE_A68K_STRING2:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_MODULA:
            switch (style)
            {
            case SCE_MODULA_COMMENT:
                return s::comment;
            case SCE_MODULA_STRING:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_COFFEESCRIPT:
            switch (style)
            {
            case SCE_COFFEESCRIPT_IDENTIFIER:
                return s::identifier;
            case SCE_COFFEESCRIPT_COMMENT:
            case SCE_COFFEESCRIPT_COMMENTLINE:
            case SCE_COFFEESCRIPT_COMMENTDOC:
            case SCE_COFFEESCRIPT_COMMENTLINEDOC:
            case SCE_COFFEESCRIPT_COMMENTDOCKEYWORD:
            case SCE_COFFEESCRIPT_COMMENTDOCKEYWORDERROR:
            case SCE_COFFEESCRIPT_COMMENTBLOCK:
                return s::comment;
            case SCE_COFFEESCRIPT_CHARACTER:
            case SCE_COFFEESCRIPT_STRING:
            case SCE_COFFEESCRIPT_STRINGEOL:
            case SCE_COFFEESCRIPT_STRINGRAW:
                return s::string;
            default:
                return s::unknown;
            }
        case SCLEX_TCMD:
            {
                switch (style)
                {
                case SCE_TCMD_COMMENT:
                    return s::comment;
                case SCE_TCMD_IDENTIFIER:
                    return s::identifier;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_AVS:
            {
                switch (style)
                {
                case SCE_AVS_COMMENTLINE:
                case SCE_AVS_COMMENTBLOCK:
                case SCE_AVS_COMMENTBLOCKN:
                    return s::comment;
                case SCE_AVS_IDENTIFIER:
                case SCE_AVS_FUNCTION:
                    return s::identifier;
                case SCE_AVS_STRING:
                    return s::string;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_ECL:
            {
                switch (style)
                {
                case SCE_ECL_COMMENT:
                case SCE_ECL_COMMENTLINE:
                case SCE_ECL_COMMENTLINEDOC:
                case SCE_ECL_COMMENTDOCKEYWORD:
                case SCE_ECL_COMMENTDOCKEYWORDERROR:
                case SCE_ECL_COMMENTDOC:
                    return s::comment;
                case SCE_ECL_IDENTIFIER:
                    return s::identifier;
                case SCE_ECL_STRING:
                case SCE_ECL_CHARACTER:
                    return s::string;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_OSCRIPT:
            {
                switch (style)
                {
                case SCE_OSCRIPT_BLOCK_COMMENT:
                case SCE_OSCRIPT_DOC_COMMENT:
                case SCE_OSCRIPT_LINE_COMMENT:
                    return s::comment;
                case SCE_OSCRIPT_DOUBLEQUOTE_STRING:
                case SCE_OSCRIPT_SINGLEQUOTE_STRING:
                    return s::string;
                case SCE_OSCRIPT_IDENTIFIER:
                case SCE_OSCRIPT_FUNCTION:
                    return s::identifier;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_VISUALPROLOG:
            {
                switch (style)
                {
                case SCE_VISUALPROLOG_CHARACTER:
                case SCE_VISUALPROLOG_COMMENT_BLOCK:
                case SCE_VISUALPROLOG_COMMENT_KEY:
                case SCE_VISUALPROLOG_COMMENT_KEY_ERROR:
                case SCE_VISUALPROLOG_COMMENT_LINE:
                    return s::comment;
                case SCE_VISUALPROLOG_STRING:
                case SCE_VISUALPROLOG_STRING_ESCAPE:
                case SCE_VISUALPROLOG_STRING_ESCAPE_ERROR:
                case SCE_VISUALPROLOG_STRING_EOL_OPEN:
                case SCE_VISUALPROLOG_STRING_VERBATIM:
                case SCE_VISUALPROLOG_STRING_VERBATIM_SPECIAL:
                case SCE_VISUALPROLOG_STRING_VERBATIM_EOL:
                    return s::string;
                case SCE_VISUALPROLOG_IDENTIFIER:
                case SCE_VISUALPROLOG_VARIABLE:
                    return s::identifier;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_STTXT:
            {
                switch (style)
                {
                case SCE_STTXT_CHARACTER:
                case SCE_STTXT_STRING1:
                case SCE_STTXT_STRING2:
                    return s::string;
                case SCE_STTXT_COMMENT:
                case SCE_STTXT_COMMENTLINE:
                    return s::comment;
                case SCE_STTXT_IDENTIFIER:
                case SCE_STTXT_FUNCTION:
                    return s::identifier;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_KVIRC:
            {
                switch (style)
                {
                case SCE_KVIRC_STRING:
                    return s::string;
                case SCE_KVIRC_COMMENT:
                case SCE_KVIRC_COMMENTBLOCK:
                    return s::comment;
                case SCE_KVIRC_VARIABLE:
                case SCE_KVIRC_FUNCTION:
                case SCE_KVIRC_STRING_VARIABLE:
                case SCE_KVIRC_STRING_FUNCTION:
                    return s::identifier;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_RUST:
            {
                switch (style)
                {
                case SCE_RUST_CHARACTER:
                case SCE_RUST_STRING:
                case SCE_RUST_STRINGR:
                    return s::string;
                case SCE_RUST_COMMENTBLOCK:
                case SCE_RUST_COMMENTBLOCKDOC:
                case SCE_RUST_COMMENTLINE:
                case SCE_RUST_COMMENTLINEDOC:
                    return s::comment;
                case SCE_RUST_IDENTIFIER:
                    return s::identifier;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_DMAP:
            {
                switch (style)
                {
                case SCE_DMAP_COMMENT:
                    return s::comment;
                case SCE_DMAP_STRING1:
                case SCE_DMAP_STRING2:
                case SCE_DMAP_STRINGEOL:
                    return s::string;
                case SCE_DMAP_IDENTIFIER:
                    return s::identifier;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_DMIS:
            {
                switch (style)
                {
                case SCE_DMIS_COMMENT:
                    return s::comment;
                case SCE_DMIS_STRING:
                    return s::string;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_REGISTRY:
            {
                switch (style)
                {
                case SCE_REG_COMMENT:
                    return s::comment;
                case SCE_REG_STRING:
                    return s::string;
                default:
                    return s::unknown;
                }
            }
        case SCLEX_BIBTEX:
            {
                switch (style)
                {
                case SCE_BIBTEX_COMMENT:
                    return s::comment;
                default:
                    return s::unknown;
                }
            }
        };
        return s::unknown;
    }
} // namespace SciUtils
