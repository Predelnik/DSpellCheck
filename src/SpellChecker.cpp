#include "Aspell.h"
#include "CommonFunctions.h"
#include "MainDef.h"
#include "PluginInterface.h"
#include "Plugin.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "Scintilla.h"
#include "Suggestions.h"

#define DEFAULT_DELIMITERS ",.!?\\\":;{}()[]\\\\/=+-"

SpellChecker::SpellChecker (const TCHAR *IniFilePathArg, SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
                            Suggestions *SuggestionsInstanceArg)
{
  CurrentPosition = 0;
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
  AspellLoaded = FALSE;
  AutoCheckText = 0;
  AspellReinitSettings ();
  LockSuggestionsBoxHide = FALSE;
  AspellPath = 0;
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
    if (AspellLoaded)
      SettingsDlgInstance->GetSimpleDlg ()->AddAvalaibleLanguages (Language);

    SettingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (!AspellLoaded);
    SettingsDlgInstance->GetSimpleDlg ()->FillAspellInfo (AspellLoaded, AspellPath);
    SettingsDlgInstance->GetSimpleDlg ()->FillSugestionsNum (SuggestionsNum);
    SettingsDlgInstance->GetAdvancedDlg ()->FillDelimiters (DelimUtf8);
    break;
  case EID_APPLY_SETTINGS:
    SettingsDlgInstance->GetSimpleDlg ()->ApplySettings (this);
    SettingsDlgInstance->GetAdvancedDlg ()->ApplySettings (this);
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
  }
  return TRUE;
}

void SpellChecker::HideSuggestionBox ()
{
  if (!LockSuggestionsBoxHide)
    SuggestionsInstance->display (false);
}

