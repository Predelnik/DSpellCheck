/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Aspell.h"
#include "CommonFunctions.h"
#include "LangList.h"
#include "MainDef.h"
#include "PluginInterface.h"
#include "Plugin.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "SciLexer.h"
#include "Scintilla.h"
#include "Suggestions.h"

#define DEFAULT_DELIMITERS _T (",.!?\":;{}()[]\\/=+-*<>|#$@%…")

SpellChecker::SpellChecker (const TCHAR *IniFilePathArg, SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
                            Suggestions *SuggestionsInstanceArg, LangList *LangListInstanceArg)
{
  CurrentPosition = 0;
  DelimUtf8 = 0;
  DelimUtf8Converted = 0;
  Speller = 0;
  IniFilePath = 0;
  Language = 0;
  MultiLanguages = 0;
  VisibleText = 0;
  DelimConverted = 0;
  VisibleTextLength = -1;
  SetString (IniFilePath, IniFilePathArg);
  SettingsDlgInstance = SettingsDlgInstanceArg;
  SuggestionsInstance = SuggestionsInstanceArg;
  NppDataInstance = NppDataInstanceArg;
  LangListInstance = LangListInstanceArg;
  AspellLoaded = FALSE;
  AutoCheckText = 0;
  MultiLangMode = 0;
  AspellReinitSettings ();
  AspellPath = 0;
  FileTypes = 0;
  CheckThose = 0;
  SBTrans = 0;
  SBSize = 0;
  CurWordList = 0;
}

SpellChecker::~SpellChecker ()
{
  AspellClear ();
  CLEAN_AND_ZERO_ARR (DelimConverted);
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DelimUtf8);
  CLEAN_AND_ZERO_ARR (Language);
  CLEAN_AND_ZERO_ARR (MultiLanguages);
  CLEAN_AND_ZERO_ARR (IniFilePath)
    CLEAN_AND_ZERO_ARR (AspellPath);
  CLEAN_AND_ZERO_ARR (VisibleText);
  CLEAN_AND_ZERO_ARR (FileTypes);
}

BOOL WINAPI SpellChecker::NotifyEvent (DWORD Event)
{
  switch (Event)
  {
  case  EID_FILL_DIALOGS:
    if (AspellLoaded)
      SettingsDlgInstance->GetSimpleDlg ()->AddAvalaibleLanguages (Language, MultiLanguages);

    SettingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (!AspellLoaded);
    SettingsDlgInstance->GetSimpleDlg ()->FillAspellInfo (AspellLoaded, AspellPath);
    SettingsDlgInstance->GetSimpleDlg ()->FillSugestionsNum (SuggestionsNum);
    SettingsDlgInstance->GetSimpleDlg ()->SetFileTypes (CheckThose, FileTypes);
    SettingsDlgInstance->GetSimpleDlg ()->SetCheckComments (CheckComments);
    SettingsDlgInstance->GetAdvancedDlg ()->FillDelimiters (DelimUtf8);
    SettingsDlgInstance->GetAdvancedDlg ()->setConversionOpts (IgnoreYo, ConvertSingleQuotes);
    SettingsDlgInstance->GetAdvancedDlg ()->SetUnderlineSettings (UnderlineColor, UnderlineStyle);
    SettingsDlgInstance->GetAdvancedDlg ()->SetIgnore (IgnoreNumbers, IgnoreCStart, IgnoreCHave, IgnoreCAll, Ignore_, IgnoreSEApostrophe);
    SettingsDlgInstance->GetAdvancedDlg ()->SetSuggBoxSettings (SBSize, SBTrans);
    SettingsDlgInstance->GetAdvancedDlg ()->SetBufferSize (BufferSize / 1024);
    break;
  case EID_APPLY_SETTINGS:
    SettingsDlgInstance->GetSimpleDlg ()->ApplySettings (this);
    SettingsDlgInstance->GetAdvancedDlg ()->ApplySettings (this);
    SaveSettings ();
    CheckFileName (); // Cause filters may change
    RefreshUnderlineStyle ();
    RecheckVisible ();
    break;
  case EID_APPLY_MULTI_LANG_SETTINGS:
    LangListInstance->ApplyChoice (this);
    break;

  case EID_HIDE_DIALOG:
    SettingsDlgInstance->display (false);
    break;
  case EID_LOAD_SETTINGS:
    LoadSettings ();
    break;
  case EID_RECHECK_VISIBLE:
    RecheckVisible ();
    break;
  case EID_SWITCH_AUTOCHECK:
    SwitchAutoCheck ();
    break;
  case EID_KILLTHREAD:
    return FALSE;
  case EID_INIT_SUGGESTIONS_BOX:
    InitSuggestionsBox ();
    break;
  case EID_SHOW_SUGGESTION_MENU:
    ShowSuggestionsMenu ();
    break;
  case EID_HIDE_SUGGESTIONS_BOX:
    HideSuggestionBox ();
    break;
  case EID_SET_SUGGESTIONS_BOX_TRANSPARENCY:
    SetSuggestionsBoxTransparency ();
    break;
  case EID_DEFAULT_DELIMITERS:
    SetDefaultDelimiters ();
    break;
  case EID_FIND_NEXT_MISTAKE:
    FindNextMistake ();
    break;
  case EID_FIND_PREV_MISTAKE:
    FindPrevMistake ();
    break;
  case EID_RECHECK_MODIFIED_ZONE:
    GetLimitsAndRecheckModified ();
    break;
  case EID_CHECK_FILE_NAME:
    CheckFileName ();
    break;
  case EID_WRITE_SETTING:
    WriteSetting ();
    break;
    /*
    case EID_APPLYMENUACTION:
    ApplyMenuActions ();
    break;
    */
  }
  return TRUE;
}

/*
void SpellChecker::ApplyMenuActions ()
{
}
*/

// For now just int option, later maybe choose option type in wParam
void SpellChecker::WriteSetting ()
{
  MSG Msg;
  GetMessage (&Msg, 0, 0, 0);
  if (Msg.message != TM_SET_SETTING)
    return;

  std::pair<TCHAR *, DWORD> *x = (std::pair<TCHAR *, DWORD> *) Msg.lParam;
  SaveToIni (x->first, LOWORD (x->second), HIWORD (x->second));
  CLEAN_AND_ZERO_ARR (x->first);
  CLEAN_AND_ZERO (x);
}

void SpellChecker::SetCheckComments (BOOL Value)
{
  CheckComments = Value;
}

void SpellChecker::CheckFileName ()
{
  TCHAR *Context = 0;
  TCHAR *Token = 0;
  TCHAR *FileTypesCopy = 0;
  TCHAR FullPath[MAX_PATH];
  SetString (FileTypesCopy, FileTypes);
  Token = _tcstok_s (FileTypesCopy, _T(";"), &Context);
  CheckTextEnabled = !CheckThose;
  ::SendMessage(NppDataInstance->_nppHandle, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM) FullPath);

  while (Token)
  {
    if (CheckThose)
    {
      CheckTextEnabled = CheckTextEnabled || PathMatchSpec (FullPath, Token);
      if (CheckTextEnabled)
        break;
    }
    else
    {
      CheckTextEnabled &= CheckTextEnabled && (!PathMatchSpec (FullPath, Token));
      if (!CheckTextEnabled)
        break;
    }
    Token = _tcstok_s (NULL, _T(";"), &Context);
  }
  CLEAN_AND_ZERO_ARR (FileTypesCopy);
  Lexer = SendMsgToEditor (NppDataInstance, SCI_GETLEXER);
}

