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

#include "AbstractSpellerInterface.h"
#include "AspellInterface.h"
#include "HunspellInterface.h"

#include "Aspell.h"
#include "DownloadDicsDlg.h"
#include "iconv.h"
#include "CommonFunctions.h"
#include "LanguageName.h"
#include "LangList.h"
#include "MainDef.h"
#include "PluginInterface.h"
#include "Plugin.h"
#include "RemoveDics.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "SciLexer.h"
#include "Scintilla.h"
#include "SelectProxy.h"
#include "Suggestions.h"

#ifdef UNICODE
#define DEFAULT_DELIMITERS L",.!?\":;{}()[]\\/=+-^$*<>|#$@%&~\u2026\u2116\u2014\u00AB\u00BB\u2013\u2022\u00A9\u203A\u201C\u201D\u00B7\u00A0\u0060\u2192\u00d7"
#else // All non-ASCII symbols removed
#define DEFAULT_DELIMITERS L",.!?\":;{}()[]\\/=+-^$*<>|#$@%&~"
#endif

HWND GetScintillaWindow(const NppData *NppDataArg)
{
  int which = -1;
  SendMessage(NppDataArg->_nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
  if (which == -1)
    return 0;
  if (which == 1)
    return NppDataArg->_scintillaSecondHandle;
  return (which == 0) ? NppDataArg->_scintillaMainHandle : NppDataArg->_scintillaSecondHandle;
}

BOOL SendMsgToBothEditors (const NppData *NppDataArg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  if (isCurrentlyTerminating ())
    return FALSE;

  SendMessage (NppDataArg->_scintillaMainHandle, Msg, wParam, lParam);
  SendMessage (NppDataArg->_scintillaSecondHandle, Msg, wParam, lParam);
  return TRUE;
}

LRESULT SendMsgToActiveEditor (BOOL *ok, HWND ScintillaWindow, UINT Msg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
  bool terminating = isCurrentlyTerminating ();
  if (ok)
   *ok = !terminating;

  if (terminating)
    return 0;

  return SendMessage (ScintillaWindow, Msg, wParam, lParam);
}

LRESULT SendMsgToNpp (BOOL *ok, const NppData *NppDataArg, UINT Msg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
  bool terminating = isCurrentlyTerminating ();
  if (ok)
   *ok = !terminating;

  if (terminating)
    return 0;

  return SendMessage (NppDataArg->_nppHandle, Msg, wParam, lParam);
}

// Remember: it's better to use PostMsg wherever possible, to avoid gui update on each message send etc etc
// Also it's better to avoid get current scintilla window too much times, since it's obviously uses 1 SendMsg call
LRESULT PostMsgToActiveEditor (HWND ScintillaWindow, UINT Msg, WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/)
{
  return PostMessage(ScintillaWindow, Msg, wParam, lParam);
}

SpellChecker::SpellChecker (const wchar_t *IniFilePathArg, SettingsDlg *SettingsDlgInstanceArg, NppData *NppDataInstanceArg,
                            Suggestions *SuggestionsInstanceArg, LangList *LangListInstanceArg)
{
  CurrentPosition = 0;
  DelimUtf8 = 0;
  DelimUtf8Converted = 0;
  IniFilePath = 0;
  AspellLanguage = 0;
  AspellMultiLanguages = 0;
  HunspellLanguage = 0;
  HunspellMultiLanguages = 0;
  VisibleText = 0;
  DelimConverted = 0;
  VisibleTextLength = -1;
  SetString (IniFilePath, IniFilePathArg);
  SettingsDlgInstance = SettingsDlgInstanceArg;
  SuggestionsInstance = SuggestionsInstanceArg;
  NppDataInstance = NppDataInstanceArg;
  LangListInstance = LangListInstanceArg;
  AutoCheckText = 0;
  MultiLangMode = 0;
  AspellPath = 0;
  HunspellPath = 0;
  FileTypes = 0;
  CheckThose = 0;
  SBTrans = 0;
  SBSize = 0;
  CurWordList = 0;
  SelectedWord = 0;
  SuggestionsMode = 1;
  WUCLength = 0;
  WUCPosition = 0;
  WUCisRight = TRUE;
  CurrentScintilla = GetScintillaWindow (NppDataInstance);
  SuggestionMenuItems = 0;
  AspellSpeller = new AspellInterface (NppDataInstance->_nppHandle);
  HunspellSpeller = new HunspellInterface (NppDataInstance->_nppHandle);
  CurrentSpeller = AspellSpeller;
  LastSuggestions = 0;
  PrepareStringForConversion ();
  memset (ServerNames, 0, sizeof (ServerNames));
  memset (DefaultServers, 0, sizeof (DefaultServers));
  AddressIsSet = 0;
  SetString (DefaultServers[0], L"ftp://ftp.snt.utwente.nl/pub/software/openoffice/contrib/dictionaries/");
  SetString (DefaultServers[1], L"ftp://sunsite.informatik.rwth-aachen.de/pub/mirror/OpenOffice/contrib/dictionaries/");
  SetString (DefaultServers[2], L"ftp://gd.tuwien.ac.at/office/openoffice/contrib/dictionaries/");
  CurrentLangs = 0;
  DecodeNames = FALSE;
  ResetHotSpotCache ();
  ProxyUserName = 0;
  ProxyHostName = 0;
  ProxyPassword = 0;
  ProxyAnonymous = TRUE;
  ProxyType = 0;
  ProxyPort = 0;
  SettingsLoaded = FALSE;
  UseProxy = FALSE;
  SettingsToSave = new std::map<wchar_t *, DWORD, bool ( *)(wchar_t *, wchar_t *)> (SortCompare);
  BOOL ok;
  BOOL res = (SendMsgToNpp (&ok, NppDataInstance, NPPM_ALLOCATESUPPORTED, 0, 0) != 0);
  if (!ok)
    return;

  if (res)
    {
      SetUseAllocatedIds (TRUE);
      int Id;
      SendMsgToNpp (&ok, NppDataInstance, NPPM_ALLOCATECMDID, 350, (LPARAM) &Id);
      if (!ok)
        return;
      SetContextMenuIdStart (Id);
      SetLangsMenuIdStart (Id + 103);
    }
}

static const char Yo[] = "\xd0\x81";
static const char Ye[] = "\xd0\x95";
static const char yo[] = "\xd1\x91";
static const char ye[] = "\xd0\xb5";
static const char PunctuationApostrophe[] = "\xe2\x80\x99";

void SpellChecker::PrepareStringForConversion ()
{
  iconv_t Conv = iconv_open ("CHAR", "UTF-8");
  const char *InString [] = {Yo, yo, Ye, ye, PunctuationApostrophe};
  char **OutString [] = {&YoANSI, &yoANSI, &YeANSI, &yeANSI, &PunctuationApostropheANSI};
  char *Buf = 0;
  char *OutBuf = 0;
  const char *InBuf = 0;
  size_t InSize = 0;
  size_t OutSize = 0;
  size_t Res = 0;

  for (int i = 0; i < countof (InString); i++)
  {
    InSize = strlen (InString[i]) + 1;
    Buf = 0;
    SetString (Buf, InString[i]);
    InBuf = Buf;
    OutSize = Utf8Length (InString[i]) + 1;
    OutBuf = new char[OutSize];
    *OutString[i] = OutBuf;
    Res = iconv (Conv, &InBuf, &InSize, &OutBuf, &OutSize);
    CLEAN_AND_ZERO_ARR (Buf);
    if (Res == (size_t) - 1)
    {
      CLEAN_AND_ZERO_ARR (*OutString[i]);
    }
  }
  iconv_close (Conv);
}

SpellChecker::~SpellChecker ()
{
  CLEAN_AND_ZERO_STRING_VECTOR (LastSuggestions);
  CLEAN_AND_ZERO (AspellSpeller);
  CLEAN_AND_ZERO (HunspellSpeller);
  CLEAN_AND_ZERO_ARR (SelectedWord);
  CLEAN_AND_ZERO_ARR (DelimConverted);
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DelimUtf8);
  CLEAN_AND_ZERO_ARR (AspellLanguage);
  CLEAN_AND_ZERO_ARR (AspellMultiLanguages);
  CLEAN_AND_ZERO_ARR (HunspellLanguage);
  CLEAN_AND_ZERO_ARR (HunspellMultiLanguages);
  CLEAN_AND_ZERO_ARR (IniFilePath);
  CLEAN_AND_ZERO_ARR (AspellPath);
  CLEAN_AND_ZERO_ARR (HunspellPath);
  CLEAN_AND_ZERO_ARR (VisibleText);
  CLEAN_AND_ZERO_ARR (FileTypes);
  CLEAN_AND_ZERO_ARR (AdditionalHunspellPath);

  CLEAN_AND_ZERO_ARR (YoANSI);
  CLEAN_AND_ZERO_ARR (yoANSI);
  CLEAN_AND_ZERO_ARR (YeANSI);
  CLEAN_AND_ZERO_ARR (yeANSI);
  CLEAN_AND_ZERO_ARR (PunctuationApostropheANSI);
  CLEAN_AND_ZERO_ARR (ProxyHostName);
  CLEAN_AND_ZERO_ARR (ProxyUserName);
  CLEAN_AND_ZERO_ARR (ProxyPassword);
  for (int i = 0; i < countof (ServerNames); i++)
    CLEAN_AND_ZERO_ARR (ServerNames[i]);
  for (int i = 0; i < countof (DefaultServers); i++)
    CLEAN_AND_ZERO_ARR (DefaultServers[i]);

  std::map<wchar_t *, DWORD, bool ( *)(wchar_t *, wchar_t *)>::iterator it = SettingsToSave->begin ();
  for (; it != SettingsToSave->end (); ++it)
  {
    delete ((*it).first);
  }
  CLEAN_AND_ZERO (SettingsToSave);

  CLEAN_AND_ZERO (CurrentLangs);
}

void InsertSuggMenuItem (HMENU Menu, wchar_t *Text, BYTE Id, int InsertPos, BOOL Separator)
{
  MENUITEMINFO mi;
  memset(&mi, 0, sizeof(mi));
  mi.cbSize = sizeof(MENUITEMINFO);
  if (Separator)
  {
    mi.fType = MFT_SEPARATOR;
  }
  else
  {
    mi.fType = MFT_STRING;
    mi.fMask = MIIM_ID | MIIM_TYPE;
    if (!GetUseAllocatedIds ())
      mi.wID = MAKEWORD (Id, DSPELLCHECK_MENU_ID);
    else
      mi.wID = GetContextMenuIdStart () + Id;

    mi.dwTypeData = Text;
    mi.cch = static_cast<int> (wcslen (Text)) + 1;
  }
  if (InsertPos == -1)
    InsertMenuItem (Menu, GetMenuItemCount (Menu), TRUE, &mi);
  else
    InsertMenuItem (Menu, InsertPos, TRUE, &mi);
}

BOOL WINAPI SpellChecker::NotifyMessage (UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch (Msg)
  {
  case TM_MENU_RESULT:
    {
      ProcessMenuResult (wParam);
      return TRUE;
    }
    break;
  case TM_PRECALCULATE_MENU:
    {
      if (CheckTextNeeded () && SuggestionsMode == SUGGESTIONS_CONTEXT_MENU)
      {
        long Pos, Length;
        WUCisRight = GetWordUnderCursorIsRight (Pos, Length, TRUE);
        if (!WUCisRight)
        {
          WUCPosition = Pos;
          WUCLength = Length;
          FillSuggestionsMenu (0);
        }
      }

      PostMessage (NppDataInstance->_nppHandle, GetCustomGUIMessageId (CustomGUIMessage::SHOW_CALCULATED_MENU), 0, (LPARAM)SuggestionMenuItems);
      SuggestionMenuItems = 0;
    }
    break;
  case TM_WRITE_SETTING:
    {
      WriteSetting (lParam);
    }
    break;
  case TM_ADD_USER_SERVER:
    {
      wchar_t *Name = (wchar_t *) wParam;
      wchar_t *TrimmedName = 0;
      SetString (TrimmedName, Name);
      FtpTrim (TrimmedName);
      wchar_t *Buf = 0;
      for (int i = 0; i < countof (DefaultServers); i++)
      {
        SetString (Buf, DefaultServers[i]);
        FtpTrim (Buf);
        if (wcscmp (Buf, TrimmedName) == 0)
          goto add_user_server_cleanup; // Nothing is done in this case
      }
      for (int i = 0; i < countof (ServerNames); i++)
      {
        SetString (Buf, ServerNames[i]);
        FtpTrim (Buf);
        if (wcscmp (Buf, TrimmedName) == 0)
          goto add_user_server_cleanup; // Nothing is done in this case
      }
      // Then we're adding finally
      CLEAN_AND_ZERO_ARR (ServerNames[countof (ServerNames) - 1]);
      for (int i = countof (ServerNames) - 1; i > 0; i--)
      {
        ServerNames[i] = ServerNames[i - 1];
      }
      ServerNames[0] = 0;
      SetString (ServerNames[0], Name);
add_user_server_cleanup:
      CLEAN_AND_ZERO_ARR (Buf);
      CLEAN_AND_ZERO_ARR (Name);
      CLEAN_AND_ZERO_ARR (TrimmedName);
      ResetDownloadCombobox ();
      SaveSettings ();
    }
    break;
  default:
    break;
  }
  return TRUE;
}

