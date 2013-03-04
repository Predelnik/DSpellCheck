#include "Aspell.h"
#include "CommonFunctions.h"
#include "MainDef.h"
#include "PluginInterface.h"
#include "Plugin.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "Scintilla.h"
#include "Suggestions.h"

SpellChecker::SpellChecker (const TCHAR *IniFilePathArg, SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
                            Suggestions *SuggestionsInstanceArg)
{
  DelimUtf8 = 0;
  DelimUtf8Converted = 0;
  Speller = 0;
  IniFilePath = 0;
  Language = 0;
  VisibleText = 0;
  VisibleTextLength = -1;
  SetString (IniFilePath, IniFilePathArg);
  SettingsDlgInstance = SettingsDlgInstanceArg;
  SuggestionsInstance = SuggestionsInstanceArg;
  NppDataInstance = NppDataInstanceArg;
  LoadAspell (0);
  LoadSettings ();
  AspellReinitSettings ();
}

SpellChecker::~SpellChecker ()
{
  AspellClear ();
}

BOOL WINAPI SpellChecker::NotifyEvent (DWORD Event)
{
  switch (Event)
  {
  case  EID_FILL_DIALOGS:
    SettingsDlgInstance->GetSimpleDlg ()->AddAvalaibleLanguages (Language);
    SettingsDlgInstance->GetAdvancedDlg ()->FillDelimeters (DelimUtf8);
    return TRUE;
  case EID_APPLY_SETTINGS:
    SettingsDlgInstance->GetSimpleDlg ()->ApplySettings (this);
    SettingsDlgInstance->GetAdvancedDlg ()->ApplySettings (this);
    return TRUE;
  case EID_HIDE_DIALOG:
    SettingsDlgInstance->display (false);
    return TRUE;
  case EID_LOAD_SETTINGS:
    LoadSettings ();
    return TRUE;
  case EID_RECHECK_VISIBLE:
    RecheckVisible ();
    return TRUE;
  case EID_SWITCH_AUTOCHECK:
    SwitchAutoCheck ();
    return TRUE;
  case EID_KILLTHREAD:
    return FALSE;
  case EID_INITSUGGESTIONS:
    InitSuggestions ();
    return TRUE;
  }
  return TRUE;
}

char* SpellChecker::GetWordUnderMouse(){
  char *ret = NULL;
  POINT p;
  if(GetCursorPos(&p) != 0){
    ScreenToClient(WindowFromPoint(p), &p);

    int initCharPos = SendMsgToEditor (NppDataInstance, SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);
    if(initCharPos != -1){
      ret = GetWordAt (initCharPos, VisibleText);
    }
  }
  return ret;
}

char *SpellChecker::GetWordAt (int CharPos, char *Text)
{
  // Text here is already tokenized
  if (!Text)
    CheckVisible ();

  int PrevZero = -1;

  if (!Text || !*Text)
    return 0;

  for (int i = 0; i <= VisibleTextLength; i++)
  {
    if (VisibleText[i] == 0)
    {
      if (i > (CharPos - VisibleTextOffset))
        return VisibleText + PrevZero + 1;
      else
        PrevZero = i;
    }
  }
  return 0;
}

void SpellChecker::InitSuggestions ()
{
  char *Word = GetWordUnderMouse ();
  if (!Word || !*Word || CheckWord (Word))
    return;
  int Pos = Word - VisibleText + VisibleTextOffset;
  int XPos = SendMsgToEditor (NppDataInstance, SCI_POINTXFROMPOSITION, 0, Pos);
  int YPos = SendMsgToEditor (NppDataInstance, SCI_POINTYFROMPOSITION, 0, Pos);
  int Line = SendMsgToEditor (NppDataInstance, SCI_LINEFROMPOSITION, Pos);
  int TextHeight = SendMsgToEditor (NppDataInstance, SCI_TEXTHEIGHT, Line);
  POINT p;
  p.x = XPos; p.y = YPos;
  ClientToScreen (GetScintillaWindow (NppDataInstance), &p);

  const AspellWordList *wl = aspell_speller_suggest (Speller, Word, -1);
  if (!wl)
    return;

  AspellStringEnumeration * els = aspell_word_list_elements(wl);
  const char *Suggestion;
  TCHAR *Buf = 0;
  HMENU PopupMenu = SuggestionsInstance->GetPopupMenu ();
  int Counter = 0;

  while ((Suggestion = aspell_string_enumeration_next(els)) != 0)
  {
    if (Counter >= 5)
      break;
    SetStringSUtf8 (Buf, Suggestion);
    InsertMenu (PopupMenu, 0, MF_BYPOSITION, 0, Buf);
    Counter++;
  }

  MoveWindow (SuggestionsInstance->getHSelf (), p.x, p.y + TextHeight, 10, 10, TRUE);
}