#define STEP_SIZE 16384
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
    Range.chrg.cpMax = IteratorPos + STEP_SIZE;
    if (Range.chrg.cpMax > DocLength - 1)
      Range.chrg.cpMax = DocLength - 1;
    Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
    SendMsgToEditor (NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
    Result = CheckText (Range.lpstrText, IteratorPos, FIND_FIRST);
    CLEAN_AND_ZERO_ARR (Range.lpstrText);
    if (Result)
      break;

    IteratorPos += STEP_SIZE;

    if (IteratorPos > DocLength - 1)
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
  if (CurLine + 1 < LineCount)
    LineEndPos = SendMsgToEditor (NppDataInstance, SCI_POSITIONFROMLINE, CurLine + 1);
  else
    LineEndPos = DocLength - 1;
  int IteratorPos = LineEndPos + 1;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
  {
    Range.chrg.cpMin = IteratorPos - STEP_SIZE;
    Range.chrg.cpMax = IteratorPos;
    if (Range.chrg.cpMin < 0)
      Range.chrg.cpMin = 0;
    Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
    SendMsgToEditor (NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
    Result = CheckText (Range.lpstrText, Range.chrg.cpMin, FIND_LAST);
    CLEAN_AND_ZERO_ARR (Range.lpstrText);
    if (Result)
      break;

    IteratorPos -= STEP_SIZE;

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
  TCHAR *Buf = 0;
  SetStringSUtf8 (Buf, DEFAULT_DELIMITERS);
  SettingsDlgInstance->GetAdvancedDlg ()->SetDelimetersEdit (Buf);
  CLEAN_AND_ZERO_ARR (Buf);
}

char* SpellChecker::GetWordUnderMouse(){
  char *ret = NULL;
  POINT p;
  if(GetCursorPos(&p) != 0){
    ScreenToClient(GetScintillaWindow (NppDataInstance), &p);

    int initCharPos = SendMsgToEditor (NppDataInstance, SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);
    if(initCharPos != -1){
      ret = GetWordAt (initCharPos, VisibleText);
    }
  }
  return ret;
}

char *SpellChecker::GetWordAt (int CharPos, char *Text)
{
  char *UsedText = Text + CharPos - VisibleTextOffset;
  if (!DelimUtf8)
    return 0;

  while (*UsedText != 0 && UsedText >= Text) // Finding closest zero before mouse pos
    UsedText--;

  UsedText++; // Then find first token after this zero

  char *Delim = 0;
  char *Context = 0;

  if (ConvertingIsNeeded)
    SetStringSUtf8 (Delim, DelimUtf8Converted);
  // We're just taking the first token (basically repeating the same code as an in CheckVisible

  char *Res = 0;
  if (!ConvertingIsNeeded)
    Res = (char *) _mbstok_s ((unsigned char *) UsedText, (unsigned char *) DelimUtf8Converted, (unsigned char **) &Context);
  else
    Res = strtok_s (UsedText, Delim, &Context);
  if (Res - Text + VisibleTextOffset > CharPos)
    return 0;
  else
    return Res;
}

void SpellChecker::SetSuggestionsBoxTransparency ()
{
  // Set WS_EX_LAYERED on this window
  SetWindowLong(SuggestionsInstance->getHSelf (), GWL_EXSTYLE,
    GetWindowLong(SuggestionsInstance->getHSelf (), GWL_EXSTYLE) | WS_EX_LAYERED);
  SetLayeredWindowAttributes(SuggestionsInstance->getHSelf (), 0, (255 * 70) / 100, LWA_ALPHA);
  // Don't sure why but this helps to fix a bug with notepad++ window resizing
  // TODO: Fix it normal way
  SuggestionsInstance->display (true);
  SuggestionsInstance->display (false);
}

void SpellChecker::InitSuggestionsBox (long OverridePos, long OverrideLen)
{
  if (!AspellLoaded)
    return;
  long Pos = 0;
  POINT p;
  if (OverridePos == -1)
  {
    if (!AutoCheckText) // If there's no red underline let's do nothing
    {
      SuggestionsInstance->display (false);
      return;
    }

    GetCursorPos (&p);
    if (WindowFromPoint (p) != GetScintillaWindow (NppDataInstance))
    {
      return;
    }

    char *Word = GetWordUnderMouse ();
    if (!Word || !*Word || CheckWord (Word))
    {
      return;
    }

    Pos = Word - VisibleText + VisibleTextOffset;
    WUCLength = strlen (Word);
  }
  else
  {
    Pos = OverridePos;
    WUCLength = OverrideLen;
  }

  WUCPosition = Pos;
  int Line = SendMsgToEditor (NppDataInstance, SCI_LINEFROMPOSITION, Pos);
  int TextHeight = SendMsgToEditor (NppDataInstance, SCI_TEXTHEIGHT, Line);
  int TextWidth = SendMsgToEditor (NppDataInstance, SCI_TEXTWIDTH, STYLE_DEFAULT, (LPARAM) "_");
  int XPos = SendMsgToEditor (NppDataInstance, SCI_POINTXFROMPOSITION, 0, Pos);
  int YPos = SendMsgToEditor (NppDataInstance, SCI_POINTYFROMPOSITION, 0, Pos);

  p.x = XPos; p.y = YPos;
  ClientToScreen (GetScintillaWindow (NppDataInstance), &p);
  MoveWindow (SuggestionsInstance->getHSelf (), p.x, p.y + TextHeight - 3, 15, 15, TRUE);
  SuggestionsInstance->display ();
}

void SpellChecker::ShowSuggestionsMenu ()
{
  if (!AspellLoaded || WUCPosition - VisibleTextOffset < 0 || WUCPosition - VisibleTextOffset + WUCLength - 1 > VisibleTextLength)
    return; // Word is already off-screen

  int Pos = WUCPosition;
  Sci_TextRange Range;
  Range.chrg.cpMin = WUCPosition;
  Range.chrg.cpMax = WUCPosition + WUCLength;
  Range.lpstrText = new char [WUCLength + 1];
  PostMsgToEditor (NppDataInstance, SCI_SETSEL, Pos, Pos + WUCLength);
  SendMsgToEditor (NppDataInstance, SCI_GETTEXTRANGE, 0, (LPARAM) &Range);

  const AspellWordList *wl = aspell_speller_suggest (Speller, Range.lpstrText, -1);
  if (!wl)
    return;

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
    if (Result == MID_IGNOREALL)
    {
      aspell_speller_add_to_session (Speller, Range.lpstrText, WUCLength + 1);
      aspell_speller_save_all_word_lists (Speller);
      if (aspell_speller_error(Speller) != 0)
      {
        AspellErrorMsgBox(GetScintillaWindow (NppDataInstance), aspell_speller_error_message(Speller));
      }
      RecheckVisible ();
    }
    else if (Result == MID_ADDTODICTIONARY)
    {
      aspell_speller_add_to_personal(Speller, Range.lpstrText, WUCLength + 1);
      aspell_speller_save_all_word_lists (Speller);
      if (aspell_speller_error(Speller) != 0)
      {
        AspellErrorMsgBox(GetScintillaWindow (NppDataInstance), aspell_speller_error_message(Speller));
      }
      RecheckVisible ();
    }
    else if (Result <= Counter)
    {
      els = aspell_word_list_elements(wl);
      for (int i = 1; i <= Result; i++)
      {
        Suggestion = aspell_string_enumeration_next(els);
      }
      PostMsgToEditor (NppDataInstance, SCI_REPLACESEL, 0, (LPARAM) Suggestion);
    }
  }
  CLEAN_AND_ZERO_ARR (Range.lpstrText);
  CLEAN_AND_ZERO_ARR (Buf);
  CLEAN_AND_ZERO_ARR (MenuString);
}

void SpellChecker::UpdateAutocheckStatus (int SaveSetting)
{
  if (SaveSetting)
    SaveToIni (_T ("Autocheck"), AutoCheckText);

  CheckMenuItem(GetMenu(NppDataInstance->_nppHandle), get_funcItem ()[0]._cmdID, MF_BYCOMMAND | (AutoCheckText ? MF_CHECKED : MF_UNCHECKED));
  RecheckVisible ();
}

void SpellChecker::LoadSettings ()
{
  char *BufUtf8 = 0;
  LoadFromIni (AspellPath, _T ("Aspell_Path"), _T (""));
  AspellLoaded = LoadAspell (AspellPath);
  if (AspellLoaded)
    AspellReinitSettings ();

  LoadFromIni (AutoCheckText, _T ("Autocheck"), 0);
  UpdateAutocheckStatus (0);
  LoadFromIniUtf8 (Language, _T ("Language"), "En_Us");
  AspellReinitSettings ();
  LoadFromIniUtf8 (BufUtf8, _T ("Delimiters"), DEFAULT_DELIMITERS, TRUE);
  SetDelimiters (BufUtf8, 0);
  LoadFromIni (SuggestionsNum, _T ("Suggestions_Number"), 5);
  CLEAN_AND_ZERO_ARR (BufUtf8);
}

void SpellChecker::CreateWordUnderline (HWND ScintillaWindow, int start, int end)
{
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_INDICATORFILLRANGE, start, (end - start + 1));
}