void SpellChecker::SetSuggType (int SuggType)
{
  SuggestionsMode = SuggType;
  HideSuggestionBox ();
}

void SpellChecker::SetShowOnlyKnow (BOOL Value)
{
  ShowOnlyKnown = Value;
}

void SpellChecker::SetInstallSystem (BOOL Value)
{
  InstallSystem = Value;
}

BOOL SpellChecker::GetShowOnlyKnown ()
{
  return ShowOnlyKnown;
}

BOOL SpellChecker::GetInstallSystem ()
{
  return InstallSystem;
}

BOOL SpellChecker::GetDecodeNames ()
{
  return DecodeNames;
}

wchar_t *SpellChecker::GetLangByIndex (int i)
{
  return CurrentLangs->at(i).OrigName;
}

void SpellChecker::ReinitLanguageLists(BOOL UpdateDialogs)
{
  int SpellerId = LibMode;
  BOOL CurrentLangExists = FALSE;
  wchar_t *CurrentLang;

  AbstractSpellerInterface *SpellerToUse = (SpellerId == 1 ?
                                            (AbstractSpellerInterface *)HunspellSpeller :
                                            (AbstractSpellerInterface *)AspellSpeller);

  if (SpellerId == 0)
  {
    GetDownloadDics ()->display (false);
    GetRemoveDics ()->display (false);
  }

  if (SpellerId == 1)
    CurrentLang = HunspellLanguage;
  else
    CurrentLang = AspellLanguage;
  CLEAN_AND_ZERO (CurrentLangs);

  if (SpellerToUse->IsWorking ())
  {
    if (UpdateDialogs)
      SettingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (FALSE);
    std::vector <wchar_t *> *LangsFromSpeller =  SpellerToUse->GetLanguageList ();
    CurrentLangs = new std::vector<LanguageName> ();

    if (!LangsFromSpeller || LangsFromSpeller->size () == 0)
    {
      if (UpdateDialogs)
        SettingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (TRUE);
      return;
    }
    for (unsigned int i = 0; i < LangsFromSpeller->size (); i++)
    {
      LanguageName Lang (LangsFromSpeller->at (i), (SpellerId == 1 && DecodeNames)); // Using them only for Hunspell
      CurrentLangs->push_back (Lang);                               // TODO: Add option - use or not use aliases.
      if (wcscmp (Lang.OrigName, CurrentLang) == 0)
        CurrentLangExists = TRUE;
    }
    if (wcscmp (CurrentLang, L"<MULTIPLE>") == 0)
      CurrentLangExists = TRUE;

    CLEAN_AND_ZERO_STRING_VECTOR (LangsFromSpeller);
    std::sort (CurrentLangs->begin (), CurrentLangs->end (), DecodeNames ? CompareAliases : CompareOriginal);
    if (!CurrentLangExists && CurrentLangs->size () > 0)
    {
      if (SpellerId == 1)
        SetHunspellLanguage (CurrentLangs->at (0).OrigName);
      else
        SetAspellLanguage (CurrentLangs->at (0).OrigName);
      RecheckVisibleBothViews ();
    }
      if (UpdateDialogs)
      SettingsDlgInstance->GetSimpleDlg ()->AddAvailableLanguages (CurrentLangs,
                                                                   SpellerId == 1 ? HunspellLanguage : AspellLanguage,
                                                                   SpellerId == 1 ? HunspellMultiLanguages : AspellMultiLanguages,
                                                                   SpellerId == 1 ? HunspellSpeller : 0);
  }
  else
  {
    if (UpdateDialogs)
      SettingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (TRUE);
  }
}

int SpellChecker::GetLibMode ()
{
  return LibMode;
}

void SpellChecker::FillDialogs (BOOL NoDisplayCall)
{
  ReinitLanguageLists (TRUE);
  SettingsDlgInstance->GetSimpleDlg ()->SetLibMode (LibMode);
  SettingsDlgInstance->GetSimpleDlg ()->FillLibInfo (AspellSpeller->IsWorking () ? 2 - (!CurrentLangs || CurrentLangs->size () == 0) : 0, AspellPath, HunspellPath, AdditionalHunspellPath);
  SettingsDlgInstance->GetSimpleDlg ()->FillSugestionsNum (SuggestionsNum);
  SettingsDlgInstance->GetSimpleDlg ()->SetFileTypes (CheckThose, FileTypes);
  SettingsDlgInstance->GetSimpleDlg ()->SetCheckComments (CheckComments);
  SettingsDlgInstance->GetSimpleDlg ()->SetDecodeNames (DecodeNames);
  SettingsDlgInstance->GetSimpleDlg ()->SetSuggType (SuggestionsMode);
  SettingsDlgInstance->GetSimpleDlg ()->SetOneUserDic (OneUserDic);
  SettingsDlgInstance->GetAdvancedDlg ()->FillDelimiters (DelimUtf8);
  SettingsDlgInstance->GetAdvancedDlg ()->SetConversionOpts (IgnoreYo, ConvertSingleQuotes, RemoveBoundaryApostrophes);
  SettingsDlgInstance->GetAdvancedDlg ()->SetUnderlineSettings (UnderlineColor, UnderlineStyle);
  SettingsDlgInstance->GetAdvancedDlg ()->SetIgnore (IgnoreNumbers, IgnoreCStart, IgnoreCHave, IgnoreCAll, Ignore_, IgnoreSEApostrophe, IgnoreOneLetter);
  SettingsDlgInstance->GetAdvancedDlg ()->SetSuggBoxSettings (SBSize, SBTrans);
  SettingsDlgInstance->GetAdvancedDlg ()->SetBufferSize (BufferSize / 1024);
  if (!NoDisplayCall)
    SettingsDlgInstance->display ();
}

void SpellChecker::RecheckVisibleBothViews ()
{
  LRESULT OldLexer = Lexer;
  EncodingType OldEncoding = CurrentEncoding;
  BOOL ok;
  Lexer = SendMsgToActiveEditor (&ok, NppDataInstance->_scintillaMainHandle, SCI_GETLEXER);
  if (!ok)
    return;
  CurrentScintilla = NppDataInstance->_scintillaMainHandle;
  RecheckVisible ();

  CurrentScintilla = NppDataInstance->_scintillaSecondHandle;
  Lexer = SendMsgToActiveEditor (&ok, NppDataInstance->_scintillaSecondHandle, SCI_GETLEXER);
  if (!ok)
    return;
  RecheckVisible ();
  Lexer = OldLexer;
  CurrentEncoding = OldEncoding;
  AspellSpeller->SetEncoding (CurrentEncoding);
  HunspellSpeller->SetEncoding (CurrentEncoding);
}

BOOL WINAPI SpellChecker::NotifyNetworkEvent (DWORD Event)
{
  if (!CurrentScintilla)
    return FALSE; // If scintilla is dead there's nothing else to do
  switch (Event)
  {
  case EID_DOWNLOAD_SELECTED:
    GetDownloadDics ()->DownloadSelected ();
    break;
  case EID_FILL_FILE_LIST:
    GetDownloadDics ()->FillFileList ();
    break;
  case EID_CANCEL_DOWNLOAD:
    // Do nothing, just unflag event
    break;
  case EID_KILLNETWORKTHREAD:
    return FALSE;
  }
  return TRUE;
}

BOOL WINAPI SpellChecker::NotifyEvent (DWORD Event)
{
  if (Event == EID_KILLTHREAD || isCurrentlyTerminating())
    return false;

  CurrentScintilla = GetScintillaWindow (NppDataInstance); // All operations should be done with current scintilla anyway

  switch (Event)
    {
    case  EID_FILL_DIALOGS:
      FillDialogs ();
      break;
    case EID_APPLY_SETTINGS:
      SettingsDlgInstance->GetSimpleDlg ()->ApplySettings (this);
      SettingsDlgInstance->GetAdvancedDlg ()->ApplySettings (this);
      FillDialogs (TRUE);
      SaveSettings ();
      CheckFileName (); // Cause filters may change
      RefreshUnderlineStyle ();
      RecheckVisibleBothViews ();
      break;
    case EID_APPLY_MULTI_LANG_SETTINGS:
      LangListInstance->ApplyChoice (this);
      SaveSettings ();
      break;

    case EID_APPLY_PROXY_SETTINGS:
      GetSelectProxy ()->ApplyChoice (this);
      SaveSettings ();
      break;

    case EID_COPY_MISSPELLINGS_TO_CLIPBOARD:
      CopyMisspellingsToClipboard ();
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
    case EID_RECHECK_VISIBLE_BOTH_VIEWS:
      {
        RecheckVisibleBothViews ();
      }
      break;
    case EID_RECHECK_INTERSECTION:
      RecheckVisible (TRUE);
      break;
    case EID_SWITCH_AUTOCHECK:
      if (!SettingsLoaded)
        return FALSE;

      SwitchAutoCheck ();
      break;
    case EID_KILLTHREAD:
      return FALSE;
    case EID_INIT_SUGGESTIONS_BOX:
      if (SuggestionsMode == SUGGESTIONS_BOX)
        InitSuggestionsBox ();
      else
        {
          /* placeholder */
        }
      break;
    case EID_SHOW_SUGGESTION_MENU:
      FillSuggestionsMenu (SuggestionsInstance->GetPopupMenu ());
      SendMessage (SuggestionsInstance->getHSelf (), WM_SHOWANDRECREATEMENU, 0, 0);
      break;
    case EID_HIDE_SUGGESTIONS_BOX:
      HideSuggestionBox ();
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
    case EID_FILL_DOWNLOAD_DICS_DIALOG:
      FillDownloadDics ();
      SendNetworkEvent (EID_FILL_FILE_LIST);
      break;
    case EID_INIT_DOWNLOAD_COMBOBOX:
      ResetDownloadCombobox ();
      break;
    case EID_REMOVE_SELECTED_DICS:
      GetRemoveDics ()->RemoveSelected (this);
      break;
    case EID_UPDATE_SELECT_PROXY:
      GetSelectProxy ()->SetOptions (UseProxy, ProxyHostName, ProxyUserName, ProxyPassword, ProxyPort, ProxyAnonymous, ProxyType);
      break;
    case EID_UPDATE_LANG_LISTS:
      ReinitLanguageLists (TRUE);
      break;
    case EID_UPDATE_LANG_LISTS_NO_GUI:
      ReinitLanguageLists (FALSE);
      break;
    case EID_UPDATE_LANGS_MENU:
      DoPluginMenuInclusion ();
      break;

    case EID_UPDATE_FROM_REMOVE_DICS_OPTIONS:
      GetRemoveDics ()->UpdateOptions (this);
      SaveSettings ();
      break;

    case EID_UPDATE_REMOVE_DICS_OPTIONS:
      GetRemoveDics ()->SetCheckBoxes (RemoveUserDics, RemoveSystem);
      break;

    case EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS:
      GetDownloadDics ()->UpdateOptions (this);
      GetDownloadDics ()->UpdateListBox ();
      SaveSettings ();
      break;

    case EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS_NO_UPDATE:
      GetDownloadDics ()->UpdateOptions (this);
      SaveSettings ();
      break;

    case EID_LIB_CHANGE:
      SettingsDlgInstance->GetSimpleDlg ()->ApplyLibChange (this);
      SettingsDlgInstance->GetSimpleDlg ()->FillLibInfo (AspellSpeller->IsWorking () ? 2 - (!CurrentLangs || CurrentLangs->size () == 0) : 0, AspellPath, HunspellPath, AdditionalHunspellPath);
      RecheckVisibleBothViews ();
      SaveSettings ();
      break;

    case EID_LANG_CHANGE:
      {
        BOOL ok;
        Lexer = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLEXER);
        if (!ok)
          break;
        RecheckVisible ();
      }
      break;

    case EID_HIDE_DOWNLOAD_DICS: // TODO: Make it async, though it's really hard
      GetDownloadDics ()->display (false);
      break;

    case EID_SHOW_SELECT_PROXY:
      GetSelectProxy ()->display ();
      break;
      /*
      case EID_APPLYMENUACTION:
      ApplyMenuActions ();
      break;
      */
    }
  return TRUE;
}