void SpellChecker::UpdateAutocheckStatus ()
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  _itot (AutoCheckText, Buf, DEFAULT_BUF_SIZE);
  WritePrivateProfileString (_T ("SpellCheck"), _T ("Autocheck"), Buf, IniFilePath);
  CheckMenuItem(GetMenu(NppDataInstance->_nppHandle), get_funcItem ()[0]._cmdID, MF_BYCOMMAND | (AutoCheckText ? MF_CHECKED : MF_UNCHECKED));
  RecheckVisible ();
}

void SpellChecker::LoadSettings ()
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  char *BufA = 0;
  char *BufUtf8 = 0;
  GetPrivateProfileString (_T ("SpellCheck"), _T ("Autocheck"), _T ("0"), Buf, DEFAULT_BUF_SIZE, IniFilePath);
  AutoCheckText = _ttoi (Buf);
  CheckMenuItem(GetMenu(NppDataInstance->_nppHandle), get_funcItem ()[0]._cmdID, MF_BYCOMMAND | (AutoCheckText ? MF_CHECKED : MF_UNCHECKED));
  UpdateAutocheckStatus ();
  GetPrivateProfileString (_T ("SpellCheck"), _T ("Language"), _T ("En_Us"), Buf, DEFAULT_BUF_SIZE, IniFilePath);
  SetString (BufA, Buf);
  SetLanguage (BufA);
  GetPrivateProfileString (_T ("SpellCheck"), _T ("Delim"), _T (" \\n\\r\\t,.!?-\\\":;{}()[]\\\\/'"), Buf, DEFAULT_BUF_SIZE, IniFilePath);
  SetStringDUtf8 (BufUtf8, Buf);
  SetDelimeters (BufUtf8);
}

void SpellChecker::CreateWordUnderline (HWND ScintillaWindow, int start, int end)
{
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_INDICATORFILLRANGE, start, (end - start + 1));
}

void SpellChecker::RemoveWordUnderline (int start, int end)
{
  SendMsgToEditor (NppDataInstance, SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
  SendMsgToEditor (NppDataInstance, SCI_INDICATORCLEARRANGE, start, (end - start + 1));
}

// Warning - temporary buffer will be created
char *SpellChecker::GetDocumentText ()
{
  int lengthDoc = (SendMsgToEditor (NppDataInstance, SCI_GETLENGTH) + 1);
  char * buf = new char [lengthDoc];
  SendMsgToEditor (NppDataInstance, SCI_GETTEXT, lengthDoc, (LPARAM)buf);
  return buf;
}

char *SpellChecker::GetVisibleText(long *offset)
{
  long top = SendMsgToEditor (NppDataInstance, SCI_GETFIRSTVISIBLELINE);
  long bottom = top + SendMsgToEditor (NppDataInstance, SCI_LINESONSCREEN);
  long line_count = SendMsgToEditor (NppDataInstance, SCI_GETLINECOUNT);
  Sci_TextRange range;
  range.chrg.cpMin = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, top);
  // Not using end of line position cause utf-8 symbols could be more than one char
  // So we use next line start as the end of our visible text
  if (bottom + 1 < line_count)
    range.chrg.cpMax = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, bottom + 1);
  else
    range.chrg.cpMax = SendMsgToEditor (NppDataInstance, SCI_GETTEXTLENGTH);

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
    PostMsgToEditor (NppDataInstance, SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
    PostMsgToEditor (NppDataInstance, SCI_INDICATORCLEARRANGE, 0, length - 1);
  }
}

//
// Place for the clean up (especially for the shortcut)
//
void SpellChecker::Cleanup()
{
  CLEAN_AND_ZERO_ARR (Language);
  CLEAN_AND_ZERO_ARR (DelimUtf8);
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);
  // We should deallocate shortcuts here
}

// Here parameter is in ANSI (may as well be utf-8 cause only english I guess)
void SpellChecker::SetLanguage (const char *Str)
{
  SetString (Language, Str);
  TCHAR *OutputString = 0;
  SetString (OutputString, Str);
  WritePrivateProfileString (_T ("SpellCheck"), _T ("Language"), OutputString, IniFilePath);
  CLEAN_AND_ZERO_ARR (OutputString);
  AspellReinitSettings ();
}