int SpellChecker::GetStyle (int Pos)
{
  return SendMsgToEditor (NppDataInstance, SCI_GETSTYLEAT, Pos);
}
// Actually for all languages which operate mostly in strings it's better to check only comments
// TODO: Fix it
int SpellChecker::CheckWordInCommentOrString (int WordStart, int WordEnd)
{
  switch (Lexer)
  {
  case SCLEX_CONTAINER:
  case SCLEX_NULL:
    return TRUE;
  case SCLEX_PYTHON:
    switch (GetStyle (WordStart))
    {
    case SCE_P_COMMENTLINE:
    case SCE_P_STRING:
      return TRUE;
    default:
      return FALSE;
    };
  case SCLEX_CPP:
  case SCLEX_OBJC:
  case SCLEX_BULLANT:
    switch (GetStyle (WordStart))
    {
    case SCE_C_COMMENT:
    case SCE_C_COMMENTLINE:
    case SCE_C_COMMENTDOC:
    case SCE_C_STRING:
      return TRUE;
    default:
      return FALSE;
    };
  case SCLEX_HTML:
  case SCLEX_XML:
    switch (GetStyle (WordStart))
    {
    case SCE_H_COMMENT:
    case SCE_H_DEFAULT:
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
    case SCE_HPHP_SIMPLESTRING:
    case SCE_HPHP_COMMENT:
    case SCE_HPHP_COMMENTLINE:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PERL:
    switch (GetStyle (WordStart))
    {
    case SCE_PL_COMMENTLINE:
    case SCE_PL_STRING_Q:
    case SCE_PL_STRING_QQ:
    case SCE_PL_STRING_QX:
    case SCE_PL_STRING_QR:
    case SCE_PL_STRING_QW:
      return TRUE;
    default:
      return FALSE;
    };
  case SCLEX_SQL:
    switch (GetStyle (WordStart))
    {
    case SCE_SQL_COMMENT:
    case SCE_SQL_COMMENTLINE:
    case SCE_SQL_COMMENTDOC:
    case SCE_SQL_STRING:
    case SCE_SQL_COMMENTLINEDOC:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PROPERTIES:
    switch (GetStyle (WordStart))
    {
    case SCE_PROPS_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ERRORLIST:
    return FALSE;
  case SCLEX_MAKEFILE:
    switch (GetStyle (WordStart))
    {
    case SCE_MAKE_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_BATCH:
    switch (GetStyle (WordStart))
    {
    case SCE_BAT_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_XCODE:
    return FALSE;
  case SCLEX_LATEX:
    switch (GetStyle (WordStart))
    {
    case SCE_L_DEFAULT:
    case SCE_L_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_LUA:
    switch (GetStyle (WordStart))
    {
    case SCE_LUA_COMMENT:
    case SCE_LUA_COMMENTLINE:
    case SCE_LUA_COMMENTDOC:
    case SCE_LUA_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_DIFF:
    switch (GetStyle (WordStart))
    {
    case SCE_DIFF_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CONF:
    switch (GetStyle (WordStart))
    {
    case SCE_CONF_COMMENT:
    case SCE_CONF_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PASCAL:
    switch (GetStyle (WordStart))
    {
    case SCE_PAS_COMMENT:
    case SCE_PAS_COMMENT2:
    case SCE_PAS_COMMENTLINE:
    case SCE_PAS_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_AVE:
    switch (GetStyle (WordStart))
    {
    case SCE_AVE_COMMENT:
    case SCE_AVE_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ADA:
    switch (GetStyle (WordStart))
    {
    case SCE_ADA_STRING:
    case SCE_ADA_COMMENTLINE:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_LISP:
    switch (GetStyle (WordStart))
    {
    case SCE_LISP_COMMENT:
    case SCE_LISP_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_RUBY:
    switch (GetStyle (WordStart))
    {
    case SCE_RB_COMMENTLINE:
    case SCE_RB_STRING:
    case SCE_RB_STRING_Q:
    case SCE_RB_STRING_QQ:
    case SCE_RB_STRING_QX:
    case SCE_RB_STRING_QR:
    case SCE_RB_STRING_QW:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_EIFFEL:
  case SCLEX_EIFFELKW:
    switch (GetStyle (WordStart))
    {
    case SCE_EIFFEL_COMMENTLINE:
    case SCE_EIFFEL_STRING:
      return TRUE;
    default:
      return FALSE;
    };
  case SCLEX_TCL:
    switch (GetStyle (WordStart))
    {
    case SCE_TCL_COMMENT:
    case SCE_TCL_COMMENTLINE:
    case SCE_TCL_BLOCK_COMMENT:
    case SCE_TCL_IN_QUOTE:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_NNCRONTAB:
    switch (GetStyle (WordStart))
    {
    case SCE_NNCRONTAB_COMMENT:
    case SCE_NNCRONTAB_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_BAAN:
    switch (GetStyle (WordStart))
    {
    case SCE_BAAN_COMMENT:
    case SCE_BAAN_COMMENTDOC:
    case SCE_BAAN_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MATLAB:
    switch (GetStyle (WordStart))
    {
    case SCE_MATLAB_COMMENT:
    case SCE_MATLAB_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_SCRIPTOL:
    switch (GetStyle (WordStart))
    {
    case SCE_SCRIPTOL_COMMENTLINE:
    case SCE_SCRIPTOL_COMMENTBLOCK:
    case SCE_SCRIPTOL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ASM:
    switch (GetStyle (WordStart))
    {
    case SCE_ASM_COMMENT:
    case SCE_ASM_COMMENTBLOCK:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CPPNOCASE:
  case SCLEX_FORTRAN:
  case SCLEX_F77:
    switch (GetStyle (WordStart))
    {
    case SCE_F_COMMENT:
    case SCE_F_STRING1:
    case SCE_F_STRING2:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CSS:
    switch (GetStyle (WordStart))
    {
    case SCE_CSS_COMMENT:
    case SCE_CSS_DOUBLESTRING:
    case SCE_CSS_SINGLESTRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_POV:
    switch (GetStyle (WordStart))
    {
    case SCE_POV_COMMENT:
    case SCE_POV_COMMENTLINE:
    case SCE_POV_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_LOUT:
    switch (GetStyle (WordStart))
    {
    case SCE_LOUT_COMMENT:
    case SCE_LOUT_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ESCRIPT:
    switch (GetStyle (WordStart))
    {
    case SCE_ESCRIPT_COMMENT:
    case SCE_ESCRIPT_COMMENTLINE:
    case SCE_ESCRIPT_COMMENTDOC:
    case SCE_ESCRIPT_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PS:
    switch (GetStyle (WordStart))
    {
    case SCE_PS_COMMENT:
    case SCE_PS_DSC_COMMENT:
    case SCE_PS_TEXT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_NSIS:
    switch (GetStyle (WordStart))
    {
    case SCE_NSIS_COMMENT:
    case SCE_NSIS_STRINGDQ:
    case SCE_NSIS_STRINGLQ:
    case SCE_NSIS_STRINGRQ:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MMIXAL:
    switch (GetStyle (WordStart))
    {
    case SCE_MMIXAL_COMMENT:
    case SCE_MMIXAL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CLW:
    switch (GetStyle (WordStart))
    {
    case SCE_CLW_COMMENT:
    case SCE_CLW_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CLWNOCASE:
  case SCLEX_LOT:
    return FALSE;
  case SCLEX_YAML:
    switch (GetStyle (WordStart))
    {
    case SCE_YAML_COMMENT:
    case SCE_YAML_TEXT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_TEX:
    switch (GetStyle (WordStart))
    {
    case SCE_TEX_TEXT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_METAPOST:
    switch (GetStyle (WordStart))
    {
    case SCE_METAPOST_TEXT:
    case SCE_METAPOST_DEFAULT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_FORTH:
    switch (GetStyle (WordStart))
    {
    case SCE_FORTH_COMMENT:
    case SCE_FORTH_COMMENT_ML:
    case SCE_FORTH_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ERLANG:
    switch (GetStyle (WordStart))
    {
    case SCE_ERLANG_COMMENT:
    case SCE_ERLANG_STRING:
    case SCE_ERLANG_COMMENT_FUNCTION:
    case SCE_ERLANG_COMMENT_MODULE:
    case SCE_ERLANG_COMMENT_DOC:
    case SCE_ERLANG_COMMENT_DOC_MACRO:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_OCTAVE:
  case SCLEX_MSSQL:
    switch (GetStyle (WordStart))
    {
    case SCE_MSSQL_COMMENT:
    case SCE_MSSQL_LINE_COMMENT:
    case SCE_MSSQL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_VERILOG:
    switch (GetStyle (WordStart))
    {
    case SCE_V_COMMENT:
    case SCE_V_COMMENTLINE:
    case SCE_V_COMMENTLINEBANG:
    case SCE_V_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_KIX:
    switch (GetStyle (WordStart))
    {
    case SCE_KIX_COMMENT:
    case SCE_KIX_STRING1:
    case SCE_KIX_STRING2:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_GUI4CLI:
    switch (GetStyle (WordStart))
    {
    case SCE_GC_COMMENTLINE:
    case SCE_GC_COMMENTBLOCK:
    case SCE_GC_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_SPECMAN:
    switch (GetStyle (WordStart))
    {
    case SCE_SN_COMMENTLINE:
    case SCE_SN_COMMENTLINEBANG:
    case SCE_SN_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_AU3:
    switch (GetStyle (WordStart))
    {
    case SCE_AU3_COMMENT:
    case SCE_AU3_COMMENTBLOCK:
    case SCE_AU3_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_APDL:
    switch (GetStyle (WordStart))
    {
    case SCE_APDL_COMMENT:
    case SCE_APDL_COMMENTBLOCK:
    case SCE_APDL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_BASH:
    switch (GetStyle (WordStart))
    {
    case SCE_SH_COMMENTLINE:
    case SCE_SH_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ASN1:
    switch (GetStyle (WordStart))
    {
    case SCE_ASN1_COMMENT:
    case SCE_ASN1_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_VHDL:
    switch (GetStyle (WordStart))
    {
    case SCE_VHDL_COMMENT:
    case SCE_VHDL_COMMENTLINEBANG:
    case SCE_VHDL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CAML:
    switch (GetStyle (WordStart))
    {
    case SCE_CAML_STRING:
    case SCE_CAML_COMMENT:
    case SCE_CAML_COMMENT1:
    case SCE_CAML_COMMENT2:
    case SCE_CAML_COMMENT3:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_VB:
  case SCLEX_VBSCRIPT:
  case SCLEX_BLITZBASIC:
  case SCLEX_PUREBASIC:
  case SCLEX_FREEBASIC:
  case SCLEX_POWERBASIC:
    switch (GetStyle (WordStart))
    {
    case SCE_B_COMMENT:
    case SCE_B_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_HASKELL:
    switch (GetStyle (WordStart))
    {
    case SCE_HA_STRING:
    case SCE_HA_COMMENTLINE:
    case SCE_HA_COMMENTBLOCK:
    case SCE_HA_COMMENTBLOCK2:
    case SCE_HA_COMMENTBLOCK3:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PHPSCRIPT:
  case SCLEX_TADS3:
    switch (GetStyle (WordStart))
    {
    case SCE_T3_BLOCK_COMMENT:
    case SCE_T3_LINE_COMMENT:
    case SCE_T3_S_STRING:
    case SCE_T3_D_STRING:
    case SCE_T3_X_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_REBOL:
    switch (GetStyle (WordStart))
    {
    case SCE_REBOL_COMMENTLINE:
    case SCE_REBOL_COMMENTBLOCK:
    case SCE_REBOL_QUOTEDSTRING:
    case SCE_REBOL_BRACEDSTRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_SMALLTALK:
    switch (GetStyle (WordStart))
    {
    case SCE_ST_STRING:
    case SCE_ST_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_FLAGSHIP:
    switch (GetStyle (WordStart))
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
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CSOUND:
    switch (GetStyle (WordStart))
    {
    case SCE_CSOUND_COMMENT:
    case SCE_CSOUND_COMMENTBLOCK:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_INNOSETUP:
    switch (GetStyle (WordStart))
    {
    case SCE_INNO_COMMENT:
    case SCE_INNO_COMMENT_PASCAL:
    case SCE_INNO_STRING_DOUBLE:
    case SCE_INNO_STRING_SINGLE:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_OPAL:
    switch (GetStyle (WordStart))
    {
    case SCE_OPAL_COMMENT_BLOCK:
    case SCE_OPAL_COMMENT_LINE:
    case SCE_OPAL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_SPICE:
    switch (GetStyle (WordStart))
    {
    case SCE_SPICE_COMMENTLINE:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_D:
    switch (GetStyle (WordStart))
    {
    case SCE_D_COMMENT:
    case SCE_D_COMMENTLINE:
    case SCE_D_COMMENTDOC:
    case SCE_D_COMMENTNESTED:
    case SCE_D_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CMAKE:
    switch (GetStyle (WordStart))
    {
    case SCE_CMAKE_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_GAP:
    switch (GetStyle (WordStart))
    {
    case SCE_GAP_COMMENT:
    case SCE_GAP_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PLM:
    switch (GetStyle (WordStart))
    {
    case SCE_PLM_COMMENT:
    case SCE_PLM_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PROGRESS:
    switch (GetStyle (WordStart))
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
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ABAQUS:
    switch (GetStyle (WordStart))
    {
    case SCE_ABAQUS_COMMENT:
    case SCE_ABAQUS_COMMENTBLOCK:
    case SCE_ABAQUS_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ASYMPTOTE:
    switch (GetStyle (WordStart))
    {
    case SCE_ASY_COMMENT:
    case SCE_ASY_COMMENTLINE:
    case SCE_ASY_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_R:
    switch (GetStyle (WordStart))
    {
    case SCE_R_COMMENT:
    case SCE_R_STRING:
    case SCE_R_STRING2:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MAGIK:
    switch (GetStyle (WordStart))
    {
    case SCE_MAGIK_COMMENT:
    case SCE_MAGIK_HYPER_COMMENT:
    case SCE_MAGIK_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_POWERSHELL:
    switch (GetStyle (WordStart))
    {
    case SCE_POWERSHELL_COMMENT:
    case SCE_POWERSHELL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MYSQL:
    switch (GetStyle (WordStart))
    {
    case SCE_MYSQL_COMMENT:
    case SCE_MYSQL_COMMENTLINE:
    case SCE_MYSQL_STRING:
    case SCE_MYSQL_SQSTRING:
    case SCE_MYSQL_DQSTRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PO:
    switch (GetStyle (WordStart))
    {
    case SCE_PO_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_TAL:
  case SCLEX_COBOL:
  case SCLEX_TACL:
  case SCLEX_SORCUS:
    switch (GetStyle (WordStart))
    {
    case SCE_SORCUS_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_POWERPRO:
    switch (GetStyle (WordStart))
    {
    case SCE_POWERPRO_COMMENTBLOCK:
    case SCE_POWERPRO_COMMENTLINE:
    case SCE_POWERPRO_DOUBLEQUOTEDSTRING:
    case SCE_POWERPRO_SINGLEQUOTEDSTRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_NIMROD:
  case SCLEX_SML:
    switch (GetStyle (WordStart))
    {
    case SCE_SML_STRING:
    case SCE_SML_COMMENT:
    case SCE_SML_COMMENT1:
    case SCE_SML_COMMENT2:
    case SCE_SML_COMMENT3:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MARKDOWN:
    return FALSE;
  case SCLEX_TXT2TAGS:
    switch (GetStyle (WordStart))
    {
    case SCE_TXT2TAGS_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_A68K:
    switch (GetStyle (WordStart))
    {
    case SCE_A68K_COMMENT:
    case SCE_A68K_STRING1:
    case SCE_A68K_STRING2:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MODULA:
    switch (GetStyle (WordStart))
    {
    case SCE_MODULA_COMMENT:
    case SCE_MODULA_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_SEARCHRESULT:
    return FALSE;
  };
  return TRUE;
}

BOOL SpellChecker::CheckTextNeeded ()
{
  return CheckTextEnabled && AutoCheckText;
}

void SpellChecker::GetLimitsAndRecheckModified ()
{
  MSG Msg;
  GetMessage (&Msg, 0, 0, 0);
  std::pair<long, long> *Pair = reinterpret_cast <std::pair<long, long> *> (Msg.lParam);
  ModifiedStart = Pair->first;
  ModifiedEnd = Pair->second;
  CLEAN_AND_ZERO (Pair);
  RecheckModified ();
}

void SpellChecker::HideSuggestionBox ()
{
  SuggestionsInstance->display (false);
}
void SpellChecker::FindNextMistake ()
{
  // TODO: /n, /r should always be present in delimiters
  CurrentPosition = SendMsgToEditor (NppDataInstance, SCI_GETCURRENTPOS);
  int CurLine = SendMsgToEditor (NppDataInstance, SCI_LINEFROMPOSITION, CurrentPosition);
  int LineStartPos = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, CurLine);
  long DocLength = SendMsgToEditor (NppDataInstance, SCI_GETLENGTH);
  int IteratorPos = LineStartPos;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
  {
    Range.chrg.cpMin = IteratorPos;
    Range.chrg.cpMax = IteratorPos + BufferSize;
    int IgnoreOffsetting = 0;
    if (Range.chrg.cpMax > DocLength)
    {
      IgnoreOffsetting = 1;
      Range.chrg.cpMax = DocLength;
    }
    Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
    SendMsgToEditor (NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
    char *IteratingStart = Range.lpstrText + Range.chrg.cpMax - Range.chrg.cpMin - 1;
    char *IteratingChar = IteratingStart;
    if (!IgnoreOffsetting)
    {
      if (!ConvertingIsNeeded)
      {
        while ((!Utf8chr ( DelimUtf8Converted, IteratingChar)) && Range.lpstrText < IteratingChar)
        {
          IteratingChar = (char *) Utf8Dec (Range.lpstrText, IteratingChar);
        }
      }
      else
      {
        while (!strchr (DelimConverted, *IteratingChar) && Range.lpstrText < IteratingChar)
          IteratingChar--;
      }

      *IteratingChar = '\0';
    }
    Result = CheckText (Range.lpstrText, IteratorPos, FIND_FIRST);
    CLEAN_AND_ZERO_ARR (Range.lpstrText);
    if (Result)
      break;

    IteratorPos += (BufferSize + IteratingChar - IteratingStart);

    if (IteratorPos > DocLength)
    {
      if (!FullCheck)
      {
        CurrentPosition = 0;
        IteratorPos = 0;
        FullCheck = TRUE;
      }
      else
        break;

      if (FullCheck && IteratorPos > CurrentPosition)
        break; // So nothing was found TODO: Message probably
    }
  }
}

void SpellChecker::FindPrevMistake ()
{
  // TODO: /n, /r should always be present in delimiters
  CurrentPosition = SendMsgToEditor (NppDataInstance, SCI_GETCURRENTPOS);
  int CurLine = SendMsgToEditor (NppDataInstance, SCI_LINEFROMPOSITION, CurrentPosition);
  int LineCount = SendMsgToEditor (NppDataInstance, SCI_GETLINECOUNT);
  long LineEndPos = 0;
  long DocLength = SendMsgToEditor (NppDataInstance, SCI_GETLENGTH);
  LineEndPos = SendMsgToEditor (NppDataInstance, SCI_GETLINEENDPOSITION, CurLine);

  int IteratorPos = LineEndPos;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
  {
    Range.chrg.cpMin = IteratorPos - BufferSize;
    Range.chrg.cpMax = IteratorPos;
    int IgnoreOffsetting = 0;
    if (Range.chrg.cpMin < 0)
    {
      Range.chrg.cpMin = 0;
      IgnoreOffsetting = 1;
    }
    Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
    SendMsgToEditor (NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
    char *IteratingStart = Range.lpstrText;
    char *IteratingChar = IteratingStart;
    if (!IgnoreOffsetting)
    {
      if (!ConvertingIsNeeded)
      {
        while ((!Utf8chr ( DelimUtf8Converted, IteratingChar)) && IteratingChar)
        {
          IteratingChar = (char *) Utf8Inc (IteratingChar);
        }
      }
      else
      {
        while (!strchr (DelimConverted, *IteratingChar) && IteratingChar)
          IteratingChar++;
      }
    }
    int offset = IteratingChar - IteratingStart;
    Result = CheckText (Range.lpstrText + offset, Range.chrg.cpMin + offset, FIND_LAST);
    CLEAN_AND_ZERO_ARR (Range.lpstrText);
    if (Result)
      break;

    IteratorPos -= (BufferSize - offset);

    if (IteratorPos < 0)
    {
      if (!FullCheck)
      {
        CurrentPosition = DocLength + 1;
        IteratorPos = DocLength;
        FullCheck = TRUE;
      }
      else
        break;

      if (FullCheck && IteratorPos < CurrentPosition - 1)
        break; // So nothing was found TODO: Message probably
    }
  }
}

void SpellChecker::SetDefaultDelimiters ()
{
  SettingsDlgInstance->GetAdvancedDlg ()->SetDelimetersEdit (DEFAULT_DELIMITERS);
}

BOOL SpellChecker::GetWordUnderMouseIsRight (long &Pos, long &Length)
{
  BOOL Ret = TRUE;
  POINT p;
  if(GetCursorPos(&p) != 0){
    ScreenToClient(GetScintillaWindow (NppDataInstance), &p);

    int initCharPos = SendMsgToEditor (NppDataInstance, SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);

    if(initCharPos != -1){
      int Line = SendMsgToEditor (NppDataInstance, SCI_LINEFROMPOSITION, initCharPos);
      long LineLength = SendMsgToEditor (NppDataInstance, SCI_LINELENGTH, Line);
      char *Buf = new char[LineLength + 1];
      SendMsgToEditor (NppDataInstance, SCI_GETLINE, Line, (LPARAM) Buf);
      Buf [LineLength] = 0;
      long Offset = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, Line);
      char *Word = GetWordAt (initCharPos, Buf, Offset);
      if (!Word || !*Word)
      {
        Ret = TRUE;
      }
      else
      {
        Pos = Word - Buf + Offset;
        long WordLen = strlen (Word);
        if (CheckWord (Word, Pos, Pos + WordLen - 1))
        {
          Ret = TRUE;
        }
        else
        {
          Ret = FALSE;
          Length = WordLen;
        }
      }
      CLEAN_AND_ZERO_ARR (Buf);
    }
  }
  return Ret;
}

char *SpellChecker::GetWordAt (long CharPos, char *Text, long Offset)
{
  char *UsedText = 0;
  if (!DelimUtf8)
    return 0;

  char *Iterator = Text + CharPos - Offset;

  if (!ConvertingIsNeeded)
  {
    while ((!Utf8chr ( DelimUtf8Converted, Iterator)) && Text < Iterator)
      Iterator = (char *) Utf8Dec (Text, Iterator);
  }
  else
  {
    while (!strchr (DelimConverted, *Iterator) && Text < Iterator)
      Iterator--;
  }

  UsedText = Iterator;

  if (!ConvertingIsNeeded)
  {
    if (Utf8chr ( DelimUtf8Converted, UsedText))
      UsedText = Utf8Inc (UsedText); // Then find first token after this zero
  }
  else
  {
    if (strchr (DelimConverted, *UsedText))
      UsedText++;
  }

  char *Context = 0;
  // We're just taking the first token (basically repeating the same code as an in CheckVisible

  char *Res = 0;
  if (!ConvertingIsNeeded)
    Res = (char *) Utf8strtok (UsedText, DelimUtf8Converted, &Context);
  else
    Res = strtok_s (UsedText, DelimConverted, &Context);
  if (Res - Text + Offset > CharPos)
    return 0;
  else
    return Res;
}

void SpellChecker::SetSuggestionsBoxTransparency ()
{
  // Set WS_EX_LAYERED on this window
  SetWindowLong(SuggestionsInstance->getHSelf (), GWL_EXSTYLE,
    GetWindowLong(SuggestionsInstance->getHSelf (), GWL_EXSTYLE) | WS_EX_LAYERED);
  SetLayeredWindowAttributes(SuggestionsInstance->getHSelf (), 0, (255 * SBTrans) / 100, LWA_ALPHA);
  SuggestionsInstance->display (true);
  SuggestionsInstance->display (false);
}

void SpellChecker::InitSuggestionsBox ()
{
  if (!AspellLoaded)
    return;
  long Pos = 0;
  POINT p;
  if (!CheckTextNeeded ()) // If there's no red underline let's do nothing
  {
    SuggestionsInstance->display (false);
    return;
  }

  GetCursorPos (&p);
  if (WindowFromPoint (p) != GetScintillaWindow (NppDataInstance))
  {
    return;
  }

  if (GetWordUnderMouseIsRight (Pos, WUCLength))
  {
    return;
  }

  WUCPosition = Pos;
  int Line = SendMsgToEditor (NppDataInstance, SCI_LINEFROMPOSITION, Pos);
  int TextHeight = SendMsgToEditor (NppDataInstance, SCI_TEXTHEIGHT, Line);
  int TextWidth = SendMsgToEditor (NppDataInstance, SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM) "_");
  int XPos = SendMsgToEditor (NppDataInstance, SCI_POINTXFROMPOSITION, 0, Pos);
  int YPos = SendMsgToEditor (NppDataInstance, SCI_POINTYFROMPOSITION, 0, Pos);

  p.x = XPos; p.y = YPos;
  RECT R;
  GetWindowRect (SuggestionsInstance->getHSelf (), &R);
  ClientToScreen (GetScintillaWindow (NppDataInstance), &p);
  MoveWindow (SuggestionsInstance->getHSelf (), p.x, p.y + TextHeight - 3, SBSize, SBSize, TRUE);
  SuggestionsInstance->display ();
}

void SpellChecker::ShowSuggestionsMenu ()
{
  if (!AspellLoaded)
    return; // Word is already off-screen

  int Pos = WUCPosition;
  Sci_TextRange Range;
  Range.chrg.cpMin = WUCPosition;
  Range.chrg.cpMax = WUCPosition + WUCLength;
  Range.lpstrText = new char [WUCLength + 1];
  PostMsgToEditor (NppDataInstance, SCI_SETSEL, Pos, Pos + WUCLength);
  SendMsgToEditor (NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
  char *Utf8Buf = 0;
  char *AnsiBuf = 0;

  if (ConvertingIsNeeded)
    SetStringDUtf8 (Utf8Buf, Range.lpstrText);
  else
    SetString (Utf8Buf, Range.lpstrText);

  const AspellWordList *wl = 0;
  AspellSpeller *SelectedSpeller = 0;
  if (!MultiLangMode)
  {
    wl = aspell_speller_suggest (Speller, Utf8Buf, -1);
  }
  else
  {
    int MaxSize = -1;
    int Size;
    // In this mode we're finding maximum by length list from selected languages
    CurWordList = 0;
    for (int i = 0; i < (int) SpellerList.size (); i++)
    {
      CurWordList = aspell_speller_suggest (SpellerList[i], Utf8Buf, -1);

      AspellStringEnumeration * els = aspell_word_list_elements(CurWordList);
      Size = aspell_word_list_size (CurWordList);

      if (Size > 0)
      {
        const char *FirstSug = aspell_string_enumeration_next(els);
        if (Utf8GetCharSize (*Utf8Buf) != Utf8GetCharSize (*FirstSug))
          continue; // Special Hack to distinguish Cyrillic words from ones written Latin letters
      }

      if (Size > MaxSize)
      {
        MaxSize = Size;
        SelectedSpeller = SpellerList[i];
        wl = CurWordList;
      }
    }
  }
  if (!wl)
  {
    CLEAN_AND_ZERO_ARR (Utf8Buf);
    return;
  }

  AspellStringEnumeration * els = aspell_word_list_elements(wl);
  const char *Suggestion;
  TCHAR *Buf = 0;
  HMENU PopupMenu = SuggestionsInstance->GetPopupMenu ();
  PopupMenu = SuggestionsInstance->GetPopupMenu ();
  int Counter = 0;

  while ((Suggestion = aspell_string_enumeration_next(els)) != 0)
  {
    if (Counter >= SuggestionsNum)
      break;
    SetStringSUtf8 (Buf, Suggestion);
    AppendMenu (PopupMenu, MF_ENABLED | MF_STRING, Counter + 1, Buf);
    Counter++;
  }

  if (Counter > 0)
    AppendMenu (PopupMenu, MF_ENABLED | MF_SEPARATOR, 0, _T (""));

  TCHAR *MenuString = new TCHAR [WUCLength + 50 + 1]; // Add "" to dictionary
  if (!ConvertingIsNeeded)
    SetStringSUtf8 (Buf, Range.lpstrText);
  else
    SetString (Buf, Range.lpstrText);
  _stprintf (MenuString, _T ("Ignore \"%s\" for Current Session"), Buf);
  AppendMenu (PopupMenu, MF_ENABLED | MF_STRING, MID_IGNOREALL, MenuString);
  _stprintf (MenuString, _T ("Add \"%s\" to Dictionary"), Buf);
  AppendMenu (PopupMenu, MF_ENABLED | MF_STRING, MID_ADDTODICTIONARY, MenuString);

  // Here we go doing blocking call for menu, so shouldn't be any desync problems
  SendMessage (SuggestionsInstance->getHSelf (), WM_SHOWANDRECREATEMENU, 0, 0);
  WaitForEvent (EID_APPLYMENUACTION);
  int Result = SuggestionsInstance->GetResult ();
  if (Result != 0)
  {
    if (!MultiLangMode)
      SelectedSpeller = Speller;
    if (Result == MID_IGNOREALL)
    {
      aspell_speller_add_to_session (SelectedSpeller, Utf8Buf, strlen (Utf8Buf) + 1);
      aspell_speller_save_all_word_lists (SelectedSpeller);
      if (aspell_speller_error(SelectedSpeller) != 0)
      {
        AspellErrorMsgBox(GetScintillaWindow (NppDataInstance), aspell_speller_error_message(SelectedSpeller));
      }
      PostMsgToEditor (NppDataInstance, SCI_SETSEL, Pos + WUCLength, Pos + WUCLength);
      RecheckVisible ();
    }
    else if (Result == MID_ADDTODICTIONARY)
    {
      aspell_speller_add_to_personal(SelectedSpeller, Utf8Buf, strlen (Utf8Buf) + 1);
      aspell_speller_save_all_word_lists (SelectedSpeller);
      if (aspell_speller_error(SelectedSpeller) != 0)
      {
        AspellErrorMsgBox(GetScintillaWindow (NppDataInstance), aspell_speller_error_message(SelectedSpeller));
      }
      PostMsgToEditor (NppDataInstance, SCI_SETSEL, Pos + WUCLength, Pos + WUCLength);
      RecheckVisible ();
    }
    else if (Result <= Counter)
    {
      els = aspell_word_list_elements(wl);
      for (int i = 1; i <= Result; i++)
      {
        Suggestion = aspell_string_enumeration_next(els);
      }
      if (ConvertingIsNeeded)
        SetStringSUtf8 (AnsiBuf, Suggestion);
      else
        SetString (AnsiBuf, Suggestion);
      SendMsgToEditor (NppDataInstance, SCI_REPLACESEL, 0, (LPARAM) AnsiBuf);
    }
  }
  CLEAN_AND_ZERO_ARR (AnsiBuf);
  CLEAN_AND_ZERO_ARR (Range.lpstrText);
  CLEAN_AND_ZERO_ARR (Buf);
  CLEAN_AND_ZERO_ARR (Utf8Buf);
  CLEAN_AND_ZERO_ARR (MenuString);
}

void SpellChecker::UpdateAutocheckStatus (int SaveSetting)
{
  SaveToIni (_T ("Autocheck"), AutoCheckText, 0);
  CheckMenuItem(GetMenu(NppDataInstance->_nppHandle), get_funcItem ()[0]._cmdID, MF_BYCOMMAND | (AutoCheckText ? MF_CHECKED : MF_UNCHECKED));
}

void SpellChecker::SetCheckThose (int CheckThoseArg)
{
  CheckThose = CheckThoseArg;
}

void SpellChecker::SetFileTypes (TCHAR *FileTypesArg)
{
  SetString (FileTypes, FileTypesArg);
}

void SpellChecker::SetMultipleLanguages (const char *MultiLanguagesArg)
{
  SetString (MultiLanguages, MultiLanguagesArg);
}

void SpellChecker::RefreshUnderlineStyle ()
{
  SendMsgToEditor(NppDataInstance, SCI_INDICSETSTYLE, SCE_ERROR_UNDERLINE, UnderlineStyle);
  SendMsgToEditor(NppDataInstance, SCI_INDICSETFORE, SCE_ERROR_UNDERLINE, UnderlineColor);
}

void SpellChecker::SetUnderlineColor (int Value)
{
  UnderlineColor = Value;
}

void SpellChecker::SetUnderlineStyle (int Value)
{
  UnderlineStyle = Value;
}

void SpellChecker::SetIgnore (BOOL IgnoreNumbersArg, BOOL IgnoreCStartArg, BOOL IgnoreCHaveArg,
                              BOOL IgnoreCAllArg, BOOL Ignore_Arg, BOOL IgnoreSEApostropheArg)
{
  IgnoreNumbers = IgnoreNumbersArg;
  IgnoreCStart = IgnoreCStartArg;
  IgnoreCHave = IgnoreCHaveArg;
  IgnoreCAll = IgnoreCAllArg;
  Ignore_ = Ignore_Arg;
  IgnoreSEApostrophe = IgnoreSEApostropheArg;
}

void SpellChecker::SaveSettings ()
{
  FILE *Fp;
  _tfopen_s (&Fp, IniFilePath, _T ("w")); // Cleaning settings file (or creating it)
  fclose (Fp);
  TCHAR *TBuf = 0;
  SaveToIni (_T ("Ignore_Yo"), IgnoreYo, 1);
  SaveToIni (_T ("Convert_Single_Quotes_To_Apostrophe"), ConvertSingleQuotes, 1);
  SaveToIni (_T ("Check_Only_Comments_And_Strings"), CheckComments, 1);
  SaveToIni (_T ("Autocheck"), AutoCheckText, 0);
  SaveToIni (_T ("Check_Those_\\_Not_Those"), CheckThose, 1);
  SaveToIni (_T ("File_Types"), FileTypes, _T ("*.*"));
  SaveToIniUtf8 (_T ("Multiple_Languages"), MultiLanguages, "");
  SaveToIni (_T ("Ignore_Having_Number"), IgnoreNumbers, 1);
  SaveToIni (_T ("Ignore_Start_Capital"), IgnoreCStart, 0);
  SaveToIni (_T ("Ignore_Have_Capital"), IgnoreCHave, 1);
  SaveToIni (_T ("Ignore_All_Capital"), IgnoreCAll, 1);
  SaveToIni (_T ("Ignore_With_"), Ignore_, 1);
  SaveToIni (_T ("Ignore_That_Start_or_End_with_'"), IgnoreSEApostrophe, 1);
  SaveToIni (_T ("Underline_Color"), UnderlineColor, 0x0000ff);
  SaveToIni (_T ("Underline_Style"), UnderlineStyle, INDIC_SQUIGGLE);
  TCHAR *Path = 0;
  GetDefaultAspellPath (Path);
  SaveToIni (_T ("Aspell_Path"), AspellPath, Path);
  CLEAN_AND_ZERO_ARR (Path);
  SaveToIni (_T ("Suggestions_Number"), SuggestionsNum, 5);
  char *DefaultDelimUtf8 = 0;
  SetStringDUtf8 (DefaultDelimUtf8, DEFAULT_DELIMITERS);
  SaveToIniUtf8 (_T ("Delimiters"), DelimUtf8, DefaultDelimUtf8, TRUE);
  CLEAN_AND_ZERO_ARR (DefaultDelimUtf8);
  SaveToIni (_T ("Find_Next_Buffer_Size"), BufferSize / 1024, 16);
  SaveToIni (_T ("Suggestions_Button_Size"), SBSize, 15);
  SaveToIni (_T ("Suggestions_Button_Opacity"), SBTrans, 70);
  SaveToIniUtf8 (_T ("Language"), Language, "en");
}

void SpellChecker::LoadSettings ()
{
  char *BufUtf8 = 0;
  TCHAR *Path = 0;
  GetDefaultAspellPath (Path);
  LoadFromIni (AspellPath, _T ("Aspell_Path"), Path);
  CLEAN_AND_ZERO_ARR (Path);
  AspellLoaded = LoadAspell (AspellPath);
  if (AspellLoaded)
    AspellReinitSettings ();

  LoadFromIni (AutoCheckText, _T ("Autocheck"), 0);
  UpdateAutocheckStatus (0);
  LoadFromIniUtf8 (MultiLanguages, _T ("Multiple_Languages"), "");
  LoadFromIniUtf8 (BufUtf8, _T ("Language"), "en");
  SetLanguage (BufUtf8, 0);
  SetStringDUtf8 (BufUtf8, DEFAULT_DELIMITERS);
  LoadFromIniUtf8 (BufUtf8, _T ("Delimiters"), BufUtf8, TRUE);
  SetDelimiters (BufUtf8, 0);
  LoadFromIni (SuggestionsNum, _T ("Suggestions_Number"), 5);
  LoadFromIni (IgnoreYo, _T ("Ignore_Yo"), 1);
  LoadFromIni (ConvertSingleQuotes, _T ("Convert_Single_Quotes_To_Apostrophe"), 1);
  LoadFromIni (CheckThose , _T ("Check_Those_\\_Not_Those"), 1);
  LoadFromIni (FileTypes, _T ("File_Types"), _T ("*.*"));
  LoadFromIni (CheckComments, _T ("Check_Only_Comments_And_Strings"), 1);
  LoadFromIni (UnderlineColor, _T ("Underline_Color"), 0x0000ff);
  LoadFromIni (UnderlineStyle, _T ("Underline_Style"), INDIC_SQUIGGLE);
  LoadFromIni (IgnoreNumbers, _T ("Ignore_Having_Number"), 1);
  LoadFromIni (IgnoreCStart, _T ("Ignore_Start_Capital"), 0);
  LoadFromIni (IgnoreCHave, _T ("Ignore_Have_Capital"), 1);
  LoadFromIni (IgnoreCAll, _T ("Ignore_All_Capital"), 1);
  LoadFromIni (Ignore_, _T ("Ignore_With_"), 1);
  LoadFromIni (IgnoreSEApostrophe, _T ("Ignore_That_Start_or_End_with_'"), 1);
  int Size, Trans;
  LoadFromIni (Size, _T ("Suggestions_Button_Size"), 15);
  LoadFromIni (Trans, _T ("Suggestions_Button_Opacity"), 70);
  SetSuggBoxSettings (Size, Trans, 0);
  LoadFromIni (Size, _T ("Find_Next_Buffer_Size"), 16);
  SetBufferSize (Size, 0);
  RefreshUnderlineStyle ();
  CLEAN_AND_ZERO_ARR (BufUtf8);
}

void SpellChecker::CreateWordUnderline (HWND ScintillaWindow, int start, int end)
{
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_INDICATORFILLRANGE, start, (end - start + 1));
}

void SpellChecker::RemoveUnderline (HWND ScintillaWindow, int start, int end)
{
  if (end < start)
    return;
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_INDICATORCLEARRANGE, start, (end - start + 1));
}

// Warning - temporary buffer will be created
char *SpellChecker::GetDocumentText ()
{
  int lengthDoc = (SendMsgToEditor (NppDataInstance, SCI_GETLENGTH) + 1);
  char * buf = new char [lengthDoc];
  SendMsgToEditor (NppDataInstance, SCI_GETTEXT, lengthDoc, (LPARAM)buf);
  return buf;
}

void SpellChecker::GetVisibleLimits(long &Start, long &Finish)
{
  long top = SendMsgToEditor (NppDataInstance, SCI_GETFIRSTVISIBLELINE);
  long bottom = top + SendMsgToEditor (NppDataInstance, SCI_LINESONSCREEN);
  top = SendMsgToEditor (NppDataInstance, SCI_DOCLINEFROMVISIBLE, top);
  bottom = SendMsgToEditor (NppDataInstance, SCI_DOCLINEFROMVISIBLE, bottom);
  long LineCount = SendMsgToEditor (NppDataInstance, SCI_GETLINECOUNT);
  Start = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, top);
  // Not using end of line position cause utf-8 symbols could be more than one char
  // So we use next line start as the end of our visible text
  if (bottom + 1 < LineCount)
    Finish = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, bottom + 1);
  else
    Finish = SendMsgToEditor (NppDataInstance, SCI_GETTEXTLENGTH);
}

char *SpellChecker::GetVisibleText(long *offset)
{
  Sci_TextRange range;
  GetVisibleLimits (range.chrg.cpMin, range.chrg.cpMax);

  if (range.chrg.cpMax < 0)
    return 0;
  char *Buf = new char [range.chrg.cpMax - range.chrg.cpMin + 1]; // + one byte for terminating zero
  range.lpstrText = Buf;
  SendMsgToEditor (NppDataInstance, SCI_GETTEXTRANGE, NULL ,(LPARAM)&range);
  *offset = range.chrg.cpMin;
  Buf[range.chrg.cpMax - range.chrg.cpMin] = 0;
  return Buf;
}

void SpellChecker::ClearAllUnderlines ()
{
  int length = SendMsgToEditor(NppDataInstance, SCI_GETLENGTH);
  if(length > 0)
  {
    PostMsgToEditor (NppDataInstance, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
    PostMsgToEditor (NppDataInstance, SCI_INDICATORCLEARRANGE, 0, length);
  }
}

void SpellChecker::Cleanup()
{
  CLEAN_AND_ZERO_ARR (Language);
  CLEAN_AND_ZERO_ARR (DelimUtf8);
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DelimConverted);
  CLEAN_AND_ZERO_ARR (AspellPath);
}

void SpellChecker::SetAspellPath (const TCHAR *Path)
{
  SetString (AspellPath, Path);
  AspellLoaded = LoadAspell (AspellPath);
  if (AspellLoaded)
    AspellReinitSettings ();
}

void SpellChecker::SaveToIni (const TCHAR *Name, const TCHAR *Value, const TCHAR *DefaultValue, BOOL InQuotes)
{
  if (DefaultValue && _tcscmp (Value, DefaultValue) == 0)
    return;

  if (InQuotes)
  {
    int Len = 1 + _tcslen (Value) + 1 + 1;
    TCHAR *Buf = new TCHAR[Len];
    _stprintf (Buf, _T ("\"%s\""), Value, Len - 3);
    WritePrivateProfileString (_T ("SpellCheck"), Name, Buf, IniFilePath);
    CLEAN_AND_ZERO_ARR (Buf);
  }
  else
  {
    WritePrivateProfileString (_T ("SpellCheck"), Name, Value, IniFilePath);
  }
}

void SpellChecker::SaveToIni (const TCHAR *Name, int Value, int DefaultValue)
{
  if (Value == DefaultValue)
    return;

  TCHAR Buf[DEFAULT_BUF_SIZE];
  _itot_s (Value, Buf, 10);
  SaveToIni (Name, Buf, 0);
}

void SpellChecker::SaveToIniUtf8 (const TCHAR *Name, const char *Value,  const char *DefaultValue, BOOL InQuotes)
{
  if (DefaultValue && strcmp (Value, DefaultValue) == 0)
    return;

  TCHAR *Buf = 0;
  SetStringSUtf8 (Buf, Value);
  SaveToIni (Name, Buf, 0, InQuotes);
  CLEAN_AND_ZERO_ARR (Buf);
}

void SpellChecker::LoadFromIni (TCHAR *&Value, const TCHAR *Name, const TCHAR *DefaultValue, BOOL InQuotes)
{
  CLEAN_AND_ZERO_ARR (Value);
  Value = new TCHAR [DEFAULT_BUF_SIZE];

  GetPrivateProfileString (_T ("SpellCheck"), Name, DefaultValue, Value, DEFAULT_BUF_SIZE, IniFilePath);

  if (InQuotes)
  {
    int Len = _tcslen (Value);
    // Proof check for quotes
    if (Value[0] != '\"' || Value[Len] != '\"')
    {
      _tcscpy_s (Value, DEFAULT_BUF_SIZE, DefaultValue);
      return;
    }

    for (int i = 0; i < Len; i++)
      Value[i] = Value[i + 1];

    Value[Len - 1] = 0;
  }
}

void SpellChecker::LoadFromIni (int &Value, const TCHAR *Name, int DefaultValue)
{
  TCHAR BufDefault[DEFAULT_BUF_SIZE];
  TCHAR *Buf = 0;
  _itot_s (DefaultValue, BufDefault, 10);
  LoadFromIni (Buf, Name, BufDefault);
  Value = _ttoi (Buf);
  CLEAN_AND_ZERO_ARR (Buf);
}

void SpellChecker::LoadFromIniUtf8 (char *&Value, const TCHAR *Name, const char *DefaultValue, BOOL InQuotes)
{
  TCHAR *BufDefault = 0;
  TCHAR *Buf = 0;
  SetStringSUtf8 (BufDefault, DefaultValue);
  LoadFromIni (Buf, Name, BufDefault);
  SetStringDUtf8 (Value, Buf);
  CLEAN_AND_ZERO_ARR (Buf);
  CLEAN_AND_ZERO_ARR (BufDefault);
}

// Here parameter is in ANSI (may as well be utf-8 cause only english I guess)
void SpellChecker::SetLanguage (const char *Str, int SaveToIni)
{
  if (strcmp (Str, "<MULTIPLE>") == 0)
    MultiLangMode = 1;
  else
  {
    MultiLangMode = 0;
  }

  SetString (Language, Str);

  AspellReinitSettings ();
}

const char *SpellChecker::GetDelimiters ()
{
  return DelimUtf8;
}

void SpellChecker::SetSuggestionsNum (int Num)
{
  SuggestionsNum = Num;
}

// Here parameter is in utf-8
void SpellChecker::SetDelimiters (const char *Str, int SaveToIni)
{
  TCHAR *DestBuf = 0;
  TCHAR *SrcBuf = 0;
  SetString (DelimUtf8, Str);
  SetStringSUtf8 (SrcBuf, DelimUtf8);
  SetParsedString (DestBuf, SrcBuf);
  int TargetBufLength = _tcslen (DestBuf) + 5 + 1;
  TCHAR *TargetBuf = new TCHAR [_tcslen (DestBuf) + 5 + 1];
  _tcscpy (TargetBuf, DestBuf);
  _tcscat_s (TargetBuf, TargetBufLength, _T (" \n\r\t\v"));
  SetStringDUtf8 (DelimUtf8Converted, TargetBuf);
  SetStringSUtf8 (DelimConverted, DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DestBuf);
  CLEAN_AND_ZERO_ARR (SrcBuf);
  CLEAN_AND_ZERO_ARR (TargetBuf);
}

const char *SpellChecker::GetLanguage ()
{
  return Language;
}

BOOL SpellChecker::AspellReinitSettings ()
{
  if (!AspellLoaded)
    return FALSE;

  if (MultiLangMode)
  {
    if (!MultiLanguages)
    {
      AspellLoaded = FALSE;
      return FALSE;
    }

    char *Token = 0;
    char *MultiLanguagesCopy = 0;
    char *Context = 0;
    SetString (MultiLanguagesCopy, MultiLanguages);
    Token = strtok_s (MultiLanguagesCopy, "|", &Context);
    for (int i = 0; i < (int) SpellerList.size (); i++)
      delete_aspell_speller (SpellerList[i]);

    SpellerList.clear ();
    while (Token)
    {
      AspellConfig *spell_config = new_aspell_config();
      aspell_config_replace (spell_config, "encoding", "utf-8");
      aspell_config_replace(spell_config, "lang", Token);
      AspellCanHaveError * possible_err = new_aspell_speller(spell_config);
      if (aspell_error_number(possible_err) == 0)
      {
        SpellerList.push_back (to_aspell_speller(possible_err));
      }
      Token = strtok_s (NULL, "|", &Context);
      delete_aspell_config (spell_config);
    }
    CLEAN_AND_ZERO_ARR (MultiLanguagesCopy);
    return TRUE;
  }

  AspellConfig *spell_config = new_aspell_config();
  aspell_config_replace (spell_config, "encoding", "utf-8");
  if (!MultiLangMode && Language && *Language)
    aspell_config_replace(spell_config, "lang", Language);
  if (Speller)
  {
    delete_aspell_speller (Speller);
    Speller = 0;
  }

  AspellCanHaveError * possible_err = new_aspell_speller(spell_config);

  if (aspell_error_number(possible_err) != 0)
  {
    delete_aspell_config (spell_config);
    if (!Language || strcmp (Language, "en") != 0)
    {
      SetLanguage ("en", 1);
      return AspellReinitSettings ();
    }
    AspellLoaded = FALSE;
    return FALSE;
  }
  else
    Speller = to_aspell_speller(possible_err);
  delete_aspell_config (spell_config);
  return TRUE;
}

BOOL SpellChecker::AspellClear ()
{
  if (!AspellLoaded)
    return FALSE;

  for (unsigned int i = 0; i < SpellerList.size (); i++)
    delete_aspell_speller (SpellerList[i]);
  delete_aspell_speller (Speller);
  return TRUE;
}

void SpellChecker::SetBufferSize (int Size, BOOL SaveIni)
{
  if (Size < 1)
    Size = 1;
  if (Size > 10 * 1024)
    Size = 10 * 1024;
  BufferSize = Size * 1024;
}

void SpellChecker::SetSuggBoxSettings (int Size, int Transparency, int SaveIni)
{
  if (SBSize != Size)
  {
    SBSize = Size;
    if (SaveIni)
      HideSuggestionBox ();
  }

  if (Transparency != SBTrans)
  {
    SBTrans = Transparency;
    // Don't sure why but this helps to fix a bug with notepad++ window resizing
    // TODO: Fix it normal way
    SetLayeredWindowAttributes(SuggestionsInstance->getHSelf (), 0, (255 * SBTrans) / 100, LWA_ALPHA);
  }
}

static const char Yo[] = "\xd0\x81";
static const char Ye[] = "\xd0\x95";
static const char yo[] = "\xd1\x91";
static const char ye[] = "\xd0\xb5";
static const char PunctuationApostrophe[] = "\xe2\x80\x99";

BOOL SpellChecker::CheckWord (char *Word, long Start, long End)
{
  BOOL res = FALSE;
  if (!AspellLoaded)
    return TRUE;
  // Well Numbers have same codes for ansi and unicode I guess, so
  // If word contains number then it's probably just a number or some crazy name
  if (CheckComments && !CheckWordInCommentOrString (Start, End))
    return TRUE;

  char *Utf8Buf = 0;

  // Since this moment we need to exit this function by settings res and going to cleanup mark
  if (ConvertingIsNeeded) // That's really part that sucks
  {
    // Well TODO: maybe convert only to unicode and tell aspell that we're using unicode
    // Cause double conversion kinda sucks
    SetStringDUtf8 (Utf8Buf, Word);
  }
  else
    Utf8Buf = Word;

  TCHAR *Ts = 0;
  long Len = strlen (Utf8Buf);

  if (IgnoreNumbers && Utf8pbrk (Utf8Buf, "0123456789") != 0)
  {
    res = TRUE;
    goto CleanUp;
  }

  if (IgnoreCStart || IgnoreCHave || IgnoreCAll)
  {
    SetStringSUtf8 (Ts, Utf8Buf);
    if (IgnoreCStart && IsCharUpper (Ts[0]))
    {
      res = TRUE;
      goto CleanUp;
    }
    if (IgnoreCHave || IgnoreCAll)
    {
      BOOL AllUpper = IsCharUpper (Ts[0]);
      for (unsigned int i = 1; i < _tcslen (Ts); i++)
      {
        if (IsCharUpper (Ts[i]))
        {
          if (IgnoreCHave)
          {
            res = TRUE;
            goto CleanUp;
          }
        }
        else
          AllUpper = FALSE;
      }

      if (AllUpper && IgnoreCAll)
      {
        res = TRUE;
        goto CleanUp;
      }
    }
  }

  if (Ignore_ && strchr (Utf8Buf, '_') != 0)
  {
    res = TRUE;
    goto CleanUp;
  }

  if (IgnoreYo)
  {
    char *Iter = Utf8Buf;
    while (Iter = strstr (Iter, Yo))
    {
      Iter[0] = Ye[0];
      Iter[1] = Ye[1];
      Iter += 2;
    }
    Iter = Utf8Buf;
    while (Iter = strstr (Iter, yo))
    {
      Iter[0] = ye[0];
      Iter[1] = ye[1];
      Iter += 2;
    }
  }

  if (ConvertSingleQuotes)
  {
    char *Iter = Utf8Buf;
    char *NestedIter = 0;
    while (Iter = strstr (Iter, PunctuationApostrophe))
    {
      *Iter = '\'';
      Iter++;
      NestedIter = Iter;
      while (*(NestedIter + 2))
      {
        *NestedIter =  *(NestedIter + 2);
        NestedIter++;
      }
      *NestedIter = 0;
      *(NestedIter + 1) = 0;
    }
    Len = strlen (Utf8Buf);
  }

  if (IgnoreSEApostrophe)
  {
    if (Utf8Buf[0] == '\'' || Utf8Buf[Len - 1] == '\'')
    {
      res = TRUE;
      goto CleanUp;
    }
  }

  if (!MultiLangMode)
  {
    res = aspell_speller_check(Speller, Utf8Buf, Len);
  }
  else
  {
    for (int i = 0; i < (int )SpellerList.size () && !res; i++)
    {
      res = res || aspell_speller_check(SpellerList[i], Utf8Buf, Len);
    }
  }

CleanUp:
  if (ConvertingIsNeeded)
    CLEAN_AND_ZERO_ARR (Utf8Buf);
  CLEAN_AND_ZERO_ARR (Ts);

  return res;
}

BOOL SpellChecker::CheckText (char *TextToCheck, long Offset, CheckTextMode Mode)
{
  if (!TextToCheck || !*TextToCheck)
    return FALSE;

  HWND ScintillaWindow = GetScintillaWindow (NppDataInstance);
  int oldid = SendMsgToEditor (ScintillaWindow, NppDataInstance, SCI_GETINDICATORCURRENT);
  char *Context = 0; // Temporary variable for strtok_s usage
  char *token;
  BOOL stop = FALSE;
  long ResultingWordEnd = -1, ResultingWordStart = -1;
  long TextLen = strlen (TextToCheck);
  std::vector<long> UnderlineBuffer;
  long WordStart = 0;
  long WordEnd = 0;

  if (!DelimUtf8)
    return FALSE;

  if (!ConvertingIsNeeded)
    token = Utf8strtok (TextToCheck, DelimUtf8Converted, &Context);
  else
    token = strtok_s (TextToCheck, DelimConverted, &Context);

  while (token)
  {
    if (token)
    {
      WordStart = Offset + token - TextToCheck;
      WordEnd = Offset + token - TextToCheck + strlen (token);

      if (!CheckWord (token, WordStart, WordEnd))
      {
        switch (Mode)
        {
        case UNDERLINE_ERRORS:
          UnderlineBuffer.push_back (WordStart);
          UnderlineBuffer.push_back (WordEnd);
          break;
        case FIND_FIRST:
          if (WordEnd > CurrentPosition)
          {
            SendMsgToEditor (NppDataInstance, SCI_SETSEL, WordStart, WordEnd);
            stop = TRUE;
          }
          break;
        case FIND_LAST:
          {
            if (WordEnd >= CurrentPosition)
            {
              stop = TRUE;
              break;
            }
            ResultingWordStart = WordStart;
            ResultingWordEnd = WordEnd;
          }
          break;
        }
        if (stop)
          break;
      }
      else
      {
      }
    }

    if (!ConvertingIsNeeded)
      token =  Utf8strtok (NULL, DelimUtf8Converted, &Context);
    else
      token = strtok_s (NULL, DelimConverted, &Context);
  }

  if (Mode == UNDERLINE_ERRORS)
  {
    long PrevPos = Offset;
    for (long i = 0; i < (long) UnderlineBuffer.size () - 1; i += 2)
    {
      RemoveUnderline (ScintillaWindow, PrevPos, UnderlineBuffer[i] - 1);
      CreateWordUnderline (ScintillaWindow, UnderlineBuffer[i], UnderlineBuffer[i + 1] - 1);
      PrevPos = UnderlineBuffer[i + 1];
    }
    RemoveUnderline (ScintillaWindow, PrevPos, Offset + TextLen);
  }

  // PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, oldid);

  switch (Mode)
  {
  case UNDERLINE_ERRORS:
    return TRUE;
  case FIND_FIRST:
    return stop;
  case FIND_LAST:
    if (ResultingWordStart == -1)
      return FALSE;
    else
    {
      SendMsgToEditor (NppDataInstance, SCI_SETSEL, ResultingWordStart, ResultingWordEnd);
      return TRUE;
    }
  };
  return FALSE;
}

void SpellChecker::ClearVisibleUnderlines ()
{
  int length = SendMsgToEditor(NppDataInstance, SCI_GETLENGTH);
  if(length > 0)
  {
    PostMsgToEditor (NppDataInstance, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
    PostMsgToEditor (NppDataInstance, SCI_INDICATORCLEARRANGE, 0, length);
  }
}

void SpellChecker::CheckVisible ()
{
  CLEAN_AND_ZERO_ARR (VisibleText);
  VisibleText = GetVisibleText (&VisibleTextOffset);
  VisibleTextLength = strlen (VisibleText);
  CheckText (VisibleText, VisibleTextOffset, UNDERLINE_ERRORS);
}

void SpellChecker::setEncodingById (int EncId)
{
  /*
  int CCH;
  char szCodePage[10];
  char *FinalString;
  */
  switch (EncId)
  {
  case SC_CP_UTF8:
    ConvertingIsNeeded = FALSE;
    // SetEncoding ("utf-8");
    break;
  default:
    {
      ConvertingIsNeeded = TRUE;
      /*
      CCH = GetLocaleInfoA (GetSystemDefaultLCID(),
      LOCALE_IDEFAULTANSICODEPAGE,
      szCodePage,
      countof (szCodePage));
      FinalString = new char [2 + strlen (szCodePage) + 1];
      strcpy (FinalString, "cp"); // Actually this encoding may as well be ISO-XXXX, that's why this code sucks
      strcat (FinalString, szCodePage);
      SetEncoding (FinalString);
      break;
      */
    }
  }
}

void SpellChecker::SwitchAutoCheck ()
{
  AutoCheckText = !AutoCheckText;
  UpdateAutocheckStatus ();
  RecheckVisible ();
}

void SpellChecker::RecheckModified ()
{
  if (!AspellLoaded)
  {
    ClearAllUnderlines ();
    return;
  }

  long FirstModifiedLine = SendMsgToEditor (NppDataInstance, SCI_LINEFROMPOSITION, ModifiedStart);
  long LastModifiedLine = SendMsgToEditor (NppDataInstance, SCI_LINEFROMPOSITION, ModifiedEnd);
  long LineCount = SendMsgToEditor (NppDataInstance, SCI_GETLINECOUNT);
  long FirstPossiblyModifiedPos = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, FirstModifiedLine);
  long LastPossiblyModifiedPos = 0;
  if (LastModifiedLine + 1 < LineCount)
    LastPossiblyModifiedPos = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, LastModifiedLine + 1);
  else
    LastPossiblyModifiedPos = SendMsgToEditor (NppDataInstance, SCI_GETLENGTH);

  Sci_TextRange Range;
  Range.chrg.cpMin = FirstPossiblyModifiedPos;
  Range.chrg.cpMax = LastPossiblyModifiedPos;
  Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
  SendMsgToEditor (NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);

  CheckText (Range.lpstrText, FirstPossiblyModifiedPos, UNDERLINE_ERRORS);
  CLEAN_AND_ZERO_ARR (Range.lpstrText);
}

void SpellChecker::SetConversionOptions (BOOL ConvertYo, BOOL ConvertSingleQuotesArg)
{
  IgnoreYo = ConvertYo;
  ConvertSingleQuotes = ConvertSingleQuotesArg;
}

void SpellChecker::RecheckVisible ()
{
  int CodepageId = 0;
  if (!AspellLoaded)
  {
    ClearAllUnderlines ();
    return;
  }
  CodepageId = (int) SendMsgToEditor (NppDataInstance, SCI_GETCODEPAGE, 0, 0);
  setEncodingById (CodepageId); // For now it just changes should we convert it to utf-8 or no
  if (CheckTextNeeded ())
    CheckVisible ();
  else
    ClearAllUnderlines ();
}

void SpellChecker::ErrorMsgBox (const TCHAR * message)
{
  TCHAR buf [DEFAULT_BUF_SIZE];
  _stprintf_s (buf, _T ("%s%s"), "DSpellCheck Error:", message, _tcslen (message));
  MessageBox (NppDataInstance->_nppHandle, message, _T("Error Happened!"), MB_OK | MB_ICONSTOP);
}