void SpellChecker::RemoveWordUnderline (int start, int end)
{
  PostMsgToEditor (NppDataInstance, SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
  PostMsgToEditor (NppDataInstance, SCI_INDICATORCLEARRANGE, start, (end - start + 1));
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
    PostMsgToEditor (NppDataInstance, SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
    PostMsgToEditor (NppDataInstance, SCI_INDICATORCLEARRANGE, 0, length);
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

void SpellChecker::SetAspellPath (const TCHAR *Path)
{
  SetString (AspellPath, Path);
  AspellLoaded = LoadAspell (AspellPath);
  if (AspellLoaded)
    AspellReinitSettings ();

  TCHAR *Buf = 0;
  GetDefaultAspellPath (Buf);
  if (_tcscmp (Buf, Path) != 0)
    SaveToIni (_T ("Aspell_Path"), Path);
  else
    SaveToIni (_T ("Aspell_Path"), _T (""));
}

void SpellChecker::SaveToIni (const TCHAR *Name, const TCHAR *Value, BOOL InQuotes)
{
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

void SpellChecker::SaveToIni (const TCHAR *Name, int Value)
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  _itot_s (Value, Buf, 10);
  SaveToIni (Name, Buf);
}

void SpellChecker::SaveToIniUtf8 (const TCHAR *Name, const char *Value, BOOL InQuotes)
{
  TCHAR *Buf = 0;
  SetStringSUtf8 (Buf, Value);
  SaveToIni (Name, Buf, InQuotes);
  CLEAN_AND_ZERO_ARR (Buf);
}

void SpellChecker::LoadFromIni (TCHAR *&Value, const TCHAR *Name, const TCHAR *DefaultValue, BOOL InQuotes)
{
  CLEAN_AND_ZERO (Value);
  Value = new TCHAR[DEFAULT_BUF_SIZE];

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
void SpellChecker::SetLanguage (const char *Str)
{
  SetString (Language, Str);
  SaveToIniUtf8 (_T ("Language"), Language);
  AspellReinitSettings ();
}

const char *SpellChecker::GetDelimiters ()
{
  return DelimUtf8;
}

void SpellChecker::SetSuggestionsNum (int Num)
{
  SuggestionsNum = Num;
  SaveToIni (_T ("Suggestions_Number"), Num);
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
  CLEAN_AND_ZERO_ARR (DestBuf);
  CLEAN_AND_ZERO_ARR (SrcBuf);
  CLEAN_AND_ZERO_ARR (TargetBuf);

  if (SaveToIni)
    SaveToIniUtf8 (_T ("Delimiters"), DelimUtf8, TRUE);
  RecheckVisible ();
}

const char *SpellChecker::GetLanguage ()
{
  return Language;
}

BOOL SpellChecker::AspellReinitSettings ()
{
  if (!AspellLoaded)
    return FALSE;

  AspellConfig *spell_config = new_aspell_config();
  aspell_config_replace (spell_config, "encoding", "utf-8");
  if (Language && *Language)
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

  delete_aspell_speller (Speller);
  return TRUE;
}

BOOL SpellChecker::CheckWord (char *Word)
{
  if (!AspellLoaded)
    return TRUE;
  // Well Numbers have same codes for ansi and unicode I guess, so
  // If word contains number then it's probably just a number or some crazy name
  // TODO: add this to options
  for (char c = '0'; c <= '9'; c++)
  {
    if (strchr (Word, c))
      return TRUE;
  }

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

BOOL SpellChecker::CheckText (char *TextToCheck, long offset, CheckTextMode Mode)
{
  if (!TextToCheck || !*TextToCheck)
    return FALSE;

  HWND ScintillaWindow = GetScintillaWindow (NppDataInstance);
  int oldid = SendMsgToEditor (ScintillaWindow, NppDataInstance, SCI_GETINDICATORCURRENT);
  char *Context = 0; // Temporary variable for strtok_s usage
  char *Delim = 0;
  char *token;
  BOOL stop = FALSE;
  long ResultingWordEnd = -1, ResultingWordStart = -1;

  if (!DelimUtf8)
    return FALSE;

  if (ConvertingIsNeeded)
    SetStringSUtf8 (Delim, DelimUtf8Converted);
  // A little bit ugly but since only function in mbstr.h require unsigned char then
  // we're gonna do explicitly all conversations before call to them.
  if (!ConvertingIsNeeded)
    token = (char *) _mbstok_s ((unsigned char *) TextToCheck, (unsigned char *) DelimUtf8Converted, (unsigned char **) &Context);
  else
    token = strtok_s (TextToCheck, Delim, &Context);

  while (token)
  {
    if (token && !CheckWord (token))
    {
      long WordStart = offset + token - TextToCheck;
      long WordEnd = offset + token - TextToCheck + strlen (token) - 1;
      switch (Mode)
      {
      case UNDERLINE_ERRORS:
        CreateWordUnderline (ScintillaWindow, WordStart, WordEnd);
        break;
      case FIND_FIRST:
        if (WordEnd >= CurrentPosition)
        {
          SendMsgToEditor (NppDataInstance, SCI_SETSEL, WordStart, WordEnd + 1);
          stop = TRUE;
        }
        break;
      case FIND_LAST:
        {
          if (WordEnd >= CurrentPosition - 1)
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

    if (!ConvertingIsNeeded)
      token =  (char *) _mbstok_s (NULL, (unsigned char *) DelimUtf8Converted, (unsigned char **) &Context);
    else
      token = strtok_s (NULL, Delim, &Context);
  }
  if (ConvertingIsNeeded)
    CLEAN_AND_ZERO_ARR (Delim);

  PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, oldid);

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
      SendMsgToEditor (NppDataInstance, SCI_SETSEL, ResultingWordStart, ResultingWordEnd + 1);
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
    PostMsgToEditor (NppDataInstance, SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
    PostMsgToEditor (NppDataInstance, SCI_INDICATORCLEARRANGE, 0, length);
  }
}

void SpellChecker::CheckVisible ()
{
  CLEAN_AND_ZERO_ARR (VisibleText);
  VisibleText = GetVisibleText (&VisibleTextOffset);
  VisibleTextLength = strlen (VisibleText);
  ClearVisibleUnderlines ();
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
}

void SpellChecker::RecheckVisible ()
{
  int CodepageId = 0;
  if (!AspellLoaded)
  {
    ClearAllUnderlines ();
    return;
  }
  LockSuggestionsBoxHide = FALSE;
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