void SpellChecker::SetRemoveUserDics (BOOL Value)
{
  RemoveUserDics = Value;
}

void SpellChecker::SetRemoveSystem (BOOL Value)
{
  RemoveSystem = Value;
}

BOOL SpellChecker::GetRemoveUserDics ()
{
  return RemoveUserDics;
}

BOOL SpellChecker::GetRemoveSystem ()
{
  return RemoveSystem;
}

void SpellChecker::DoPluginMenuInclusion (BOOL Invalidate)
{
  BOOL Res;
  MENUITEMINFO Mif;
  HMENU DSpellCheckMenu = GetDSpellCheckMenu ();
  if (!DSpellCheckMenu)
    return;
  HMENU LangsSubMenu = GetLangsSubMenu (DSpellCheckMenu);
  if (LangsSubMenu)
    DestroyMenu (LangsSubMenu);
  wchar_t *CurLang = (LibMode == 1) ? HunspellLanguage : AspellLanguage;
  HMENU NewMenu = CreatePopupMenu ();
  if (!Invalidate)
  {
    if (CurrentLangs)
    {
      if (CurrentLangs->size () > 0)
      {
        for (unsigned int i = 0; i < CurrentLangs->size (); i++)
        {
          int Checked = (wcscmp (CurLang, CurrentLangs->at(i).OrigName) == 0) ? (MFT_RADIOCHECK | MF_CHECKED) : MF_UNCHECKED;
          Res = AppendMenu (NewMenu, MF_STRING | Checked, GetUseAllocatedIds () ? i + GetLangsMenuIdStart () : MAKEWORD (i, LANGUAGE_MENU_ID), DecodeNames ? CurrentLangs->at(i).AliasName : CurrentLangs->at(i).OrigName);
          if (!Res)
            return;
        }
        int Checked = (wcscmp (CurLang, L"<MULTIPLE>") == 0) ? (MFT_RADIOCHECK | MF_CHECKED)  : MF_UNCHECKED;
        Res = AppendMenu (NewMenu, MF_STRING | Checked, GetUseAllocatedIds () ? MULTIPLE_LANGS + GetLangsMenuIdStart () : MAKEWORD (MULTIPLE_LANGS, LANGUAGE_MENU_ID), L"Multiple Languages");
        Res = AppendMenu (NewMenu, MF_SEPARATOR, 0, 0);
        Res = AppendMenu (NewMenu, MF_STRING, GetUseAllocatedIds () ? CUSTOMIZE_MULTIPLE_DICS + GetLangsMenuIdStart () : MAKEWORD (CUSTOMIZE_MULTIPLE_DICS, LANGUAGE_MENU_ID), L"Set Multiple Languages...");
        if (LibMode == 1) // Only Hunspell supported
        {
          Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? DOWNLOAD_DICS + GetLangsMenuIdStart () : MAKEWORD (DOWNLOAD_DICS, LANGUAGE_MENU_ID), L"Download More Languages...");
          Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? REMOVE_DICS + GetLangsMenuIdStart () : MAKEWORD (REMOVE_DICS, LANGUAGE_MENU_ID), L"Remove Unneeded Languages...");
        }
      }
      else if (LibMode == 1)
        Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? DOWNLOAD_DICS + GetLangsMenuIdStart () : MAKEWORD (DOWNLOAD_DICS, LANGUAGE_MENU_ID), L"Download Languages...");
    }
  }
  else
    Res = AppendMenu (NewMenu, MF_STRING | MF_DISABLED, 0, L"Loading...");

  Mif.fMask = MIIM_SUBMENU | MIIM_STATE;
  Mif.cbSize = sizeof (MENUITEMINFO);
  Mif.hSubMenu = (CurrentLangs ? NewMenu : 0);
  Mif.fState = (!CurrentLangs ? MFS_GRAYED : MFS_ENABLED);

  SetMenuItemInfo (DSpellCheckMenu, QUICK_LANG_CHANGE_ITEM, TRUE, &Mif);
}

void SpellChecker::FillDownloadDics ()
{
  GetDownloadDics ()->SetOptions (ShowOnlyKnown, InstallSystem);
}

void SpellChecker::ResetDownloadCombobox ()
{
  HWND TargetCombobox = GetDlgItem (GetDownloadDics ()->getHSelf (), IDC_ADDRESS);
  wchar_t Buf[DEFAULT_BUF_SIZE];
  ComboBox_GetText (TargetCombobox, Buf, DEFAULT_BUF_SIZE);
  if (AddressIsSet)
  {
    PreserveCurrentAddressIndex ();
  }
  ComboBox_ResetContent (TargetCombobox);
  for (int i = 0; i < countof (DefaultServers); i++)
  {
    ComboBox_AddString (TargetCombobox, DefaultServers[i]);
  }
  for (int i = 0; i < countof (ServerNames); i++)
  {
    if (*ServerNames[i])
      ComboBox_AddString (TargetCombobox, ServerNames[i]);
  }
  if (LastUsedAddress < USER_SERVER_CONST)
    ComboBox_SetCurSel (TargetCombobox, LastUsedAddress);
  else
    ComboBox_SetCurSel (TargetCombobox, LastUsedAddress - USER_SERVER_CONST + countof (DefaultServers));
  AddressIsSet = 1;
}

void SpellChecker::PreserveCurrentAddressIndex ()
{
  HWND TargetCombobox = GetDlgItem (GetDownloadDics ()->getHSelf (), IDC_ADDRESS);
  if (!TargetCombobox)
    return;
  wchar_t CurText [DEFAULT_BUF_SIZE];
  wchar_t *Buf = 0;
  ComboBox_GetText (TargetCombobox, CurText, DEFAULT_BUF_SIZE);
  FtpTrim (CurText);
  for (int i = 0; i < countof (ServerNames); i++)
  {
    SetString (Buf, DefaultServers[i]);
    FtpTrim (Buf);
    if (wcscmp (Buf, CurText) == 0)
    {
      LastUsedAddress = i;
      goto cleanup;
    }
  };
  for (int i = 0; i < countof (ServerNames); i++)
  {
    SetString (Buf, ServerNames[i]);
    FtpTrim (Buf);
    if (wcscmp (Buf, CurText) == 0)
    {
      LastUsedAddress = USER_SERVER_CONST + i;
      goto cleanup;
    }
  }
  LastUsedAddress = 0;
cleanup:
  CLEAN_AND_ZERO_ARR (Buf);
}

/*
void SpellChecker::ApplyMenuActions ()
{
}
*/

// For now just int option, later maybe choose option type in wParam
void SpellChecker::WriteSetting (LPARAM lParam)
{
  std::pair<wchar_t *, DWORD> *x = (std::pair<wchar_t *, DWORD> *) lParam;
  if (SettingsToSave->find (x->first) == SettingsToSave->end ())
    (*SettingsToSave)[x->first] = x->second;
  else
  {
    CLEAN_AND_ZERO_ARR (x->first);
  }
  CLEAN_AND_ZERO (x);
}

void SpellChecker::SetCheckComments (BOOL Value)
{
  CheckComments = Value;
}

void SpellChecker::CheckFileName ()
{
  wchar_t *Context = 0;
  wchar_t *Token = 0;
  wchar_t *FileTypesCopy = 0;
  wchar_t FullPath[MAX_PATH];
  SetString (FileTypesCopy, FileTypes);
  Token = wcstok_s (FileTypesCopy, L";", &Context);
  CheckTextEnabled = !CheckThose;
  BOOL ok;
  SendMsgToNpp (&ok, NppDataInstance, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM) FullPath);
  if (!ok)
    goto cleanup;

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
    Token = wcstok_s (NULL, L";", &Context);
  }
  Lexer = SendMsgToActiveEditor (0, GetCurrentScintilla (), SCI_GETLEXER);

cleanup:
  CLEAN_AND_ZERO_ARR (FileTypesCopy);
}