const char *SpellChecker::GetDelimeters ()
{
  return DelimUtf8;
}

// Here parameter is in utf-8
void SpellChecker::SetDelimeters (const char *Str)
{
  TCHAR *DestBuf = 0;
  TCHAR *SrcBuf = 0;
  SetString (DelimUtf8, Str);
  SetStringSUtf8 (SrcBuf, DelimUtf8);
  SetParsedString (DestBuf, SrcBuf);
  SetStringDUtf8 (DelimUtf8Converted, DestBuf);
  CLEAN_AND_ZERO_ARR (DestBuf);
  CLEAN_AND_ZERO_ARR (SrcBuf);

  TCHAR *OutputString = 0;
  SetStringSUtf8 (OutputString, Str);
  WritePrivateProfileString (_T ("SpellCheck"), _T ("Delimeters"), OutputString, IniFilePath);
  CLEAN_AND_ZERO_ARR (OutputString);
  // RecheckDocument ();
}

const char *SpellChecker::GetLanguage ()
{
  return Language;
}

BOOL SpellChecker::AspellReinitSettings ()
{
  AspellConfig *spell_config = new_aspell_config();
  aspell_config_replace (spell_config, "encoding", "utf-8");
  if (Language)
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
    return FALSE;
  }
  else
    Speller = to_aspell_speller(possible_err);
  delete_aspell_config (spell_config);
  return TRUE;
}

BOOL SpellChecker::AspellClear ()
{
  delete_aspell_speller (Speller);
  return TRUE;
}

BOOL SpellChecker::CheckWord (char *Word)
{
  if (ConvertingIsNeeded) // That's really part that sucks
  {
    // Well TODO: maybe convert only to unicode and tell aspell that we're using unicode
    // Cause double conversion kinda sucks
    char *Utf8Buf = 0;
    SetStringDUtf8 (Utf8Buf, Word);
    BOOL res = aspell_speller_check(Speller, Utf8Buf, strlen (Utf8Buf));
    CLEAN_AND_ZERO_ARR (Utf8Buf);
    return res;
  }
  return aspell_speller_check(Speller, Word, strlen (Word));
}

void SpellChecker::CheckText (char *TextToCheck, long offset)
{
  if (!TextToCheck || !*TextToCheck)
    return;

  HWND ScintillaWindow = GetScintillaWindow (NppDataInstance);
  int oldid = SendMsgToEditor (ScintillaWindow, NppDataInstance, SCI_GETINDICATORCURRENT);
  char *context = 0; // Temporary variable for strtok_s usage
  char *Delim = 0;
  char *token;

  if (!DelimUtf8)
    return;

  if (ConvertingIsNeeded)
    SetStringSUtf8 (Delim, DelimUtf8Converted);
  // A little bit ugly but since only function in mbstr.h require unsigned char then
  // we're gonna do explicitly all conversations before call to them.

  if (!ConvertingIsNeeded)
    token = (char *) _mbstok_s ((unsigned char *) TextToCheck, (unsigned char *) DelimUtf8Converted, (unsigned char **) &context);
  else
    token = strtok_s (TextToCheck, Delim, &context);

  while (token)
  {
    if (token && !CheckWord (token))
      CreateWordUnderline (ScintillaWindow, offset + token - TextToCheck, offset + token - TextToCheck + strlen (token) - 1);

    if (!ConvertingIsNeeded)
      token =  (char *) _mbstok_s (NULL, (unsigned char *) DelimUtf8Converted, (unsigned char **) &context);
    else
      token = strtok_s (NULL, Delim, &context);
  }
  if (ConvertingIsNeeded)
    CLEAN_AND_ZERO_ARR (Delim);

  SendMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, oldid);
}

void SpellChecker::CheckVisible ()
{
  CLEAN_AND_ZERO_ARR (VisibleText);
  VisibleText = GetVisibleText (&VisibleTextOffset);
  VisibleTextLength = strlen (VisibleText);
  ClearAllUnderlines ();
  CheckText (VisibleText, VisibleTextOffset);
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
}

void SpellChecker::RecheckVisible ()
{
  int CodepageId = 0;
  CodepageId = (int) SendMsgToEditor (NppDataInstance, SCI_GETCODEPAGE, 0, 0);
  setEncodingById (CodepageId); // For now it just changes should we convert it to utf-8 or no
  if (AutoCheckText)
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
