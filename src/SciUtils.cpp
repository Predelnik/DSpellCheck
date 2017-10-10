#include "SciUtils.h"
#include "SciLexer.h"

bool SciUtils::is_comment_or_string(LRESULT lexer, LRESULT style)
{
    switch (lexer)
    {
    case SCLEX_CONTAINER:
    case SCLEX_NULL:
        return true;
    case SCLEX_USER:
        switch (style)
        {
        case SCE_USER_STYLE_COMMENT:
        case SCE_USER_STYLE_COMMENTLINE:
        case SCE_USER_STYLE_DELIMITER1:
        case SCE_USER_STYLE_DELIMITER2:
        case SCE_USER_STYLE_DELIMITER3:
        case SCE_USER_STYLE_DELIMITER4:
        case SCE_USER_STYLE_DELIMITER5:
        case SCE_USER_STYLE_DELIMITER6:
        case SCE_USER_STYLE_DELIMITER7:
        case SCE_USER_STYLE_DELIMITER8:
            return TRUE;
        default:
            return FALSE;
        };
    case SCLEX_PYTHON:
        switch (style)
        {
        case SCE_P_COMMENTLINE:
        case SCE_P_COMMENTBLOCK:
        case SCE_P_STRING:
        case SCE_P_TRIPLE:
        case SCE_P_TRIPLEDOUBLE:
            return true;
        default:
            return false;
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
        case SCE_C_STRING:
            return true;
        default:
            return false;
        };
    case SCLEX_HTML:
    case SCLEX_XML:
        switch (style)
        {
        case SCE_H_COMMENT:
        case SCE_H_DEFAULT:
        case SCE_H_TAGUNKNOWN:
        case SCE_H_DOUBLESTRING:
        case SCE_H_SINGLESTRING:
        case SCE_H_XCCOMMENT:
        case SCE_H_SGML_COMMENT:
        case SCE_HJ_COMMENT:
        case SCE_HJ_COMMENTLINE:
        case SCE_HJ_COMMENTDOC:
        case SCE_HJ_STRINGEOL:
        case SCE_HJA_COMMENT:
        case SCE_HJA_COMMENTLINE:
        case SCE_HJA_COMMENTDOC:
        case SCE_HJA_DOUBLESTRING:
        case SCE_HJA_SINGLESTRING:
        case SCE_HJA_STRINGEOL:
        case SCE_HB_COMMENTLINE:
        case SCE_HB_STRING:
        case SCE_HB_STRINGEOL:
        case SCE_HBA_COMMENTLINE:
        case SCE_HBA_STRING:
        case SCE_HP_COMMENTLINE:
        case SCE_HP_STRING:
        case SCE_HPA_COMMENTLINE:
        case SCE_HPA_STRING:
        case SCE_HPHP_HSTRING:
        case SCE_HPHP_SIMPLESTRING:
        case SCE_HPHP_COMMENT:
        case SCE_HPHP_COMMENTLINE:
            return true;
        default:
            return false;
        }
    case SCLEX_PERL:
        switch (style)
        {
        case SCE_PL_COMMENTLINE:
        case SCE_PL_STRING_Q:
        case SCE_PL_STRING_QQ:
        case SCE_PL_STRING_QX:
        case SCE_PL_STRING_QR:
        case SCE_PL_STRING_QW:
            return true;
        default:
            return false;
        };
    case SCLEX_SQL:
        switch (style)
        {
        case SCE_SQL_COMMENT:
        case SCE_SQL_COMMENTLINE:
        case SCE_SQL_COMMENTDOC:
        case SCE_SQL_STRING:
        case SCE_SQL_COMMENTLINEDOC:
            return true;
        default:
            return false;
        }
    case SCLEX_PROPERTIES:
        switch (style)
        {
        case SCE_PROPS_COMMENT:
            return true;
        default:
            return false;
        }
    case SCLEX_ERRORLIST:
        return false;
    case SCLEX_MAKEFILE:
        switch (style)
        {
        case SCE_MAKE_COMMENT:
            return true;
        default:
            return false;
        }
    case SCLEX_BATCH:
        switch (style)
        {
        case SCE_BAT_COMMENT:
            return true;
        default:
            return false;
        }
    case SCLEX_XCODE:
        return false;
    case SCLEX_LATEX:
        switch (style)
        {
        case SCE_L_DEFAULT:
        case SCE_L_COMMENT:
            return true;
        default:
            return false;
        }
    case SCLEX_LUA:
        switch (style)
        {
        case SCE_LUA_COMMENT:
        case SCE_LUA_COMMENTLINE:
        case SCE_LUA_COMMENTDOC:
        case SCE_LUA_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_DIFF:
        switch (style)
        {
        case SCE_DIFF_COMMENT:
            return true;
        default:
            return false;
        }
    case SCLEX_CONF:
        switch (style)
        {
        case SCE_CONF_COMMENT:
        case SCE_CONF_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_PASCAL:
        switch (style)
        {
        case SCE_PAS_COMMENT:
        case SCE_PAS_COMMENT2:
        case SCE_PAS_COMMENTLINE:
        case SCE_PAS_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_AVE:
        switch (style)
        {
        case SCE_AVE_COMMENT:
        case SCE_AVE_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_ADA:
        switch (style)
        {
        case SCE_ADA_STRING:
        case SCE_ADA_COMMENTLINE:
            return true;
        default:
            return false;
        }
    case SCLEX_LISP:
        switch (style)
        {
        case SCE_LISP_COMMENT:
        case SCE_LISP_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_RUBY:
        switch (style)
        {
        case SCE_RB_COMMENTLINE:
        case SCE_RB_STRING:
        case SCE_RB_STRING_Q:
        case SCE_RB_STRING_QQ:
        case SCE_RB_STRING_QX:
        case SCE_RB_STRING_QR:
        case SCE_RB_STRING_QW:
            return true;
        default:
            return false;
        }
    case SCLEX_EIFFEL:
    case SCLEX_EIFFELKW:
        switch (style)
        {
        case SCE_EIFFEL_COMMENTLINE:
        case SCE_EIFFEL_STRING:
            return true;
        default:
            return false;
        };
    case SCLEX_TCL:
        switch (style)
        {
        case SCE_TCL_COMMENT:
        case SCE_TCL_COMMENTLINE:
        case SCE_TCL_BLOCK_COMMENT:
        case SCE_TCL_IN_QUOTE:
            return true;
        default:
            return false;
        }
    case SCLEX_NNCRONTAB:
        switch (style)
        {
        case SCE_NNCRONTAB_COMMENT:
        case SCE_NNCRONTAB_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_BAAN:
        switch (style)
        {
        case SCE_BAAN_COMMENT:
        case SCE_BAAN_COMMENTDOC:
        case SCE_BAAN_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_MATLAB:
        switch (style)
        {
        case SCE_MATLAB_COMMENT:
        case SCE_MATLAB_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_SCRIPTOL:
        switch (style)
        {
        case SCE_SCRIPTOL_COMMENTLINE:
        case SCE_SCRIPTOL_COMMENTBLOCK:
        case SCE_SCRIPTOL_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_ASM:
        switch (style)
        {
        case SCE_ASM_COMMENT:
        case SCE_ASM_COMMENTBLOCK:
            return true;
        default:
            return false;
        }
    case SCLEX_CPPNOCASE:
    case SCLEX_FORTRAN:
    case SCLEX_F77:
        switch (style)
        {
        case SCE_F_COMMENT:
        case SCE_F_STRING1:
        case SCE_F_STRING2:
            return true;
        default:
            return false;
        }
    case SCLEX_CSS:
        switch (style)
        {
        case SCE_CSS_COMMENT:
        case SCE_CSS_DOUBLESTRING:
        case SCE_CSS_SINGLESTRING:
            return true;
        default:
            return false;
        }
    case SCLEX_POV:
        switch (style)
        {
        case SCE_POV_COMMENT:
        case SCE_POV_COMMENTLINE:
        case SCE_POV_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_LOUT:
        switch (style)
        {
        case SCE_LOUT_COMMENT:
        case SCE_LOUT_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_ESCRIPT:
        switch (style)
        {
        case SCE_ESCRIPT_COMMENT:
        case SCE_ESCRIPT_COMMENTLINE:
        case SCE_ESCRIPT_COMMENTDOC:
        case SCE_ESCRIPT_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_PS:
        switch (style)
        {
        case SCE_PS_COMMENT:
        case SCE_PS_DSC_COMMENT:
        case SCE_PS_TEXT:
            return true;
        default:
            return false;
        }
    case SCLEX_NSIS:
        switch (style)
        {
        case SCE_NSIS_COMMENT:
        case SCE_NSIS_STRINGDQ:
        case SCE_NSIS_STRINGLQ:
        case SCE_NSIS_STRINGRQ:
            return true;
        default:
            return false;
        }
    case SCLEX_MMIXAL:
        switch (style)
        {
        case SCE_MMIXAL_COMMENT:
        case SCE_MMIXAL_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_CLW:
        switch (style)
        {
        case SCE_CLW_COMMENT:
        case SCE_CLW_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_CLWNOCASE:
    case SCLEX_LOT:
        return false;
    case SCLEX_YAML:
        switch (style)
        {
        case SCE_YAML_COMMENT:
        case SCE_YAML_TEXT:
            return true;
        default:
            return false;
        }
    case SCLEX_TEX:
        switch (style)
        {
        case SCE_TEX_TEXT:
            return true;
        default:
            return false;
        }
    case SCLEX_METAPOST:
        switch (style)
        {
        case SCE_METAPOST_TEXT:
        case SCE_METAPOST_DEFAULT:
            return true;
        default:
            return false;
        }
    case SCLEX_FORTH:
        switch (style)
        {
        case SCE_FORTH_COMMENT:
        case SCE_FORTH_COMMENT_ML:
        case SCE_FORTH_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_ERLANG:
        switch (style)
        {
        case SCE_ERLANG_COMMENT:
        case SCE_ERLANG_STRING:
        case SCE_ERLANG_COMMENT_FUNCTION:
        case SCE_ERLANG_COMMENT_MODULE:
        case SCE_ERLANG_COMMENT_DOC:
        case SCE_ERLANG_COMMENT_DOC_MACRO:
            return true;
        default:
            return false;
        }
    case SCLEX_OCTAVE:
    case SCLEX_MSSQL:
        switch (style)
        {
        case SCE_MSSQL_COMMENT:
        case SCE_MSSQL_LINE_COMMENT:
        case SCE_MSSQL_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_VERILOG:
        switch (style)
        {
        case SCE_V_COMMENT:
        case SCE_V_COMMENTLINE:
        case SCE_V_COMMENTLINEBANG:
        case SCE_V_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_KIX:
        switch (style)
        {
        case SCE_KIX_COMMENT:
        case SCE_KIX_STRING1:
        case SCE_KIX_STRING2:
            return true;
        default:
            return false;
        }
    case SCLEX_GUI4CLI:
        switch (style)
        {
        case SCE_GC_COMMENTLINE:
        case SCE_GC_COMMENTBLOCK:
        case SCE_GC_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_SPECMAN:
        switch (style)
        {
        case SCE_SN_COMMENTLINE:
        case SCE_SN_COMMENTLINEBANG:
        case SCE_SN_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_AU3:
        switch (style)
        {
        case SCE_AU3_COMMENT:
        case SCE_AU3_COMMENTBLOCK:
        case SCE_AU3_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_APDL:
        switch (style)
        {
        case SCE_APDL_COMMENT:
        case SCE_APDL_COMMENTBLOCK:
        case SCE_APDL_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_BASH:
        switch (style)
        {
        case SCE_SH_COMMENTLINE:
        case SCE_SH_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_ASN1:
        switch (style)
        {
        case SCE_ASN1_COMMENT:
        case SCE_ASN1_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_VHDL:
        switch (style)
        {
        case SCE_VHDL_COMMENT:
        case SCE_VHDL_COMMENTLINEBANG:
        case SCE_VHDL_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_CAML:
        switch (style)
        {
        case SCE_CAML_STRING:
        case SCE_CAML_COMMENT:
        case SCE_CAML_COMMENT1:
        case SCE_CAML_COMMENT2:
        case SCE_CAML_COMMENT3:
            return true;
        default:
            return false;
        }
    case SCLEX_VB:
    case SCLEX_VBSCRIPT:
    case SCLEX_BLITZBASIC:
    case SCLEX_PUREBASIC:
    case SCLEX_FREEBASIC:
    case SCLEX_POWERBASIC:
        switch (style)
        {
        case SCE_B_COMMENT:
        case SCE_B_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_HASKELL:
        switch (style)
        {
        case SCE_HA_STRING:
        case SCE_HA_COMMENTLINE:
        case SCE_HA_COMMENTBLOCK:
        case SCE_HA_COMMENTBLOCK2:
        case SCE_HA_COMMENTBLOCK3:
            return true;
        default:
            return false;
        }
    case SCLEX_PHPSCRIPT:
    case SCLEX_TADS3:
        switch (style)
        {
        case SCE_T3_BLOCK_COMMENT:
        case SCE_T3_LINE_COMMENT:
        case SCE_T3_S_STRING:
        case SCE_T3_D_STRING:
        case SCE_T3_X_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_REBOL:
        switch (style)
        {
        case SCE_REBOL_COMMENTLINE:
        case SCE_REBOL_COMMENTBLOCK:
        case SCE_REBOL_QUOTEDSTRING:
        case SCE_REBOL_BRACEDSTRING:
            return true;
        default:
            return false;
        }
    case SCLEX_SMALLTALK:
        switch (style)
        {
        case SCE_ST_STRING:
        case SCE_ST_COMMENT:
            return true;
        default:
            return false;
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
        case SCE_FS_STRING:
        case SCE_FS_COMMENTDOC_C:
        case SCE_FS_COMMENTLINEDOC_C:
        case SCE_FS_STRING_C:
            return true;
        default:
            return false;
        }
    case SCLEX_CSOUND:
        switch (style)
        {
        case SCE_CSOUND_COMMENT:
        case SCE_CSOUND_COMMENTBLOCK:
            return true;
        default:
            return false;
        }
    case SCLEX_INNOSETUP:
        switch (style)
        {
        case SCE_INNO_COMMENT:
        case SCE_INNO_COMMENT_PASCAL:
        case SCE_INNO_STRING_DOUBLE:
        case SCE_INNO_STRING_SINGLE:
            return true;
        default:
            return false;
        }
    case SCLEX_OPAL:
        switch (style)
        {
        case SCE_OPAL_COMMENT_BLOCK:
        case SCE_OPAL_COMMENT_LINE:
        case SCE_OPAL_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_SPICE:
        switch (style)
        {
        case SCE_SPICE_COMMENTLINE:
            return true;
        default:
            return false;
        }
    case SCLEX_D:
        switch (style)
        {
        case SCE_D_COMMENT:
        case SCE_D_COMMENTLINE:
        case SCE_D_COMMENTDOC:
        case SCE_D_COMMENTNESTED:
        case SCE_D_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_CMAKE:
        switch (style)
        {
        case SCE_CMAKE_COMMENT:
            return true;
        default:
            return false;
        }
    case SCLEX_GAP:
        switch (style)
        {
        case SCE_GAP_COMMENT:
        case SCE_GAP_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_PLM:
        switch (style)
        {
        case SCE_PLM_COMMENT:
        case SCE_PLM_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_PROGRESS:
        switch (style)
        {
        case SCE_4GL_STRING:
        case SCE_4GL_COMMENT1:
        case SCE_4GL_COMMENT2:
        case SCE_4GL_COMMENT3:
        case SCE_4GL_COMMENT4:
        case SCE_4GL_COMMENT5:
        case SCE_4GL_COMMENT6:
        case SCE_4GL_STRING_:
        case SCE_4GL_COMMENT1_:
        case SCE_4GL_COMMENT2_:
        case SCE_4GL_COMMENT3_:
        case SCE_4GL_COMMENT4_:
        case SCE_4GL_COMMENT5_:
        case SCE_4GL_COMMENT6_:
            return true;
        default:
            return false;
        }
    case SCLEX_ABAQUS:
        switch (style)
        {
        case SCE_ABAQUS_COMMENT:
        case SCE_ABAQUS_COMMENTBLOCK:
        case SCE_ABAQUS_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_ASYMPTOTE:
        switch (style)
        {
        case SCE_ASY_COMMENT:
        case SCE_ASY_COMMENTLINE:
        case SCE_ASY_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_R:
        switch (style)
        {
        case SCE_R_COMMENT:
        case SCE_R_STRING:
        case SCE_R_STRING2:
            return true;
        default:
            return false;
        }
    case SCLEX_MAGIK:
        switch (style)
        {
        case SCE_MAGIK_COMMENT:
        case SCE_MAGIK_HYPER_COMMENT:
        case SCE_MAGIK_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_POWERSHELL:
        switch (style)
        {
        case SCE_POWERSHELL_COMMENT:
        case SCE_POWERSHELL_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_MYSQL:
        switch (style)
        {
        case SCE_MYSQL_COMMENT:
        case SCE_MYSQL_COMMENTLINE:
        case SCE_MYSQL_STRING:
        case SCE_MYSQL_SQSTRING:
        case SCE_MYSQL_DQSTRING:
            return true;
        default:
            return false;
        }
    case SCLEX_PO:
        switch (style)
        {
        case SCE_PO_COMMENT:
            return true;
        default:
            return false;
        }
    case SCLEX_TAL:
    case SCLEX_COBOL:
    case SCLEX_TACL:
    case SCLEX_SORCUS:
        switch (style)
        {
        case SCE_SORCUS_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_POWERPRO:
        switch (style)
        {
        case SCE_POWERPRO_COMMENTBLOCK:
        case SCE_POWERPRO_COMMENTLINE:
        case SCE_POWERPRO_DOUBLEQUOTEDSTRING:
        case SCE_POWERPRO_SINGLEQUOTEDSTRING:
            return true;
        default:
            return false;
        }
    case SCLEX_NIMROD:
    case SCLEX_SML:
        switch (style)
        {
        case SCE_SML_STRING:
        case SCE_SML_COMMENT:
        case SCE_SML_COMMENT1:
        case SCE_SML_COMMENT2:
        case SCE_SML_COMMENT3:
            return true;
        default:
            return false;
        }
    case SCLEX_MARKDOWN:
        return false;
    case SCLEX_TXT2TAGS:
        switch (style)
        {
        case SCE_TXT2TAGS_COMMENT:
            return true;
        default:
            return false;
        }
    case SCLEX_A68K:
        switch (style)
        {
        case SCE_A68K_COMMENT:
        case SCE_A68K_STRING1:
        case SCE_A68K_STRING2:
            return true;
        default:
            return false;
        }
    case SCLEX_MODULA:
        switch (style)
        {
        case SCE_MODULA_COMMENT:
        case SCE_MODULA_STRING:
            return true;
        default:
            return false;
        }
    case SCLEX_SEARCHRESULT:
        return false;
    };
    return true;
}