LRESULT SpellChecker::GetStyle (int Pos)
{
  return SendMsgToActiveEditor (0, GetCurrentScintilla (), SCI_GETSTYLEAT, Pos);
}
// Actually for all languages which operate mostly in strings it's better to check only comments
// TODO: Fix it
int SpellChecker::CheckWordInCommentOrString (LRESULT Style)
{
  switch (Lexer)
  {
  case SCLEX_CONTAINER:
  case SCLEX_NULL:
    return TRUE;
  case SCLEX_PYTHON:
    switch (Style)
    {
    case SCE_P_COMMENTLINE:
    case SCE_P_COMMENTBLOCK:
    case SCE_P_STRING:
    case SCE_P_TRIPLE:
    case SCE_P_TRIPLEDOUBLE:
      return TRUE;
    default:
      return FALSE;
    };
  case SCLEX_CPP:
  case SCLEX_OBJC:
  case SCLEX_BULLANT:
    switch (Style)
    {
    case SCE_C_COMMENT:
    case SCE_C_COMMENTLINE:
    case SCE_C_COMMENTDOC:
    case SCE_C_COMMENTLINEDOC:
    case SCE_C_STRING:
      return TRUE;
    default:
      return FALSE;
    };
  case SCLEX_HTML:
  case SCLEX_XML:
    switch (Style)
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
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PERL:
    switch (Style)
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
    switch (Style)
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
    switch (Style)
    {
    case SCE_PROPS_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ERRORLIST:
    return FALSE;
  case SCLEX_MAKEFILE:
    switch (Style)
    {
    case SCE_MAKE_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_BATCH:
    switch (Style)
    {
    case SCE_BAT_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_XCODE:
    return FALSE;
  case SCLEX_LATEX:
    switch (Style)
    {
    case SCE_L_DEFAULT:
    case SCE_L_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_LUA:
    switch (Style)
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
    switch (Style)
    {
    case SCE_DIFF_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CONF:
    switch (Style)
    {
    case SCE_CONF_COMMENT:
    case SCE_CONF_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PASCAL:
    switch (Style)
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
    switch (Style)
    {
    case SCE_AVE_COMMENT:
    case SCE_AVE_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ADA:
    switch (Style)
    {
    case SCE_ADA_STRING:
    case SCE_ADA_COMMENTLINE:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_LISP:
    switch (Style)
    {
    case SCE_LISP_COMMENT:
    case SCE_LISP_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_RUBY:
    switch (Style)
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
    switch (Style)
    {
    case SCE_EIFFEL_COMMENTLINE:
    case SCE_EIFFEL_STRING:
      return TRUE;
    default:
      return FALSE;
    };
  case SCLEX_TCL:
    switch (Style)
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
    switch (Style)
    {
    case SCE_NNCRONTAB_COMMENT:
    case SCE_NNCRONTAB_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_BAAN:
    switch (Style)
    {
    case SCE_BAAN_COMMENT:
    case SCE_BAAN_COMMENTDOC:
    case SCE_BAAN_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MATLAB:
    switch (Style)
    {
    case SCE_MATLAB_COMMENT:
    case SCE_MATLAB_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_SCRIPTOL:
    switch (Style)
    {
    case SCE_SCRIPTOL_COMMENTLINE:
    case SCE_SCRIPTOL_COMMENTBLOCK:
    case SCE_SCRIPTOL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ASM:
    switch (Style)
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
    switch (Style)
    {
    case SCE_F_COMMENT:
    case SCE_F_STRING1:
    case SCE_F_STRING2:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CSS:
    switch (Style)
    {
    case SCE_CSS_COMMENT:
    case SCE_CSS_DOUBLESTRING:
    case SCE_CSS_SINGLESTRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_POV:
    switch (Style)
    {
    case SCE_POV_COMMENT:
    case SCE_POV_COMMENTLINE:
    case SCE_POV_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_LOUT:
    switch (Style)
    {
    case SCE_LOUT_COMMENT:
    case SCE_LOUT_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ESCRIPT:
    switch (Style)
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
    switch (Style)
    {
    case SCE_PS_COMMENT:
    case SCE_PS_DSC_COMMENT:
    case SCE_PS_TEXT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_NSIS:
    switch (Style)
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
    switch (Style)
    {
    case SCE_MMIXAL_COMMENT:
    case SCE_MMIXAL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CLW:
    switch (Style)
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
    switch (Style)
    {
    case SCE_YAML_COMMENT:
    case SCE_YAML_TEXT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_TEX:
    switch (Style)
    {
    case SCE_TEX_TEXT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_METAPOST:
    switch (Style)
    {
    case SCE_METAPOST_TEXT:
    case SCE_METAPOST_DEFAULT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_FORTH:
    switch (Style)
    {
    case SCE_FORTH_COMMENT:
    case SCE_FORTH_COMMENT_ML:
    case SCE_FORTH_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ERLANG:
    switch (Style)
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
    switch (Style)
    {
    case SCE_MSSQL_COMMENT:
    case SCE_MSSQL_LINE_COMMENT:
    case SCE_MSSQL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_VERILOG:
    switch (Style)
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
    switch (Style)
    {
    case SCE_KIX_COMMENT:
    case SCE_KIX_STRING1:
    case SCE_KIX_STRING2:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_GUI4CLI:
    switch (Style)
    {
    case SCE_GC_COMMENTLINE:
    case SCE_GC_COMMENTBLOCK:
    case SCE_GC_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_SPECMAN:
    switch (Style)
    {
    case SCE_SN_COMMENTLINE:
    case SCE_SN_COMMENTLINEBANG:
    case SCE_SN_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_AU3:
    switch (Style)
    {
    case SCE_AU3_COMMENT:
    case SCE_AU3_COMMENTBLOCK:
    case SCE_AU3_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_APDL:
    switch (Style)
    {
    case SCE_APDL_COMMENT:
    case SCE_APDL_COMMENTBLOCK:
    case SCE_APDL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_BASH:
    switch (Style)
    {
    case SCE_SH_COMMENTLINE:
    case SCE_SH_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ASN1:
    switch (Style)
    {
    case SCE_ASN1_COMMENT:
    case SCE_ASN1_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_VHDL:
    switch (Style)
    {
    case SCE_VHDL_COMMENT:
    case SCE_VHDL_COMMENTLINEBANG:
    case SCE_VHDL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_CAML:
    switch (Style)
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
    switch (Style)
    {
    case SCE_B_COMMENT:
    case SCE_B_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_HASKELL:
    switch (Style)
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
    switch (Style)
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
    switch (Style)
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
    switch (Style)
    {
    case SCE_ST_STRING:
    case SCE_ST_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_FLAGSHIP:
    switch (Style)
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
    switch (Style)
    {
    case SCE_CSOUND_COMMENT:
    case SCE_CSOUND_COMMENTBLOCK:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_INNOSETUP:
    switch (Style)
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
    switch (Style)
    {
    case SCE_OPAL_COMMENT_BLOCK:
    case SCE_OPAL_COMMENT_LINE:
    case SCE_OPAL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_SPICE:
    switch (Style)
    {
    case SCE_SPICE_COMMENTLINE:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_D:
    switch (Style)
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
    switch (Style)
    {
    case SCE_CMAKE_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_GAP:
    switch (Style)
    {
    case SCE_GAP_COMMENT:
    case SCE_GAP_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PLM:
    switch (Style)
    {
    case SCE_PLM_COMMENT:
    case SCE_PLM_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_PROGRESS:
    switch (Style)
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
    switch (Style)
    {
    case SCE_ABAQUS_COMMENT:
    case SCE_ABAQUS_COMMENTBLOCK:
    case SCE_ABAQUS_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_ASYMPTOTE:
    switch (Style)
    {
    case SCE_ASY_COMMENT:
    case SCE_ASY_COMMENTLINE:
    case SCE_ASY_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_R:
    switch (Style)
    {
    case SCE_R_COMMENT:
    case SCE_R_STRING:
    case SCE_R_STRING2:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MAGIK:
    switch (Style)
    {
    case SCE_MAGIK_COMMENT:
    case SCE_MAGIK_HYPER_COMMENT:
    case SCE_MAGIK_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_POWERSHELL:
    switch (Style)
    {
    case SCE_POWERSHELL_COMMENT:
    case SCE_POWERSHELL_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MYSQL:
    switch (Style)
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
    switch (Style)
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
    switch (Style)
    {
    case SCE_SORCUS_STRING:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_POWERPRO:
    switch (Style)
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
    switch (Style)
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
    switch (Style)
    {
    case SCE_TXT2TAGS_COMMENT:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_A68K:
    switch (Style)
    {
    case SCE_A68K_COMMENT:
    case SCE_A68K_STRING1:
    case SCE_A68K_STRING2:
      return TRUE;
    default:
      return FALSE;
    }
  case SCLEX_MODULA:
    switch (Style)
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
  BOOL ok;
  CurrentPosition = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETCURRENTPOS);
  if (!ok) return;
  auto CurLine = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, CurrentPosition);
  if (!ok) return;
  auto LineStartPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, CurLine);
  if (!ok) return;
  auto DocLength = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLENGTH);
  if (!ok) return;
  auto IteratorPos = LineStartPos;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
  {
    Range.chrg.cpMin = static_cast<long> (IteratorPos);
    Range.chrg.cpMax = static_cast<long> (IteratorPos + BufferSize);
    int IgnoreOffsetting = 0;
    if (Range.chrg.cpMax > DocLength)
    {
      IgnoreOffsetting = 1;
      Range.chrg.cpMax = static_cast<long> (DocLength);
    }
    Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
    SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
    if (!ok) return;
    char *IteratingStart = Range.lpstrText + Range.chrg.cpMax - Range.chrg.cpMin - 1;
    char *IteratingChar = IteratingStart;
    if (!IgnoreOffsetting)
    {
      if (CurrentEncoding == ENCODING_UTF8)
      {
        while (Utf8IsCont (*IteratingChar) && Range.lpstrText < IteratingChar)
          IteratingChar--;

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
    SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_COLOURISE, Range.chrg.cpMin, Range.chrg.cpMax);
    if (!ok) return;
    SCNotification scn;
    scn.nmhdr.code = SCN_SCROLLED;
    SendMsgToNpp (&ok, NppDataInstance, WM_NOTIFY, 0, (LPARAM )&scn); // To fix bug with hotspots being removed
    if (!ok) return;
    Result = CheckText (Range.lpstrText, static_cast<long> (IteratorPos), FIND_FIRST);
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
  BOOL ok;
  CurrentPosition = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETCURRENTPOS);
  if (!ok) return;
  auto CurLine = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, CurrentPosition);
  if (!ok) return;
  auto DocLength = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLENGTH);
  if (!ok) return;
  auto LineEndPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLINEENDPOSITION, CurLine);
  if (!ok) return;

  auto IteratorPos = LineEndPos;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
    {
      Range.chrg.cpMin = static_cast<long> (IteratorPos - BufferSize);
      Range.chrg.cpMax = static_cast<long> (IteratorPos);
      int IgnoreOffsetting = 0;
      if (Range.chrg.cpMin < 0)
        {
          Range.chrg.cpMin = 0;
          IgnoreOffsetting = 1;
        }
      Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
      SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
      if (!ok)
        {
          CLEAN_AND_ZERO_ARR (Range.lpstrText);
          return;
        }
      char *IteratingStart = Range.lpstrText;
      char *IteratingChar = IteratingStart;
      if (!IgnoreOffsetting)
        {
          if (CurrentEncoding == ENCODING_UTF8)
            {
              while (Utf8IsCont (*IteratingChar) && *IteratingChar)
                IteratingChar++;

              while ((!Utf8chr ( DelimUtf8Converted, IteratingChar)) && *IteratingChar)
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
      auto offset = IteratingChar - IteratingStart;
      SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_COLOURISE, Range.chrg.cpMin + offset, Range.chrg.cpMax);
      if (!ok)
        {
          CLEAN_AND_ZERO_ARR (Range.lpstrText);
          return;
        }
      SCNotification scn;
      scn.nmhdr.code = SCN_SCROLLED;
      SendMsgToNpp (&ok, NppDataInstance, WM_NOTIFY, 0, (LPARAM) &scn);
      if (!ok)
        {
          CLEAN_AND_ZERO_ARR (Range.lpstrText);
          return;
        }
      Result = CheckText (Range.lpstrText + offset, static_cast<long> (Range.chrg.cpMin + offset), FIND_LAST);
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

HWND SpellChecker::GetCurrentScintilla ()
{
  return CurrentScintilla;
}

BOOL SpellChecker::GetWordUnderCursorIsRight (long &Pos, long &Length, BOOL UseTextCursor)
{
  BOOL Ret = TRUE;
  POINT p;
  std::ptrdiff_t iniwchar_tPos = -1;
  LRESULT SelectionStart = 0;
  LRESULT SelectionEnd = 0;

  if (!UseTextCursor)
  {
    if (GetCursorPos(&p) == 0)
      return TRUE;

    auto *scintilla = GetScintillaWindow (NppDataInstance);
    if (!scintilla)
      return TRUE;
    ScreenToClient(scintilla, &p);

    BOOL ok;
    iniwchar_tPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);
    if (!ok) return TRUE;
  }
  else
  {
    BOOL ok;
    SelectionStart = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETSELECTIONSTART);
    if (!ok) return TRUE;
    SelectionEnd = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETSELECTIONEND);
    if (!ok) return TRUE;
    iniwchar_tPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETCURRENTPOS);
    if (!ok) return TRUE;
  }

  if (iniwchar_tPos != -1)
  {
    BOOL ok;
    auto Line = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, iniwchar_tPos);
    if (!ok) return TRUE;
    auto LineLength = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINELENGTH, Line);
    if (!ok) return TRUE;
    char *Buf = new char[LineLength + 1];
    SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLINE, Line, (LPARAM) Buf);
    if (!ok) return TRUE;
    Buf [LineLength] = 0;
    auto Offset = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, Line);
    if (!ok) return TRUE;
    char *Word = GetWordAt (static_cast<long> (iniwchar_tPos), Buf, static_cast<long> (Offset));
    if (!Word || !*Word)
    {
      Ret = TRUE;
    }
    else
    {
      Pos = static_cast<long> (Word - Buf + Offset);
      long PosEnd = Pos + static_cast<long> (strlen(Word));
      CheckSpecialDelimeters (Word, Buf, Pos, PosEnd);
      long WordLen = PosEnd - Pos;
      if (SelectionStart != SelectionEnd && (SelectionStart != Pos || SelectionEnd != Pos + WordLen))
      {
        CLEAN_AND_ZERO_ARR (Buf);
        return TRUE;
      }
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
  return Ret;
}

char *SpellChecker::GetWordAt (long CharPos, char *Text, long Offset)
{
  char *UsedText = 0;
  if (!DelimUtf8)
    return 0;

  char *Iterator = Text + CharPos - Offset;

  if (CurrentEncoding == ENCODING_UTF8)
  {
    if (Utf8chr ( DelimUtf8Converted, Iterator))
      Iterator = Utf8Dec (Text, Iterator);

    if (Iterator == 0)
      return 0;

    while ((!Utf8chr ( DelimUtf8Converted, Iterator)) && Text < Iterator)
      Iterator = (char *) Utf8Dec (Text, Iterator);
  }
  else
  {
    if (strchr ( DelimConverted, *Iterator))
      Iterator--;
    if (Iterator < Text)
      return 0;

    while (!strchr (DelimConverted, *Iterator) && Text < Iterator)
      Iterator--;

    if (Iterator < Text)
      return 0;
  }

  UsedText = Iterator;

  if (CurrentEncoding == ENCODING_UTF8)
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
  if (CurrentEncoding == ENCODING_UTF8)
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
  SetLayeredWindowAttributes(SuggestionsInstance->getHSelf (), 0, static_cast<BYTE> ((255 * SBTrans) / 100), LWA_ALPHA);
  SuggestionsInstance->display (true);
  SuggestionsInstance->display (false);
}

void SpellChecker::InitSuggestionsBox ()
{
  if (!CurrentSpeller->IsWorking ())
    return;
  POINT p;
  if (!CheckTextNeeded ()) // If there's no red underline let's do nothing
  {
    SuggestionsInstance->display (false);
    return;
  }

  GetCursorPos (&p);
  auto *scintilla = GetScintillaWindow (NppDataInstance);
  if (!scintilla || WindowFromPoint (p) != scintilla)
  {
    return;
  }

  long Pos, Length;
  if (GetWordUnderCursorIsRight (Pos, Length))
  {
    return;
  }
  WUCLength = Length;
  WUCPosition = Pos;
  BOOL ok;
  auto Line = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, WUCPosition);
  if (!ok) return;
  auto TextHeight = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_TEXTHEIGHT, Line);
  if (!ok) return;
  auto XPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POINTXFROMPOSITION, 0, WUCPosition);
  if (!ok) return;
  auto YPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POINTYFROMPOSITION, 0, WUCPosition);
  if (!ok) return;

  p.x = static_cast<LONG> (XPos);
  p.y = static_cast<LONG> (YPos);
  RECT R;
  GetWindowRect (GetCurrentScintilla (), &R);
  scintilla = GetScintillaWindow (NppDataInstance);
  if (!scintilla)
    return;

  ClientToScreen (scintilla, &p);
  if (R.top > p.y + TextHeight - 3 || R.left > p.x || R.bottom < p.y + TextHeight - 3 + SBSize ||  R.right < p.x + SBSize)
    return;
  MoveWindow (SuggestionsInstance->getHSelf (), p.x, p.y + static_cast<int> (TextHeight) - 3, SBSize, SBSize, TRUE);
  SuggestionsInstance->display ();
}

void SpellChecker::ProcessMenuResult (WPARAM MenuId)
{
  if ((!GetUseAllocatedIds () && HIBYTE (MenuId) != DSPELLCHECK_MENU_ID &&
       HIBYTE (MenuId) != LANGUAGE_MENU_ID)
      || (GetUseAllocatedIds () && ((int) MenuId < GetContextMenuIdStart () || (int) MenuId > GetContextMenuIdStart () + 350)))
    return;
  int UsedMenuId = 0;
  if (GetUseAllocatedIds ())
  {
    UsedMenuId = ((int) MenuId < GetLangsMenuIdStart () ? DSPELLCHECK_MENU_ID : LANGUAGE_MENU_ID);
  }
  else
  {
    UsedMenuId = HIBYTE (MenuId);
  }

  switch (UsedMenuId)
  {
  case  DSPELLCHECK_MENU_ID:
    {
      char *AnsiBuf = 0;
      WPARAM Result = 0;
      if (!GetUseAllocatedIds ())
        Result = LOBYTE (MenuId);
      else
        Result = MenuId - GetContextMenuIdStart ();

      if (Result != 0)
      {
        if (Result == MID_IGNOREALL)
        {
          ApplyConversions (SelectedWord);
          CurrentSpeller->IgnoreAll (SelectedWord);
          WUCLength = strlen (SelectedWord);
          BOOL ok;
          SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_SETSEL, WUCPosition + WUCLength , WUCPosition + WUCLength );
          if (!ok) return;
          RecheckVisibleBothViews ();
        }
        else if (Result == MID_ADDTODICTIONARY)
        {
          ApplyConversions (SelectedWord);
          CurrentSpeller->AddToDictionary (SelectedWord);
          WUCLength = strlen (SelectedWord);
          BOOL ok;
          SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_SETSEL, WUCPosition + WUCLength , WUCPosition + WUCLength );
          if (!ok) return;
          RecheckVisibleBothViews ();
        }
        else if ((unsigned int)Result <= LastSuggestions->size ())
        {
          if (CurrentEncoding == ENCODING_ANSI)
            SetStringSUtf8 (AnsiBuf, LastSuggestions->at (Result - 1));
          else
            SetString (AnsiBuf, LastSuggestions->at (Result - 1));

          BOOL ok;
          SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_REPLACESEL, 0, (LPARAM) AnsiBuf);
          if (!ok) return;

          CLEAN_AND_ZERO_ARR (AnsiBuf);
        }
      }
    }
    break;
  case LANGUAGE_MENU_ID:
    {
      WPARAM Result = 0;
      if (!GetUseAllocatedIds ())
        Result = LOBYTE (MenuId);
      else
        Result = MenuId - GetLangsMenuIdStart ();

      wchar_t *LangString = 0;
      if (Result == MULTIPLE_LANGS)
      {
        LangString = L"<MULTIPLE>";
      }
      else if (Result == CUSTOMIZE_MULTIPLE_DICS ||
               Result == DOWNLOAD_DICS ||
               Result == REMOVE_DICS)
      {
        // All actions are done in GUI thread in that case
        return;
      }
      else
        LangString = CurrentLangs->at (Result).OrigName;
      DoPluginMenuInclusion (TRUE);

      if (LibMode == 0)
        SetAspellLanguage (LangString);
      else
        SetHunspellLanguage (LangString);

      ReinitLanguageLists (TRUE);
      UpdateLangsMenu ();
      RecheckVisibleBothViews ();
      SaveSettings ();
      break;
    }
  }
}

void SpellChecker::FillSuggestionsMenu (HMENU Menu)
{
  if (!CurrentSpeller->IsWorking ())
    return; // Word is already off-screen

  int Pos = WUCPosition;
  Sci_TextRange Range;
  wchar_t *Buf = 0;
  Range.chrg.cpMin = WUCPosition;
  Range.chrg.cpMax = WUCPosition + static_cast<long> (WUCLength);
  Range.lpstrText = new char [WUCLength + 1];
  PostMsgToActiveEditor (GetCurrentScintilla (), SCI_SETSEL, Pos, Pos + WUCLength);
  if (SuggestionsMode == SUGGESTIONS_BOX)
    {
      // PostMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL, Pos, Pos + WUCLength);
    }
  else
    {
      SuggestionMenuItems = new std::vector <SuggestionsMenuItem *>;
    }

  BOOL ok;
  SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
  if (!ok)
    {
      CLEAN_AND_ZERO_ARR (Range.lpstrText);
      return;
    }

  SetString (SelectedWord, Range.lpstrText);
  ApplyConversions (SelectedWord);

  CLEAN_AND_ZERO_STRING_VECTOR (LastSuggestions);
  LastSuggestions = CurrentSpeller->GetSuggestions (SelectedWord);
  if (!LastSuggestions)
    return;

  for (size_t i = 0; i < LastSuggestions->size (); i++)
    {
      if (i >= (unsigned int) SuggestionsNum)
        break;
      SetStringSUtf8 (Buf, LastSuggestions->at (i));
      if (SuggestionsMode == SUGGESTIONS_BOX)
        InsertSuggMenuItem (Menu, Buf, static_cast<BYTE> (i + 1), -1);
      else
        SuggestionMenuItems->push_back (new SuggestionsMenuItem (Buf, static_cast<BYTE> (i + 1)));
    }

  if (LastSuggestions->size () > 0)
    {
      if (SuggestionsMode == SUGGESTIONS_BOX)
        InsertSuggMenuItem (Menu, L"", 0, 103, TRUE);
      else
        SuggestionMenuItems->push_back (new SuggestionsMenuItem (L"", 0, TRUE));
    }

  wchar_t *MenuString = new wchar_t [WUCLength + 50 + 1]; // Add "" to dictionary
  char *BufUtf8 = 0;
  if (CurrentEncoding == ENCODING_UTF8)
    SetString (BufUtf8, Range.lpstrText);
  else
    SetStringDUtf8 (BufUtf8, Range.lpstrText);
  ApplyConversions (BufUtf8);
  SetStringSUtf8 (Buf, BufUtf8);
  swprintf (MenuString, L"Ignore \"%s\" for Current Session", Buf);
  if (SuggestionsMode == SUGGESTIONS_BOX)
    InsertSuggMenuItem (Menu, MenuString, MID_IGNOREALL, -1);
  else
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (MenuString, MID_IGNOREALL));
  swprintf (MenuString, L"Add \"%s\" to Dictionary", Buf);
  if (SuggestionsMode == SUGGESTIONS_BOX)
    InsertSuggMenuItem (Menu, MenuString, MID_ADDTODICTIONARY, -1);
  else
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (MenuString, MID_ADDTODICTIONARY));

  if (SuggestionsMode == SUGGESTIONS_CONTEXT_MENU)
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (L"", 0, TRUE));

  CLEAN_AND_ZERO_ARR (Range.lpstrText);
  CLEAN_AND_ZERO_ARR (Buf);
  CLEAN_AND_ZERO_ARR (BufUtf8);

  CLEAN_AND_ZERO_ARR (MenuString);
}

void SpellChecker::UpdateAutocheckStatus (int SaveSetting)
{
  if (SaveSetting)
    SaveSettings ();

  SendMsgToNpp (0, NppDataInstance, NPPM_SETMENUITEMCHECK, get_funcItem ()[0]._cmdID, AutoCheckText);
  SendMessage (NppDataInstance->_nppHandle, GetCustomGUIMessageId (CustomGUIMessage::AUTOCHECK_STATE_CHANGED), AutoCheckText, 0);
}

void SpellChecker::SetCheckThose (int CheckThoseArg)
{
  CheckThose = CheckThoseArg;
}

void SpellChecker::SetFileTypes (wchar_t *FileTypesArg)
{
  SetString (FileTypes, FileTypesArg);
}

void SpellChecker::SetHunspellMultipleLanguages (const char *MultiLanguagesArg)
{
  SetString (HunspellMultiLanguages, MultiLanguagesArg);
}

void SpellChecker::SetAspellMultipleLanguages (const char *MultiLanguagesArg)
{
  SetString (AspellMultiLanguages, MultiLanguagesArg);
}

void SpellChecker::RefreshUnderlineStyle ()
{
  SendMsgToBothEditors(NppDataInstance, SCI_INDICSETSTYLE, SCE_ERROR_UNDERLINE, UnderlineStyle);
  SendMsgToBothEditors(NppDataInstance, SCI_INDICSETFORE, SCE_ERROR_UNDERLINE, UnderlineColor);
}

void SpellChecker::SetUnderlineColor (int Value)
{
  UnderlineColor = Value;
}

void SpellChecker::SetUnderlineStyle (int Value)
{
  UnderlineStyle = Value;
}

void SpellChecker::SetProxyUserName (wchar_t *Str)
{
  SetString (ProxyUserName, Str);
}

void SpellChecker::SetProxyHostName (wchar_t *Str)
{
  SetString (ProxyHostName, Str);
}

void SpellChecker::SetProxyPassword (wchar_t *Str)
{
  SetString (ProxyPassword, Str);
}

void SpellChecker::SetProxyPort (int Value)
{
  ProxyPort = Value;
}

void SpellChecker::SetUseProxy (BOOL Value)
{
  UseProxy = Value;
}

void SpellChecker::SetProxyAnonymous (BOOL Value)
{
  ProxyAnonymous = Value;
}

void SpellChecker::SetProxyType (int Value)
{
  ProxyType = Value;
}

wchar_t *SpellChecker::GetProxyUserName ()
{
  return ProxyUserName;
}

wchar_t *SpellChecker::GetProxyHostName ()
{
  return ProxyHostName;
}

wchar_t *SpellChecker::GetProxyPassword ()
{
  return ProxyPassword;
}

int SpellChecker::GetProxyPort ()
{
  return ProxyPort;
}

BOOL SpellChecker::GetUseProxy ()
{
  return UseProxy;
}

BOOL SpellChecker::GetProxyAnonymous ()
{
  return ProxyAnonymous;
}

int SpellChecker::GetProxyType ()
{
  return ProxyType;
}

void SpellChecker::SetIgnore (BOOL IgnoreNumbersArg, BOOL IgnoreCStartArg, BOOL IgnoreCHaveArg,
                              BOOL IgnoreCAllArg, BOOL Ignore_Arg, BOOL IgnoreSEApostropheArg, BOOL IgnoreOneLetterArg)
{
  IgnoreNumbers = IgnoreNumbersArg;
  IgnoreCStart = IgnoreCStartArg;
  IgnoreCHave = IgnoreCHaveArg;
  IgnoreCAll = IgnoreCAllArg;
  Ignore_ = Ignore_Arg;
  IgnoreSEApostrophe = IgnoreSEApostropheArg;
  IgnoreOneLetter = IgnoreOneLetterArg;
}

void SpellChecker::GetDefaultHunspellPath (wchar_t *&Path)
{
  Path = new wchar_t[MAX_PATH];
  wcscpy (Path, IniFilePath);
  wchar_t *Pointer = wcsrchr (Path, L'\\');
  *Pointer = 0;
  wcscat (Path, L"\\Hunspell");
}

void SpellChecker::SaveSettings ()
{
  FILE *Fp;
  _wfopen_s (&Fp, IniFilePath, L"w"); // Cleaning settings file (or creating it)
  fclose (Fp);
  if (!SettingsLoaded)
    return;
  SaveToIni (L"Autocheck", AutoCheckText, 1);
  SaveToIni (L"Hunspell_Multiple_Languages", HunspellMultiLanguages, L"");
  SaveToIni (L"Aspell_Multiple_Languages", AspellMultiLanguages, L"");
  SaveToIni (L"Hunspell_Language", HunspellLanguage, L"en_GB");
  SaveToIni (L"Aspell_Language", AspellLanguage, L"en");
  SaveToIni (L"Remove_User_Dics_On_Dic_Remove", RemoveUserDics, 0);
  SaveToIni (L"Remove_Dics_For_All_Users", RemoveSystem, 0);
  SaveToIni (L"Show_Only_Known", ShowOnlyKnown, TRUE);
  SaveToIni (L"Install_Dictionaries_For_All_Users", InstallSystem, FALSE);
  wchar_t Buf[DEFAULT_BUF_SIZE];
  for (int i = 0; i < countof (ServerNames); i++)
  {
    if (!*ServerNames[i])
      continue;
    swprintf (Buf, L"Server_Address[%d]", i);
    SaveToIni (Buf, ServerNames[i], L"");
  }
  SaveToIni (L"Suggestions_Control", SuggestionsMode, 1);
  SaveToIni (L"Ignore_Yo", IgnoreYo, 0);
  SaveToIni (L"Convert_Single_Quotes_To_Apostrophe", ConvertSingleQuotes, 1);
  SaveToIni (L"Remove_Ending_And_Beginning_Apostrophe", RemoveBoundaryApostrophes, 1);
  SaveToIni (L"Check_Only_Comments_And_Strings", CheckComments, 1);
  SaveToIni (L"Check_Those_\\_Not_Those", CheckThose, 1);
  SaveToIni (L"File_Types", FileTypes, L"*.*");
  SaveToIni (L"Ignore_Having_Number", IgnoreNumbers, 1);
  SaveToIni (L"Ignore_Start_Capital", IgnoreCStart, 0);
  SaveToIni (L"Ignore_Have_Capital", IgnoreCHave, 1);
  SaveToIni (L"Ignore_All_Capital", IgnoreCAll, 1);
  SaveToIni (L"Ignore_With_", Ignore_, 1);
  SaveToIni (L"Ignore_That_Start_or_End_with_'", IgnoreSEApostrophe, 0);
  SaveToIni (L"Ignore_One_Letter", IgnoreOneLetter, 0);
  SaveToIni (L"Underline_Color", UnderlineColor, 0x0000ff);
  SaveToIni (L"Underline_Style", UnderlineStyle, INDIC_SQUIGGLE);
  wchar_t *Path = 0;
  GetDefaultAspellPath (Path);
  SaveToIni (L"Aspell_Path", AspellPath, Path);
  CLEAN_AND_ZERO_ARR (Path);
  GetDefaultHunspellPath (Path);
  SaveToIni (L"User_Hunspell_Path", HunspellPath, Path);
  SaveToIni (L"System_Hunspell_Path", AdditionalHunspellPath, L".\\plugins\\config\\Hunspell");
  CLEAN_AND_ZERO_ARR (Path);
  SaveToIni (L"Suggestions_Number", SuggestionsNum, 5);
  char *DefaultDelimUtf8 = 0;
  SetStringDUtf8 (DefaultDelimUtf8, DEFAULT_DELIMITERS);
  SaveToIniUtf8 (L"Delimiters", DelimUtf8, DefaultDelimUtf8, TRUE);
  CLEAN_AND_ZERO_ARR (DefaultDelimUtf8);
  SaveToIni (L"Find_Next_Buffer_Size", BufferSize / 1024, 4);
  SaveToIni (L"Suggestions_Button_Size", SBSize, 15);
  SaveToIni (L"Suggestions_Button_Opacity", SBTrans, 70);
  SaveToIni (L"Library", LibMode, 1);
  PreserveCurrentAddressIndex ();
  SaveToIni (L"Last_Used_Address_Index", LastUsedAddress, 0);
  SaveToIni (L"Decode_Language_Names", DecodeNames, TRUE);
  SaveToIni (L"United_User_Dictionary(Hunspell)", OneUserDic, FALSE);

  SaveToIni (L"Use_Proxy", UseProxy, FALSE);
  SaveToIni (L"Proxy_User_Name", ProxyUserName, L"anonymous");
  SaveToIni (L"Proxy_Host_Name", ProxyHostName, L"");
  SaveToIni (L"Proxy_Password", ProxyPassword, L"");
  SaveToIni (L"Proxy_Port", ProxyPort, 808);
  SaveToIni (L"Proxy_Is_Anonymous", ProxyAnonymous, TRUE);
  SaveToIni (L"Proxy_Type", ProxyType, 0);
  std::map<wchar_t *, DWORD, bool ( *)(wchar_t *, wchar_t *)>::iterator it = SettingsToSave->begin ();
  for (; it != SettingsToSave->end (); ++it)
  {
    SaveToIni ((*it).first, LOWORD ((*it).second), HIWORD ((*it).second));
  }
}

void SpellChecker::SetDecodeNames (BOOL Value)
{
  DecodeNames = Value;
}

void SpellChecker::SetOneUserDic (BOOL Value)
{
  OneUserDic = Value;
  HunspellSpeller->SetUseOneDic (Value);
}

BOOL SpellChecker::GetOneUserDic ()
{
  return OneUserDic;
}

void SpellChecker::SetLibMode (int i)
{
  LibMode = i;
  if (i == 0)
  {
    AspellReinitSettings ();
    CurrentSpeller = AspellSpeller;
  }
  else
  {
    CurrentSpeller = HunspellSpeller;
    HunspellReinitSettings (0);
  }
}

void SpellChecker::LoadSettings ()
{
  char *BufUtf8 = 0;
  wchar_t *Path = 0;
  wchar_t *TBuf = 0;
  SettingsLoaded = TRUE;
  GetDefaultAspellPath (Path);
  LoadFromIni (AspellPath, L"Aspell_Path", Path);
  CLEAN_AND_ZERO_ARR (Path);
  GetDefaultHunspellPath (Path);
  LoadFromIni (HunspellPath, L"User_Hunspell_Path", Path);
  CLEAN_AND_ZERO_ARR (Path);

  AdditionalHunspellPath = 0;
  LoadFromIni (AdditionalHunspellPath, L"System_Hunspell_Path", L".\\plugins\\config\\Hunspell");

  LoadFromIni (SuggestionsMode, L"Suggestions_Control", 1);
  LoadFromIni (AutoCheckText, L"Autocheck", 1);
  UpdateAutocheckStatus (0);
  LoadFromIni (AspellMultiLanguages, L"Aspell_Multiple_Languages", L"");
  LoadFromIni (HunspellMultiLanguages, L"Hunspell_Multiple_Languages", L"");
  LoadFromIni (TBuf, L"Aspell_Language", L"en");
  SetAspellLanguage (TBuf);
  CLEAN_AND_ZERO_ARR (TBuf);
  LoadFromIni (TBuf, L"Hunspell_Language", L"en_GB");
  SetHunspellLanguage (TBuf);
  CLEAN_AND_ZERO_ARR (TBuf);

  SetStringDUtf8 (BufUtf8, DEFAULT_DELIMITERS);
  LoadFromIniUtf8 (BufUtf8, L"Delimiters", BufUtf8);
  SetDelimiters (BufUtf8);
  LoadFromIni (SuggestionsNum, L"Suggestions_Number", 5);
  LoadFromIni (IgnoreYo, L"Ignore_Yo", 0);
  LoadFromIni (ConvertSingleQuotes, L"Convert_Single_Quotes_To_Apostrophe", 1);
  LoadFromIni (RemoveBoundaryApostrophes, L"Remove_Ending_And_Beginning_Apostrophe", 1);
  LoadFromIni (CheckThose , L"Check_Those_\\_Not_Those", 1);
  LoadFromIni (FileTypes, L"File_Types", L"*.*");
  LoadFromIni (CheckComments, L"Check_Only_Comments_And_Strings", 1);
  LoadFromIni (UnderlineColor, L"Underline_Color", 0x0000ff);
  LoadFromIni (UnderlineStyle, L"Underline_Style", INDIC_SQUIGGLE);
  LoadFromIni (IgnoreNumbers, L"Ignore_Having_Number", 1);
  LoadFromIni (IgnoreCStart, L"Ignore_Start_Capital", 0);
  LoadFromIni (IgnoreCHave, L"Ignore_Have_Capital", 1);
  LoadFromIni (IgnoreCAll, L"Ignore_All_Capital", 1);
  LoadFromIni (IgnoreOneLetter, L"Ignore_One_Letter", 0);
  LoadFromIni (Ignore_, L"Ignore_With_", 1);
  int Value;
  LoadFromIni (Value, L"United_User_Dictionary(Hunspell)", FALSE);
  SetOneUserDic (Value);
  LoadFromIni (IgnoreSEApostrophe, L"Ignore_That_Start_or_End_with_'", 0);

  HunspellSpeller->SetDirectory (HunspellPath);
  HunspellSpeller->SetAdditionalDirectory (AdditionalHunspellPath);
  AspellSpeller->Init (AspellPath);
  int x;
  LoadFromIni (x, L"Library", 1);
  SetLibMode (x);
  int Size, Trans;
  LoadFromIni (Size, L"Suggestions_Button_Size", 15);
  LoadFromIni (Trans, L"Suggestions_Button_Opacity", 70);
  SetSuggBoxSettings (Size, Trans, 0);
  LoadFromIni (Size, L"Find_Next_Buffer_Size", 4);
  SetBufferSize (Size);
  RefreshUnderlineStyle ();
  CLEAN_AND_ZERO_ARR (BufUtf8);
  LoadFromIni (ShowOnlyKnown, L"Show_Only_Known", TRUE);
  LoadFromIni (InstallSystem, L"Install_Dictionaries_For_All_Users", FALSE);
  wchar_t Buf[DEFAULT_BUF_SIZE];
  for (int i = 0; i < countof (ServerNames); i++)
  {
    swprintf (Buf, L"Server_Address[%d]", i);
    LoadFromIni (ServerNames[i], Buf, L"");
  }
  LoadFromIni (LastUsedAddress, L"Last_Used_Address_Index", 0);
  LoadFromIni (RemoveUserDics, L"Remove_User_Dics_On_Dic_Remove", 0);
  LoadFromIni (RemoveSystem, L"Remove_Dics_For_All_Users", 0);
  LoadFromIni (DecodeNames, L"Decode_Language_Names", TRUE);

  LoadFromIni (UseProxy, L"Use_Proxy", FALSE);
  LoadFromIni (ProxyUserName, L"Proxy_User_Name", L"anonymous");
  LoadFromIni (ProxyHostName, L"Proxy_Host_Name", L"");
  LoadFromIni (ProxyPassword, L"Proxy_Password", L"");
  LoadFromIni (ProxyPort, L"Proxy_Port", 808);
  LoadFromIni (ProxyAnonymous, L"Proxy_Is_Anonymous", TRUE);
  LoadFromIni (ProxyType, L"Proxy_Type", 0);
}

void SpellChecker::CreateWordUnderline (HWND ScintillaWindow, long start, long end)
{
  PostMsgToActiveEditor (ScintillaWindow, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
  PostMsgToActiveEditor (ScintillaWindow, SCI_INDICATORFILLRANGE, start, (end - start + 1));
}

void SpellChecker::RemoveUnderline (HWND ScintillaWindow, long start, long end)
{
  if (end < start)
    return;
  PostMsgToActiveEditor (ScintillaWindow, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
  PostMsgToActiveEditor (ScintillaWindow, SCI_INDICATORCLEARRANGE, start, (end - start + 1));
}


void SpellChecker::GetVisibleLimits (long &Start, long &Finish)
{
  BOOL ok;
  auto top = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETFIRSTVISIBLELINE);
  if (!ok) goto ErrorBranch;
  auto bottom = top + SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINESONSCREEN);
  if (!ok) goto ErrorBranch;
  top = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_DOCLINEFROMVISIBLE, top);
  if (!ok) goto ErrorBranch;
  bottom = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_DOCLINEFROMVISIBLE, bottom);
  if (!ok) goto ErrorBranch;
  auto LineCount = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLINECOUNT);
  if (!ok) goto ErrorBranch;
  Start = static_cast<long> (SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, top));
  if (!ok) goto ErrorBranch;
  // Not using end of line position cause utf-8 symbols could be more than one char
  // So we use next line start as the end of our visible text
  if (bottom + 1 < LineCount)
    {
      Finish = static_cast<long> (SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, bottom + 1));
      if (!ok) goto ErrorBranch;
    }
  else
    {
      Finish = static_cast<long> (SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTLENGTH));
      if (!ok) goto ErrorBranch;
    }
  return;

ErrorBranch:
  Start = 0;
  Finish = 0;
}

char *SpellChecker::GetVisibleText(long *offset, BOOL NotIntersectionOnly)
{
  Sci_TextRange range;
  GetVisibleLimits (range.chrg.cpMin, range.chrg.cpMax);

  if (range.chrg.cpMax < 0 || range.chrg.cpMin > range.chrg.cpMax)
    return 0;

  PreviousA = range.chrg.cpMin;
  PreviousB = range.chrg.cpMax;

  if (NotIntersectionOnly)
  {
    if (range.chrg.cpMin < PreviousA && range.chrg.cpMax >= PreviousA)
      range.chrg.cpMax = PreviousA - 1;
    else if (range.chrg.cpMax > PreviousB && range.chrg.cpMin <= PreviousB)
      range.chrg.cpMin = PreviousB + 1;
  }

  char *Buf = new char [range.chrg.cpMax - range.chrg.cpMin + 1]; // + one byte for terminating zero
  range.lpstrText = Buf;
  BOOL ok;
  SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTRANGE, NULL , (LPARAM)&range );
  if (!ok)
    {
      CLEAN_AND_ZERO_ARR (Buf);
      return NULL;
    }
  *offset = range.chrg.cpMin;
  Buf[range.chrg.cpMax - range.chrg.cpMin] = 0;
  return Buf;
}

void SpellChecker::ClearAllUnderlines ()
{
  BOOL ok;
  auto length = SendMsgToActiveEditor(&ok, GetCurrentScintilla (), SCI_GETLENGTH);
  if (!ok)
    return;
  if (length > 0)
  {
    PostMsgToActiveEditor (GetCurrentScintilla (), SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
    PostMsgToActiveEditor (GetCurrentScintilla (), SCI_INDICATORCLEARRANGE, 0, length);
  }
}

void SpellChecker::Cleanup()
{
  CLEAN_AND_ZERO_ARR (AspellLanguage);
  CLEAN_AND_ZERO_ARR (HunspellLanguage);
  CLEAN_AND_ZERO_ARR (AspellMultiLanguages);
  CLEAN_AND_ZERO_ARR (HunspellMultiLanguages);
  CLEAN_AND_ZERO_ARR (DelimUtf8);
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DelimConverted);
  CLEAN_AND_ZERO_ARR (AspellPath);
}

void SpellChecker::SetAspellPath (const wchar_t *Path)
{
  SetString (AspellPath, Path);
  AspellReinitSettings ();
}

void SpellChecker::SetHunspellPath (const wchar_t *Path)
{
  SetString (HunspellPath, Path);
  HunspellReinitSettings (1);
}

void SpellChecker::SetHunspellAdditionalPath (const wchar_t *Path)
{
  if (!*Path)
    return;
  SetString (AdditionalHunspellPath, Path);
  HunspellReinitSettings (1);
}

void SpellChecker::SaveToIni (const wchar_t *Name, const wchar_t *Value, const wchar_t *DefaultValue, BOOL InQuotes)
{
  if (!Name || !Value)
    return;

  if (DefaultValue && wcscmp (Value, DefaultValue) == 0)
    return;

  if (InQuotes)
  {
    auto Len = 1 + wcslen (Value) + 1 + 1;
    wchar_t *Buf = new wchar_t[Len];
    swprintf (Buf, L"\"%s\"", Value);
    WritePrivateProfileString (L"SpellCheck", Name, Buf, IniFilePath);
    CLEAN_AND_ZERO_ARR (Buf);
  }
  else
  {
    WritePrivateProfileString (L"SpellCheck", Name, Value, IniFilePath);
  }
}

void SpellChecker::SaveToIni (const wchar_t *Name, int Value, int DefaultValue)
{
  if (!Name)
    return;

  if (Value == DefaultValue)
    return;

  wchar_t Buf[DEFAULT_BUF_SIZE];
  _itow_s (Value, Buf, 10);
  SaveToIni (Name, Buf, 0);
}

void SpellChecker::SaveToIniUtf8 (const wchar_t *Name, const char *Value,  const char *DefaultValue, BOOL InQuotes)
{
  if (!Name || !Value)
    return;

  if (DefaultValue && strcmp (Value, DefaultValue) == 0)
    return;

  wchar_t *Buf = 0;
  SetStringSUtf8 (Buf, Value);
  SaveToIni (Name, Buf, 0, InQuotes);
  CLEAN_AND_ZERO_ARR (Buf);
}

void SpellChecker::LoadFromIni (wchar_t *&Value, const wchar_t *Name, const wchar_t *DefaultValue, BOOL InQuotes)
{
  if (!Name || !DefaultValue)
    return;

  CLEAN_AND_ZERO_ARR (Value);
  Value = new wchar_t [DEFAULT_BUF_SIZE];

  GetPrivateProfileString (L"SpellCheck", Name, DefaultValue, Value, DEFAULT_BUF_SIZE, IniFilePath);

  if (InQuotes)
  {
    auto Len = wcslen (Value);
    // Proof check for quotes
    if (Value[0] != '\"' || Value[Len] != '\"')
    {
      wcscpy_s (Value, DEFAULT_BUF_SIZE, DefaultValue);
      return;
    }

    for (int i = 0; i < Len; i++)
      Value[i] = Value[i + 1];

    Value[Len - 1] = 0;
  }
}

void SpellChecker::LoadFromIni (int &Value, const wchar_t *Name, int DefaultValue)
{
  if (!Name)
    return;

  wchar_t BufDefault[DEFAULT_BUF_SIZE];
  wchar_t *Buf = 0;
  _itow_s (DefaultValue, BufDefault, 10);
  LoadFromIni (Buf, Name, BufDefault);
  Value = _wtoi (Buf);
  CLEAN_AND_ZERO_ARR (Buf);
}

void SpellChecker::LoadFromIniUtf8 (char *&Value, const wchar_t *Name, const char *DefaultValue, BOOL InQuotes)
{
  if (!Name || !DefaultValue)
    return;

  wchar_t *BufDefault = 0;
  wchar_t *Buf = 0;
  SetStringSUtf8 (BufDefault, DefaultValue);
  LoadFromIni (Buf, Name, BufDefault, InQuotes);
  SetStringDUtf8 (Value, Buf);
  CLEAN_AND_ZERO_ARR (Buf);
  CLEAN_AND_ZERO_ARR (BufDefault);
}

// Here parameter is in ANSI (may as well be utf-8 cause only English I guess)
void SpellChecker::SetAspellLanguage (const wchar_t *Str)
{
  SetString (AspellLanguage, Str);

  if (wcscmp (Str, L"<MULTIPLE>") == 0)
  {
    SetMultipleLanguages (AspellMultiLanguages, AspellSpeller);
    AspellSpeller->SetMode (1);
  }
  else
  {
    wchar_t *TBuf = 0;
    SetString (TBuf, Str);
    AspellSpeller->SetLanguage (TBuf);
    CLEAN_AND_ZERO_ARR (TBuf);
    CurrentSpeller->SetMode (0);
  }
}

void SpellChecker::SetHunspellLanguage (const wchar_t *Str)
{
  SetString (HunspellLanguage, Str);

  if (wcscmp (Str, L"<MULTIPLE>") == 0)
  {
    SetMultipleLanguages (HunspellMultiLanguages, HunspellSpeller);
    HunspellSpeller->SetMode (1);
  }
  else
  {
    HunspellSpeller->SetLanguage (HunspellLanguage);
    HunspellSpeller->SetMode (0);
  }
}

const char *SpellChecker::GetDelimiters ()
{
  return DelimUtf8;
}

void SpellChecker::SetSuggestionsNum (int Num)
{
  SuggestionsNum = Num;
}

// Here parameter is in UTF-8
void SpellChecker::SetDelimiters (const char *Str)
{
  wchar_t *DestBuf = 0;
  wchar_t *SrcBuf = 0;
  SetString (DelimUtf8, Str);
  SetStringSUtf8 (SrcBuf, DelimUtf8);
  SetParsedString (DestBuf, SrcBuf);
  auto TargetBufLength = wcslen (DestBuf) + 5 + 1;
  wchar_t *TargetBuf = new wchar_t [wcslen (DestBuf) + 5 + 1];
  wcscpy (TargetBuf, DestBuf);
  wcscat_s (TargetBuf, TargetBufLength, L" \n\r\t\v");
  SetStringDUtf8 (DelimUtf8Converted, TargetBuf);
  SetStringSUtf8 (DelimConverted, DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DestBuf);
  CLEAN_AND_ZERO_ARR (SrcBuf);
  CLEAN_AND_ZERO_ARR (TargetBuf);
}

void SpellChecker::SetMultipleLanguages (const wchar_t *MultiString, AbstractSpellerInterface *Speller)
{
  wchar_t *Context = 0;
  std::vector<wchar_t *> *MultiLangList = new std::vector<wchar_t *>;
  wchar_t *MultiStringCopy = 0;
  wchar_t *Token = 0;
  wchar_t *TBuf = 0;

  SetString (MultiStringCopy, MultiString);
  Token = wcstok_s (MultiStringCopy, L"|", &Context);
  while (Token)
  {
    TBuf = 0;
    SetString (TBuf, Token);
    MultiLangList->push_back (TBuf);
    Token = wcstok_s (NULL, L"|", &Context);
  }

  Speller->SetMultipleLanguages (MultiLangList);
  CLEAN_AND_ZERO_ARR (MultiStringCopy);
  CLEAN_AND_ZERO_STRING_VECTOR (MultiLangList);
}

BOOL SpellChecker::HunspellReinitSettings (BOOL ResetDirectory)
{
  if (ResetDirectory)
  {
    HunspellSpeller->SetDirectory (HunspellPath);
    HunspellSpeller->SetAdditionalDirectory (AdditionalHunspellPath);
  }
  char *MultiLanguagesCopy = 0;
  if (wcscmp (HunspellLanguage, L"<MULTIPLE>") != 0)
    HunspellSpeller->SetLanguage (HunspellLanguage);
  else
    SetMultipleLanguages (HunspellMultiLanguages, HunspellSpeller);

  CLEAN_AND_ZERO_ARR (MultiLanguagesCopy);
  return TRUE;
}

BOOL SpellChecker::AspellReinitSettings ()
{
  AspellSpeller->Init (AspellPath);

  if (wcscmp (AspellLanguage, L"<MULTIPLE>") != 0)
  {
    AspellSpeller->SetLanguage (AspellLanguage);
  }
  else
    SetMultipleLanguages (AspellMultiLanguages, AspellSpeller);
  return TRUE;
}

void SpellChecker::SetBufferSize(int Size)
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
    SetLayeredWindowAttributes(SuggestionsInstance->getHSelf (), 0, static_cast<BYTE> ((255 * SBTrans) / 100), LWA_ALPHA);
  }
}

void SpellChecker::ApplyConversions (char *Word) // In Utf-8, Maybe shortened during conversion
{
  const char *ConvertFrom [3];
  const char *ConvertTo [3];
  int Apply[3] = {IgnoreYo, IgnoreYo, ConvertSingleQuotes};

  if (CurrentEncoding == ENCODING_ANSI)
  {
    ConvertFrom[0] = YoANSI;
    ConvertFrom[1] = yoANSI;
    ConvertFrom[2] = PunctuationApostropheANSI;
    ConvertTo[0] = YeANSI;
    ConvertTo[1] = yeANSI;
    ConvertTo[2] = "\'";
  }
  else
  {
    ConvertFrom[0] = Yo;
    ConvertFrom[1] = yo;
    ConvertFrom[2] = PunctuationApostrophe;
    ConvertTo[0] = Ye;
    ConvertTo[1] = ye;
    ConvertTo[2] = "\'";
  }

  // FOR NOW It works only if destination string is shorter than source string.

  for (int i = 0; i < countof (ConvertFrom); i++)
  {
    if (!Apply[i] || ConvertFrom [i] == 0 || ConvertTo[i] == 0 || *ConvertFrom [i] == 0 || *ConvertTo [i] == 0)
      continue;

    char *Iter = Word;
    char *NestedIter = 0;
    auto Diff = strlen (ConvertFrom[i]) - strlen (ConvertTo[i]);
    if (Diff < 0)
      continue; // For now this case isn't needed.
    while (true)
    {
      Iter = strstr (Iter, ConvertFrom[i]);
      if (!Iter)
        break;

      for (size_t j = 0; j < strlen (ConvertTo[i]); j++)
      {
        *Iter = ConvertTo[i][j];
        Iter++;
      }
      NestedIter = Iter;
      while (*(NestedIter + Diff))
      {
        *NestedIter =  *(NestedIter + Diff);
        NestedIter++;
      }
      for (int j = 0; j < Diff; j++)
        *(NestedIter + j) = 0;
    }
  }
}

void SpellChecker::ResetHotSpotCache ()
{
  memset (HotSpotCache, -1, sizeof (HotSpotCache));
}

BOOL SpellChecker::CheckWord (char *Word, long Start, long /*End*/)
{
  BOOL res = FALSE;
  if (!CurrentSpeller->IsWorking () || !Word || !*Word)
    return TRUE;
  // Well Numbers have same codes for ANSI and Unicode I guess, so
  // If word contains number then it's probably just a number or some crazy name
  auto Style = GetStyle (Start);
  if (CheckComments && !CheckWordInCommentOrString (Style))
    return TRUE;

  if (HotSpotCache[Style] == -1)
    {
      BOOL ok;
      HotSpotCache[Style] = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_STYLEGETHOTSPOT, Style);
      if (!ok)
        return TRUE;
    }

  if (HotSpotCache[Style] == 1)
    return TRUE;

  ApplyConversions (Word);

  wchar_t *Ts = 0;
  auto SymbolsNum = (CurrentEncoding == ENCODING_UTF8) ? Utf8Length (Word) : strlen (Word);
  if (SymbolsNum == 0)
  {
    res = TRUE;
    goto CleanUp;
  }

  if (IgnoreOneLetter && SymbolsNum == 1)
  {
    res = TRUE;
    goto CleanUp;
  }

  if (IgnoreNumbers && (CurrentEncoding == ENCODING_UTF8 ? Utf8pbrk (Word, "0123456789") : strpbrk (Word, "0123456789")) != 0) // Same for UTF-8 and not
  {
    res = TRUE;
    goto CleanUp;
  }

  if (IgnoreCStart || IgnoreCHave || IgnoreCAll)
  {
    if (CurrentEncoding == ENCODING_UTF8)
      SetStringSUtf8 (Ts, Word);
    else
      SetString (Ts, Word);
    if (IgnoreCStart && IsCharUpper (Ts[0]))
    {
      res = TRUE;
      goto CleanUp;
    }
    if (IgnoreCHave || IgnoreCAll)
    {
      BOOL AllUpper = IsCharUpper (Ts[0]);
      for (unsigned int i = 1; i < wcslen (Ts); i++)
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

  if (Ignore_ && strchr (Word, '_') != 0) // I guess the same for UTF-8 and ANSI
  {
    res = TRUE;
    goto CleanUp;
  }

  auto Len = strlen (Word);

  if (IgnoreSEApostrophe)
  {
    if (Word[0] == '\'' || Word[Len - 1] == '\'')
    {
      res = TRUE;
      goto CleanUp;
    }
  }

  res = CurrentSpeller->CheckWord (Word);

CleanUp:
  CLEAN_AND_ZERO_ARR (Ts);

  return res;
}

void SpellChecker::CheckSpecialDelimeters (char *&Word, const char *TextStart, long &WordStart, long &WordEnd)
{
  if (RemoveBoundaryApostrophes)
  {
    while (*Word == '\'' && *Word != '\0')
    {
      *Word = '\0';
      Word++;
      WordStart++;
    }

    char *it = Word + strlen (Word) - 1;
    while (*it == '\'' && *it != '\0' && it > TextStart)
    {
      *it = '\0';
      WordEnd--;
      it--;
    }
  }
}

int SpellChecker::CheckTextDefaultAnswer (CheckTextMode Mode)
{
  switch (Mode)
    {
    case SpellChecker::UNDERLINE_ERRORS:
    case SpellChecker::FIND_FIRST:
    case SpellChecker::FIND_LAST:
      return FALSE;
    case SpellChecker::GET_FIRST:
      return -1;
    }
  return -1;
}

int SpellChecker::CheckText (char *TextToCheck, long Offset, CheckTextMode Mode)
{
  if (!TextToCheck || !*TextToCheck)
    {
      return CheckTextDefaultAnswer (Mode);
    }

  HWND ScintillaWindow = GetCurrentScintilla ();
  BOOL ok;
  SendMsgToActiveEditor (&ok, ScintillaWindow, SCI_GETINDICATORCURRENT);
  if (!ok)
    return CheckTextDefaultAnswer (Mode);
  char *Context = 0; // Temporary variable for strtok_s usage
  char *token;
  BOOL stop = FALSE;
  long ResultingWordEnd = -1, ResultingWordStart = -1;
  auto TextLen = strlen (TextToCheck);
  std::vector<long> UnderlineBuffer;
  long WordStart = 0;
  long WordEnd = 0;

  if (!DelimUtf8)
    return CheckTextDefaultAnswer (Mode);

  if (CurrentEncoding == ENCODING_UTF8)
    token = Utf8strtok (TextToCheck, DelimUtf8Converted, &Context);
  else
    token = strtok_s (TextToCheck, DelimConverted, &Context);

  while (token)
    {
      if (token)
        {
          WordStart = Offset + static_cast<long> (token - TextToCheck);
          WordEnd = Offset + static_cast<long> (token - TextToCheck + strlen (token));
          CheckSpecialDelimeters (token, TextToCheck, WordStart, WordEnd);
          if (WordEnd < WordStart)
            goto newtoken;


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
                      SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_SETSEL, WordStart, WordEnd);
                      if (!ok)
                        return FALSE;
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
                case GET_FIRST:
                  return WordStart;
                  break;
                }
              if (stop)
                break;
            }
          else
            {
            }
        }

newtoken:
      if (CurrentEncoding == ENCODING_UTF8)
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
      RemoveUnderline (ScintillaWindow, PrevPos, Offset + static_cast<long> (TextLen) - 1);
    }

  // PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT, oldid);

  switch (Mode)
    {
    case UNDERLINE_ERRORS:
      return TRUE;
    case FIND_FIRST:
      return stop;
    case GET_FIRST:
      return -1;
      break;
    case FIND_LAST:
      if (ResultingWordStart == -1)
        return FALSE;
      else
        {
          SendMsgToActiveEditor (0, GetCurrentScintilla (), SCI_SETSEL, ResultingWordStart, ResultingWordEnd);
          return TRUE;
        }
    };
  return FALSE;
}

void SpellChecker::ClearVisibleUnderlines ()
{
  BOOL ok;
  auto length = SendMsgToActiveEditor(&ok, GetCurrentScintilla (), SCI_GETLENGTH);
  if (!ok) return;
  if (length > 0)
  {
    PostMsgToActiveEditor (GetCurrentScintilla (), SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
    PostMsgToActiveEditor (GetCurrentScintilla (), SCI_INDICATORCLEARRANGE, 0, length);
  }
}

void SpellChecker::CheckVisible (BOOL NotIntersectionOnly)
{
  CLEAN_AND_ZERO_ARR (VisibleText);
  VisibleText = GetVisibleText (&VisibleTextOffset, NotIntersectionOnly);
  if (!VisibleText)
    return;
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
    CurrentEncoding = ENCODING_UTF8;
    // SetEncoding ("utf-8");
    break;
  default:
    {
      CurrentEncoding = ENCODING_ANSI;
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
  HunspellSpeller->SetEncoding (CurrentEncoding);
  AspellSpeller->SetEncoding (CurrentEncoding);
}

void SpellChecker::SwitchAutoCheck ()
{
  AutoCheckText = !AutoCheckText;
  UpdateAutocheckStatus ();
  RecheckVisibleBothViews ();
}

void SpellChecker::RecheckModified ()
{
  if (!CurrentSpeller->IsWorking ())
    {
      ClearAllUnderlines ();
      return;
    }

  BOOL ok;
  auto FirstModifiedLine = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, ModifiedStart);
  if (!ok) return;
  auto LastModifiedLine = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, ModifiedEnd);
  if (!ok) return;
  auto LineCount = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLINECOUNT);
  if (!ok) return;
  auto FirstPossiblyModifiedPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, FirstModifiedLine);
  if (!ok) return;
  LRESULT LastPossiblyModifiedPos = 0;
  if (LastModifiedLine + 1 < LineCount)
    {
      LastPossiblyModifiedPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, LastModifiedLine + 1);
      if (!ok) return;
    }
  else
    {
      LastPossiblyModifiedPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLENGTH);
      if (!ok) return;
    }

  Sci_TextRange Range;
  Range.chrg.cpMin = static_cast<long> (FirstPossiblyModifiedPos);
  Range.chrg.cpMax = static_cast<long> (LastPossiblyModifiedPos);
  Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
  SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTRANGE, 0, (LPARAM) &Range);

  CheckText (Range.lpstrText, static_cast<long> (FirstPossiblyModifiedPos), UNDERLINE_ERRORS);
  CLEAN_AND_ZERO_ARR (Range.lpstrText);
}

void SpellChecker::SetConversionOptions (BOOL ConvertYo, BOOL ConvertSingleQuotesArg, BOOL RemoveBoundaryApostrophesArg)
{
  IgnoreYo = ConvertYo;
  ConvertSingleQuotes = ConvertSingleQuotesArg;
  RemoveBoundaryApostrophes = RemoveBoundaryApostrophesArg;
}

void SpellChecker::RecheckVisible (BOOL NotIntersectionOnly)
{
  int CodepageId = 0;
  if (!CurrentSpeller->IsWorking ())
  {
    ClearAllUnderlines ();
    return;
  }
  BOOL ok;
  CodepageId = (int) SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETCODEPAGE, 0, 0);
  if (!ok)
    return;
  setEncodingById (CodepageId); // For now it just changes should we convert it to utf-8 or no
  if (CheckTextNeeded ())
    CheckVisible (NotIntersectionOnly);
  else
    ClearAllUnderlines ();
}

void SpellChecker::ErrorMsgBox (const wchar_t *message)
{
  wchar_t buf [DEFAULT_BUF_SIZE];
  swprintf_s (buf, L"DSpellCheck Error: %ws", message);
  MessageBox (NppDataInstance->_nppHandle, message, L"Error Happened!", MB_OK | MB_ICONSTOP);
}

void SpellChecker::CopyMisspellingsToClipboard ()
{
  BOOL ok;
  auto lengthDoc = (SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLENGTH) + 1);
  if (!ok)
    return;
  char *buf = new char [lengthDoc];
  SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXT, lengthDoc, (LPARAM)buf);
  if (!ok)
    return;
  int res = 0;
  std::string str; // Yay for first use of std::stirng
  do
  {
     res = CheckText (buf + res, res, GET_FIRST);
     if (res != -1)
     {
       str += std::string (buf + res);
       str += "\n";
     }
     else
       break;

     while (*(buf + res) != 0)
       res++;

     while (*(buf + res) == 0)
       res++;

     if (res >= lengthDoc)
       break;
  } while (true);

  wchar_t *wchar_str = 0;

  switch (CurrentEncoding)
  {
  case ENCODING_UTF8:
    SetStringSUtf8 (wchar_str, str.c_str ());
    break;
  case ENCODING_ANSI:
    SetString (wchar_str, str.c_str ());
    break;
  }

  const size_t len = (wcslen (wchar_str) + 1) * 2;
  HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
  memcpy(GlobalLock(hMem), wchar_str, len);
  GlobalUnlock(hMem);
  OpenClipboard(0);
  EmptyClipboard();
  SetClipboardData(CF_UNICODETEXT, hMem);
  CloseClipboard();
  CLEAN_AND_ZERO_ARR (buf);
  CLEAN_AND_ZERO_ARR (wchar_str);
}

SuggestionsMenuItem::SuggestionsMenuItem (wchar_t *TextArg, BYTE IdArg, BOOL SeparatorArg /*= FALSE*/)
{
  Text = 0;
  SetString (Text, TextArg);
  Id = IdArg;
  Separator = SeparatorArg;
}
