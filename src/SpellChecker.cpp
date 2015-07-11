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

#include "SpellerController.h"
#include "AspellController.h"
#include "HunspellController.h"

#include "Aspell.h"
#include "DownloadDicsDlg.h"
#include "iconv.h"
#include "CommonFunctions.h"
#include "LanguageName.h"
#include "LangListDialog.h"
#include "MainDef.h"
#include "PluginInterface.h"
#include "Plugin.h"
#include "RemoveDicsDialog.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "SciLexer.h"
#include "Scintilla.h"
#include "SelectProxy.h"
#include "Suggestions.h"
#include "Settings.h"

HWND GetScintillaWindow(const NppData *NppDataArg)
{
  int which = -1;
  if (isCurrentlyTerminating ())
    return 0;
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

SpellChecker::SpellChecker(const TCHAR *IniFilePathArg,
                           SettingsDlg *SettingsDlgInstanceArg,
                           NppData *NppDataInstanceArg,
                           Suggestions *SuggestionsInstanceArg,
                           LangListDialog *LangListInstanceArg)
    : m_settings{
          std::make_unique<Settings>([this](const SettingsData &settingsData) {
            onSettingsChanged(settingsData);
          })} {
  CurrentPosition = 0;
  DelimUtf8Converted = 0;
  IniFilePath = 0;
  VisibleText = 0;
  DelimConverted = 0;
  VisibleTextLength = -1;
  setString (IniFilePath, IniFilePathArg);
  settingsDlgInstance = SettingsDlgInstanceArg;
  SuggestionsInstance = SuggestionsInstanceArg;
  NppDataInstance = NppDataInstanceArg;
  LangListInstance = LangListInstanceArg;
  AutoCheckText = 0;
  MultiLangMode = 0;
  CurWordList = 0;
  SelectedWord = 0;
  WUCLength = 0;
  WUCPosition = 0;
  WUCisRight = TRUE;
  CurrentScintilla = GetScintillaWindow (NppDataInstance);
  SuggestionMenuItems = 0;
  m_spellers[SpellerType::aspell] = std::make_unique<AspellController> (NppDataInstance->_nppHandle);
  m_spellers[SpellerType::hunspell] = std::make_unique<HunspellController> (NppDataInstance->_nppHandle);
  LastSuggestions = 0;
  PrepareStringForConversion ();
  memset (ServerNames, 0, sizeof (ServerNames));
  memset (DefaultServers, 0, sizeof (DefaultServers));
  AddressIsSet = 0;
  setString (DefaultServers[0], _T ("ftp://ftp.snt.utwente.nl/pub/software/openoffice/contrib/dictionaries/"));
  setString (DefaultServers[1], _T ("ftp://sunsite.informatik.rwth-aachen.de/pub/mirror/OpenOffice/contrib/dictionaries/"));
  setString (DefaultServers[2], _T ("ftp://gd.tuwien.ac.at/office/openoffice/contrib/dictionaries/"));
  ResetHotSpotCache ();
  ProxyUserName = 0;
  ProxyHostName = 0;
  ProxyPassword = 0;
  ProxyAnonymous = TRUE;
  ProxyType = 0;
  ProxyPort = 0;
  SettingsLoaded = FALSE;
  UseProxy = FALSE;
  SettingsToSave = new std::map<TCHAR *, DWORD, bool ( *)(TCHAR *, TCHAR *)> (SortCompare);
  BOOL ok;
  BOOL res = SendMsgToNpp (&ok, NppDataInstance, NPPM_ALLOCATESUPPORTED, 0, 0);
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
    setString (Buf, InString[i]);
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
  CLEAN_AND_ZERO_ARR (SelectedWord);
  CLEAN_AND_ZERO_ARR (DelimConverted);
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);

  CLEAN_AND_ZERO_ARR (IniFilePath);
  CLEAN_AND_ZERO_ARR (VisibleText);


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

  std::map<TCHAR *, DWORD, bool ( *)(TCHAR *, TCHAR *)>::iterator it = SettingsToSave->begin ();
  for (; it != SettingsToSave->end (); ++it)
  {
    delete ((*it).first);
  }
  CLEAN_AND_ZERO (SettingsToSave);
}

void InsertSuggMenuItem (HMENU Menu, TCHAR *Text, BYTE Id, int InsertPos, BOOL Separator)
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
    mi.cch = _tcslen (Text) + 1;
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
      if (CheckTextNeeded () && settings ().suggestionControlType == SuggestionControlType::menu)
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

      PostMessage (NppDataInstance->_nppHandle, getCustomGUIMessageId (CustomGUIMessage::SHOW_CALCULATED_MENU), 0, (LPARAM)SuggestionMenuItems);
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
      TCHAR *Name = (TCHAR *) wParam;
      TCHAR *TrimmedName = 0;
      setString (TrimmedName, Name);
      FtpTrim (TrimmedName);
      TCHAR *Buf = 0;
      for (int i = 0; i < countof (DefaultServers); i++)
      {
        setString (Buf, DefaultServers[i]);
        FtpTrim (Buf);
        if (_tcscmp (Buf, TrimmedName) == 0)
          goto add_user_server_cleanup; // Nothing is done in this case
      }
      for (int i = 0; i < countof (ServerNames); i++)
      {
        setString (Buf, ServerNames[i]);
        FtpTrim (Buf);
        if (_tcscmp (Buf, TrimmedName) == 0)
          goto add_user_server_cleanup; // Nothing is done in this case
      }
      // Then we're adding finally
      CLEAN_AND_ZERO_ARR (ServerNames[countof (ServerNames) - 1]);
      for (int i = countof (ServerNames) - 1; i > 0; i--)
      {
        ServerNames[i] = ServerNames[i - 1];
      }
      ServerNames[0] = 0;
      setString (ServerNames[0], Name);
add_user_server_cleanup:
      CLEAN_AND_ZERO_ARR (Buf);
      CLEAN_AND_ZERO_ARR (Name);
      CLEAN_AND_ZERO_ARR (TrimmedName);
      ResetDownloadCombobox ();
      SaveSettings ();
    }
    break;
  case TM_SETTINGS_CHANGED:
    {
      std::unique_ptr<SettingsData> data{ reinterpret_cast<SettingsData *> (wParam) };
      setNewSettings (*data);
      SaveSettings ();
    }
    break;
  default:
    break;
  }
  return TRUE;
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

TCHAR *SpellChecker::GetLangByIndex (int i)
{
  return m_activeLangList->at(i).OrigName;
}

SpellerController *SpellChecker::activeSpeller ()
{
  return m_spellers[m_settings->get ().activeSpellerType].get ();
}

void SpellChecker::ReinitLanguageLists(BOOL UpdateDialogs)
{
  bool currentLanguageExists = false;

  SpellerController *spellerToUse = activeSpeller ();

  if (m_settings->get ().activeSpellerType == SpellerType::aspell)
  {
    GetDownloadDics ()->display (false);
    getRemoveDicsDialog ()->display (false);
  }

  m_activeLangList.reset ();

  if (spellerToUse->IsWorking ())
  {
    if (UpdateDialogs)
      settingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (FALSE);
    std::vector <TCHAR *> *LangsFromSpeller =  spellerToUse->getLanguageList ();
    m_activeLangList = std::make_shared<std::vector <LanguageName>> ();

    if (!LangsFromSpeller || LangsFromSpeller->size () == 0)
    {
      if (UpdateDialogs)
        settingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (TRUE);
      return;
    }
    for (unsigned int i = 0; i < LangsFromSpeller->size (); i++)
    {
      LanguageName Lang(
          LangsFromSpeller->at(i),
          (settings ().activeSpellerType == SpellerType::hunspell &&
           settings ().useLanguageNameAliases),
          spellerToUse->GetLangOnlySystem(
              LangsFromSpeller->at(i)) != FALSE);  // Using them only for Hunspell
      m_activeLangList->push_back (Lang);                               // TODO: Add option - use or not use aliases.
      if (m_settings->get ().activeSpellerSettings ().activeLanguage == Lang.OrigName)
        currentLanguageExists = true;
    }
    if (m_settings->get ().activeSpellerSettings ().activeLanguage == L"<MULTIPLE>")
      currentLanguageExists = true;

    CLEAN_AND_ZERO_STRING_VECTOR (LangsFromSpeller);
    std::sort (m_activeLangList->begin (), m_activeLangList->end (), settings ().useLanguageNameAliases ? CompareAliases : CompareOriginal);
    if (!currentLanguageExists && m_activeLangList->size () > 0)
    {
      {
        auto settings = m_settings->change ();
        settings->activeSpellerSettings ().activeLanguage = m_activeLangList->at (0).OrigName;
      }
      RecheckVisibleBothViews ();
    }
    PostMessage(NppDataInstance->_nppHandle,
                getCustomGUIMessageId(CustomGUIMessage::CONFIGURATION_CHANGED),
                0, 0);
  }
  else
  {
    if (UpdateDialogs)
      settingsDlgInstance->GetSimpleDlg ()->DisableLanguageCombo (TRUE);
  }
}

void SpellChecker::RecheckVisibleBothViews ()
{
  int OldLexer = Lexer;
  EncodingType OldEncoding = currentEncoding;
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
  currentEncoding = OldEncoding;
  for (auto &speller : m_spellers)
    speller->setEncoding (currentEncoding);
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
  if (Event == EID_KILLTHREAD)
    return false;

  CurrentScintilla = GetScintillaWindow (NppDataInstance); // All operations should be done with current scintilla anyway
  if (!CurrentScintilla)
    return FALSE; // If scintilla is dead there's nothing else to do
  switch (Event)
    {
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
      settingsDlgInstance->display (false);
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
      if (settings ().suggestionControlType == SuggestionControlType::box)
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
      hideSuggestionBox ();
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
      getRemoveDicsDialog ()->RemoveSelected (this);
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
      getRemoveDicsDialog ()->UpdateOptions (this);
      SaveSettings ();
      break;

    case EID_UPDATE_REMOVE_DICS_OPTIONS:
      getRemoveDicsDialog ()->SetCheckBoxes (RemoveUserDics, RemoveSystem);
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
      DoPluginMenuInclusion ();
      ReinitLanguageLists(TRUE);
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

    case EID_HUNSPELL_DICTIONARIES_CHANGE:
      onHunspellDictionariesChange ();

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
  HMENU NewMenu = CreatePopupMenu ();
  if (!Invalidate)
  {
    if (m_activeLangList)
    {
      if (m_activeLangList->size () > 0)
      {
        for (unsigned int i = 0; i < m_activeLangList->size (); i++)
        {
          int Checked = (_tcscmp (currentLanguage (), m_activeLangList->at(i).OrigName) == 0) ? (MFT_RADIOCHECK | MF_CHECKED) : MF_UNCHECKED;
          Res = AppendMenu (NewMenu, MF_STRING | Checked, GetUseAllocatedIds () ? i + GetLangsMenuIdStart () : MAKEWORD (i, LANGUAGE_MENU_ID), settings ().useLanguageNameAliases ? m_activeLangList->at(i).AliasName : m_activeLangList->at(i).OrigName);
          if (!Res)
            return;
        }
        int Checked = (_tcscmp (currentLanguage (), _T ("<MULTIPLE>")) == 0) ? (MFT_RADIOCHECK | MF_CHECKED)  : MF_UNCHECKED;
        Res = AppendMenu (NewMenu, MF_STRING | Checked, GetUseAllocatedIds () ? MULTIPLE_LANGS + GetLangsMenuIdStart () : MAKEWORD (MULTIPLE_LANGS, LANGUAGE_MENU_ID), _T ("Multiple Languages"));
        Res = AppendMenu (NewMenu, MF_SEPARATOR, 0, 0);
        Res = AppendMenu (NewMenu, MF_STRING, GetUseAllocatedIds () ? CUSTOMIZE_MULTIPLE_DICS + GetLangsMenuIdStart () : MAKEWORD (CUSTOMIZE_MULTIPLE_DICS, LANGUAGE_MENU_ID), _T ("Set Multiple Languages..."));
        if (m_settings->get ().activeSpellerType == SpellerType::hunspell) // Only Hunspell supported
        {
          Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? DOWNLOAD_DICS + GetLangsMenuIdStart () : MAKEWORD (DOWNLOAD_DICS, LANGUAGE_MENU_ID), _T ("Download More Languages..."));
          Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? REMOVE_DICS + GetLangsMenuIdStart () : MAKEWORD (REMOVE_DICS, LANGUAGE_MENU_ID), _T ("Remove Unneeded Languages..."));
        }
      }
      else if (m_settings->get ().activeSpellerType == SpellerType::hunspell)
        Res = AppendMenu (NewMenu, MF_STRING , GetUseAllocatedIds () ? DOWNLOAD_DICS + GetLangsMenuIdStart () : MAKEWORD (DOWNLOAD_DICS, LANGUAGE_MENU_ID), _T ("Download Languages..."));
    }
  }
  else
    Res = AppendMenu (NewMenu, MF_STRING | MF_DISABLED, 0, _T ("Loading..."));

  Mif.fMask = MIIM_SUBMENU | MIIM_STATE;
  Mif.cbSize = sizeof (MENUITEMINFO);
  Mif.hSubMenu = (m_activeLangList ? NewMenu : 0);
  Mif.fState = (!m_activeLangList ? MFS_GRAYED : MFS_ENABLED);

  SetMenuItemInfo (DSpellCheckMenu, QUICK_LANG_CHANGE_ITEM, TRUE, &Mif);
}

void SpellChecker::FillDownloadDics ()
{
  GetDownloadDics ()->SetOptions (ShowOnlyKnown, InstallSystem);
}

void SpellChecker::ResetDownloadCombobox ()
{
  HWND TargetCombobox = GetDlgItem (GetDownloadDics ()->getHSelf (), IDC_ADDRESS);
  TCHAR Buf[DEFAULT_BUF_SIZE];
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
  TCHAR CurText [DEFAULT_BUF_SIZE];
  TCHAR *Buf = 0;
  ComboBox_GetText (TargetCombobox, CurText, DEFAULT_BUF_SIZE);
  FtpTrim (CurText);
  for (int i = 0; i < countof (ServerNames); i++)
  {
    setString (Buf, DefaultServers[i]);
    FtpTrim (Buf);
    if (_tcscmp (Buf, CurText) == 0)
    {
      LastUsedAddress = i;
      goto cleanup;
    }
  };
  for (int i = 0; i < countof (ServerNames); i++)
  {
    setString (Buf, ServerNames[i]);
    FtpTrim (Buf);
    if (_tcscmp (Buf, CurText) == 0)
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
  std::pair<TCHAR *, DWORD> *x = (std::pair<TCHAR *, DWORD> *) lParam;
  if (SettingsToSave->find (x->first) == SettingsToSave->end ())
    (*SettingsToSave)[x->first] = x->second;
  else
  {
    CLEAN_AND_ZERO_ARR (x->first);
  }
  CLEAN_AND_ZERO (x);
}

void SpellChecker::CheckFileName ()
{
  TCHAR *Context = 0;
  TCHAR *Token = 0;
  TCHAR *FileTypesCopy = 0;
  TCHAR FullPath[MAX_PATH];
  setString (FileTypesCopy, settings ().fileMask.c_str ());
  Token = _tcstok_s (FileTypesCopy, _T(";"), &Context);
  switch (settings ().fileMaskOption)
    {
      case FileMaskOption::inclusion:
      checkTextEnabled = false;
      break;
      case FileMaskOption::exclusion:
      checkTextEnabled = true;
      break;
    }
  BOOL ok;
  SendMsgToNpp (&ok, NppDataInstance, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM) FullPath);
  if (!ok)
    goto cleanup;

  bool stop = false;
  while (Token)
  {
    switch (settings ().fileMaskOption)
    {
    case FileMaskOption::inclusion:
      checkTextEnabled = checkTextEnabled || PathMatchSpec (FullPath, Token);
      if (checkTextEnabled)
        stop = true;
      break;
    case FileMaskOption::exclusion:
      checkTextEnabled &= checkTextEnabled && (!PathMatchSpec (FullPath, Token));
      if (!checkTextEnabled)
        stop = true;
      break;
    }
    if (stop)
      break;

    Token = _tcstok_s (NULL, _T(";"), &Context);
  }
  Lexer = SendMsgToActiveEditor (0, GetCurrentScintilla (), SCI_GETLEXER);

cleanup:
  CLEAN_AND_ZERO_ARR (FileTypesCopy);
}

int SpellChecker::GetStyle (int Pos)
{
  return SendMsgToActiveEditor (0, GetCurrentScintilla (), SCI_GETSTYLEAT, Pos);
}
// Actually for all languages which operate mostly in strings it's better to check only comments
// TODO: Fix it
int SpellChecker::CheckWordInCommentOrString (int Style)
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
  return checkTextEnabled && AutoCheckText;
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

void SpellChecker::hideSuggestionBox ()
{
  SuggestionsInstance->display (false);
}
void SpellChecker::FindNextMistake ()
{
  BOOL ok;
  CurrentPosition = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETCURRENTPOS);
  if (!ok) return;
  int CurLine = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, CurrentPosition);
  if (!ok) return;
  int LineStartPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, CurLine);
  if (!ok) return;
  long DocLength = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLENGTH);
  if (!ok) return;
  int IteratorPos = LineStartPos;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
  {
    Range.chrg.cpMin = IteratorPos;
    Range.chrg.cpMax = IteratorPos + settings ().bufferSizeForSearch * 1024;
    int IgnoreOffsetting = 0;
    if (Range.chrg.cpMax > DocLength)
    {
      IgnoreOffsetting = 1;
      Range.chrg.cpMax = DocLength;
    }
    Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
    SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTRANGE, 0, (LPARAM) &Range);
    if (!ok) return;
    char *IteratingStart = Range.lpstrText + Range.chrg.cpMax - Range.chrg.cpMin - 1;
    char *IteratingChar = IteratingStart;
    if (!IgnoreOffsetting)
    {
      if (currentEncoding == ENCODING_UTF8)
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
    Result = CheckText (Range.lpstrText, IteratorPos, FIND_FIRST);
    CLEAN_AND_ZERO_ARR (Range.lpstrText);
    if (Result)
      break;

    IteratorPos += (settings ().bufferSizeForSearch * 1024 + IteratingChar - IteratingStart);

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
  int CurLine = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, CurrentPosition);
  if (!ok) return;
  long LineEndPos = 0;
  long DocLength = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLENGTH);
  if (!ok) return;
  LineEndPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLINEENDPOSITION, CurLine);
  if (!ok) return;

  int IteratorPos = LineEndPos;
  Sci_TextRange Range;
  BOOL Result = FALSE;
  BOOL FullCheck = FALSE;

  while (1)
    {
      Range.chrg.cpMin = IteratorPos - settings ().bufferSizeForSearch * 1024;
      Range.chrg.cpMax = IteratorPos;
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
          if (currentEncoding == ENCODING_UTF8)
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
      int offset = IteratingChar - IteratingStart;
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
      Result = CheckText (Range.lpstrText + offset, Range.chrg.cpMin + offset, FIND_LAST);
      CLEAN_AND_ZERO_ARR (Range.lpstrText);
      if (Result)
        break;

      IteratorPos -= (settings ().bufferSizeForSearch * 1024 - offset);

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
  settingsDlgInstance->GetAdvancedDlg ()->SetDelimetersEdit (DEFAULT_DELIMITERS);
}

HWND SpellChecker::GetCurrentScintilla ()
{
  return CurrentScintilla;
}

BOOL SpellChecker::GetWordUnderCursorIsRight (long &Pos, long &Length, BOOL UseTextCursor)
{
  BOOL Ret = TRUE;
  POINT p;
  int initCharPos = -1;
  int SelectionStart = 0;
  int SelectionEnd = 0;

  if (!UseTextCursor)
  {
    if (GetCursorPos(&p) == 0)
      return TRUE;

    ScreenToClient(GetScintillaWindow (NppDataInstance), &p);

    BOOL ok;
    initCharPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);
    if (!ok) return TRUE;
  }
  else
  {
    BOOL ok;
    SelectionStart = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETSELECTIONSTART);
    if (!ok) return TRUE;
    SelectionEnd = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETSELECTIONEND);
    if (!ok) return TRUE;
    initCharPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETCURRENTPOS);
    if (!ok) return TRUE;
  }

  if (initCharPos != -1)
  {
    BOOL ok;
    int Line = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, initCharPos);
    if (!ok) return TRUE;
    long LineLength = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINELENGTH, Line);
    if (!ok) return TRUE;
    char *Buf = new char[LineLength + 1];
    SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLINE, Line, (LPARAM) Buf);
    if (!ok) return TRUE;
    Buf [LineLength] = 0;
    long Offset = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, Line);
    if (!ok) return TRUE;
    char *Word = GetWordAt (initCharPos, Buf, Offset);
    if (!Word || !*Word)
    {
      Ret = TRUE;
    }
    else
    {
      Pos = Word - Buf + Offset;
      long PosEnd = Pos + strlen (Word);
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
  if (!DelimUtf8Converted)
    return 0;

  char *Iterator = Text + CharPos - Offset;

  if (currentEncoding == ENCODING_UTF8)
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

  if (currentEncoding == ENCODING_UTF8)
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
  if (currentEncoding == ENCODING_UTF8)
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
  SetLayeredWindowAttributes(SuggestionsInstance->getHSelf (), 0, static_cast<BYTE> ((255 * settings().suggestionBoxTransparency) / 100), LWA_ALPHA);
  SuggestionsInstance->display (true);
  SuggestionsInstance->display (false);
}

void SpellChecker::InitSuggestionsBox ()
{
  if (!activeSpeller ()->IsWorking ())
    return;
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

  long Pos, Length;
  if (GetWordUnderCursorIsRight (Pos, Length))
  {
    return;
  }
  WUCLength = Length;
  WUCPosition = Pos;
  BOOL ok;
  int Line = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, WUCPosition);
  if (!ok) return;
  int TextHeight = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_TEXTHEIGHT, Line);
  if (!ok) return;
  int XPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POINTXFROMPOSITION, 0, WUCPosition);
  if (!ok) return;
  int YPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POINTYFROMPOSITION, 0, WUCPosition);
  if (!ok) return;

  p.x = XPos;
  p.y = YPos;
  RECT R;
  GetWindowRect (GetCurrentScintilla (), &R);
  ClientToScreen (GetScintillaWindow (NppDataInstance), &p);
  if (R.top > p.y + TextHeight - 3 || R.left > p.x || R.bottom < p.y + TextHeight - 3 + settings ().suggestionBoxSize ||  R.right < p.x + settings ().suggestionBoxSize)
    return;
  MoveWindow (SuggestionsInstance->getHSelf (), p.x, p.y + TextHeight - 3, settings ().suggestionBoxSize, settings ().suggestionBoxSize, TRUE);
  SuggestionsInstance->display ();
}

void SpellChecker::ProcessMenuResult (UINT MenuId)
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
      int Result = 0;
      if (!GetUseAllocatedIds ())
        Result = LOBYTE (MenuId);
      else
        Result = MenuId - GetContextMenuIdStart ();

      if (Result != 0)
      {
        if (Result == MID_IGNOREALL)
        {
          ApplyConversions (SelectedWord);
          activeSpeller()->IgnoreAll (SelectedWord);
          WUCLength = strlen (SelectedWord);
          BOOL ok;
          SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_SETSEL, WUCPosition + WUCLength , WUCPosition + WUCLength );
          if (!ok) return;
          RecheckVisibleBothViews ();
        }
        else if (Result == MID_ADDTODICTIONARY)
        {
          ApplyConversions (SelectedWord);
          activeSpeller()->AddToDictionary (SelectedWord);
          WUCLength = strlen (SelectedWord);
          BOOL ok;
          SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_SETSEL, WUCPosition + WUCLength , WUCPosition + WUCLength );
          if (!ok) return;
          RecheckVisibleBothViews ();
        }
        else if ((unsigned int)Result <= LastSuggestions->size ())
        {
          if (currentEncoding == ENCODING_ANSI)
            SetStringSUtf8 (AnsiBuf, LastSuggestions->at (Result - 1));
          else
            setString (AnsiBuf, LastSuggestions->at (Result - 1));

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
      int Result = 0;
      if (!GetUseAllocatedIds ())
        Result = LOBYTE (MenuId);
      else
        Result = MenuId - GetLangsMenuIdStart ();

      TCHAR *langString = 0;
      if (Result == MULTIPLE_LANGS)
      {
        langString = _T ("<MULTIPLE>");
      }
      else if (Result == CUSTOMIZE_MULTIPLE_DICS ||
               Result == DOWNLOAD_DICS ||
               Result == REMOVE_DICS)
      {
        // All actions are done in GUI thread in that case
        return;
      }
      else
        langString = m_activeLangList->at (Result).OrigName;
      DoPluginMenuInclusion (TRUE);

      {
        auto settings = m_settings->change ();
        settings->activeSpellerSettings().activeLanguage = langString;
      }


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
  if (!activeSpeller()->IsWorking ())
    return; // Word is already off-screen

  int Pos = WUCPosition;
  Sci_TextRange Range;
  TCHAR *Buf = 0;
  Range.chrg.cpMin = WUCPosition;
  Range.chrg.cpMax = WUCPosition + WUCLength;
  Range.lpstrText = new char [WUCLength + 1];
  PostMsgToActiveEditor (GetCurrentScintilla (), SCI_SETSEL, Pos, Pos + WUCLength);
  if (settings ().suggestionControlType == SuggestionControlType::box)
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

  setString (SelectedWord, Range.lpstrText);
  ApplyConversions (SelectedWord);

  CLEAN_AND_ZERO_STRING_VECTOR (LastSuggestions);
  LastSuggestions = activeSpeller()->GetSuggestions (SelectedWord);
  if (!LastSuggestions)
    return;

  for (int i = 0; i < static_cast<int> (LastSuggestions->size ()); i++)
    {
      if (i >= settings ().suggestionCount)
        break;
      SetStringSUtf8 (Buf, LastSuggestions->at (i));
      switch (settings ().suggestionControlType)
      {
      case SuggestionControlType::box:
        InsertSuggMenuItem (Menu, Buf, static_cast<BYTE> (i + 1), -1);
        break;
      case SuggestionControlType::menu:
        SuggestionMenuItems->push_back (new SuggestionsMenuItem (Buf, static_cast<BYTE> (i + 1)));
        break;
      }
    }

  if (LastSuggestions->size () > 0)
    {
      switch (settings ().suggestionControlType)
      {
      case SuggestionControlType::box:
        InsertSuggMenuItem (Menu, _T (""), 0, 103, TRUE);
        break;
      case SuggestionControlType::menu:
        SuggestionMenuItems->push_back (new SuggestionsMenuItem (_T (""), 0, TRUE));
        break;
      }
    }

  TCHAR *MenuString = new TCHAR [WUCLength + 50 + 1]; // Add "" to dictionary
  char *BufUtf8 = 0;
  if (currentEncoding == ENCODING_UTF8)
    setString (BufUtf8, Range.lpstrText);
  else
    SetStringDUtf8 (BufUtf8, Range.lpstrText);
  ApplyConversions (BufUtf8);
  SetStringSUtf8 (Buf, BufUtf8);
  _stprintf (MenuString, _T ("Ignore \"%s\" for Current Session"), Buf);
  switch (settings ().suggestionControlType)
  {
  case SuggestionControlType::box:
    InsertSuggMenuItem (Menu, MenuString, MID_IGNOREALL, -1);
    break;
  case SuggestionControlType::menu:
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (MenuString, MID_IGNOREALL));
    break;
  }
  _stprintf (MenuString, _T ("Add \"%s\" to Dictionary"), Buf);
  switch (settings ().suggestionControlType)
  {
  case SuggestionControlType::box:
    InsertSuggMenuItem (Menu, MenuString, MID_ADDTODICTIONARY, -1);
    break;
  case SuggestionControlType::menu:
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (MenuString, MID_ADDTODICTIONARY));
    SuggestionMenuItems->push_back (new SuggestionsMenuItem (_T (""), 0, TRUE));
    break;
  }

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
  SendMessage (NppDataInstance->_nppHandle, getCustomGUIMessageId (CustomGUIMessage::AUTOCHECK_STATE_CHANGED), AutoCheckText, 0);
}

void SpellChecker::RefreshUnderlineStyle ()
{
  SendMsgToBothEditors(NppDataInstance, SCI_INDICSETSTYLE, SCE_ERROR_UNDERLINE, settings ().underlineStyle);
  SendMsgToBothEditors(NppDataInstance, SCI_INDICSETFORE, SCE_ERROR_UNDERLINE, settings ().underlineColor);
}

void SpellChecker::SetProxyUserName (TCHAR *Str)
{
  setString (ProxyUserName, Str);
}

void SpellChecker::SetProxyHostName (TCHAR *Str)
{
  setString (ProxyHostName, Str);
}

void SpellChecker::SetProxyPassword (TCHAR *Str)
{
  setString (ProxyPassword, Str);
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

TCHAR *SpellChecker::GetProxyUserName ()
{
  return ProxyUserName;
}

TCHAR *SpellChecker::GetProxyHostName ()
{
  return ProxyHostName;
}

TCHAR *SpellChecker::GetProxyPassword ()
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

void SpellChecker::GetDefaultHunspellPath (TCHAR *&Path)
{
  Path = new TCHAR[MAX_PATH];
  _tcscpy (Path, IniFilePath);
  TCHAR *Pointer = _tcsrchr (Path, _T ('\\'));
  *Pointer = 0;
  _tcscat (Path, _T ("\\Hunspell"));
}

void SpellChecker::SaveSettings ()
{
  FILE *Fp;
  _tfopen_s (&Fp, IniFilePath, _T ("w")); // Cleaning settings file (or creating it)
  fclose (Fp);
  if (!SettingsLoaded)
    return;
  SaveToIni (_T ("Autocheck"), AutoCheckText, 1);
  SaveToIni (_T ("Hunspell_Multiple_Languages"), m_settings->get ().spellerSettings[SpellerType::hunspell].activeMultiLanguage.data (), _T (""));
  SaveToIni (_T ("Aspell_Multiple_Languages"), m_settings->get ().spellerSettings[SpellerType::aspell].activeMultiLanguage.data (), _T (""));
  SaveToIni (_T ("Hunspell_Language"), m_settings->get ().spellerSettings[SpellerType::hunspell].activeLanguage.data (), _T ("en_GB"));
  SaveToIni (_T ("Aspell_Language"), m_settings->get ().spellerSettings[SpellerType::aspell].activeLanguage.data (), _T ("en"));
  SaveToIni (_T ("Remove_User_Dics_On_Dic_Remove"), RemoveUserDics, 0);
  SaveToIni (_T ("Remove_Dics_For_All_Users"), RemoveSystem, 0);
  SaveToIni (_T ("Show_Only_Known"), ShowOnlyKnown, TRUE);
  SaveToIni (_T ("Install_Dictionaries_For_All_Users"), InstallSystem, FALSE);
  TCHAR Buf[DEFAULT_BUF_SIZE];
  for (int i = 0; i < countof (ServerNames); i++)
  {
    if (!*ServerNames[i])
      continue;
    _stprintf (Buf, _T ("Server_Address[%d]"), i);
    SaveToIni (Buf, ServerNames[i], _T (""));
  }
  SaveToIni (_T ("Suggestions_Control"), static_cast<int> (settings ().suggestionControlType), 1);
  SaveToIni (_T ("Ignore_Yo"), settings ().treatYoAsYe, 0);
  SaveToIni (_T ("Convert_Single_Quotes_To_Apostrophe"), settings ().convertUnicodeQuotes, 1);
  SaveToIni (_T ("Remove_Ending_And_Beginning_Apostrophe"), settings ().skipStartingFinishingApostrophe, 1);
  SaveToIni (_T ("Check_Only_Comments_And_Strings"), settings ().skipCode, 1);
  SaveToIni (_T ("Check_Those_\\_Not_Those"), static_cast<int> (settings ().fileMaskOption), 1);
  SaveToIni (_T ("File_Types"), settings ().fileMask.c_str (), _T ("*.*"));
  SaveToIni (_T ("Ignore_Having_Number"), settings ().skipNumbers, 1);
  SaveToIni (_T ("Ignore_Start_Capital"), settings ().skipCapitalizedWords, 0);
  SaveToIni (_T ("Ignore_Have_Capital"), settings ().skipWordsHavingCapitals, 1);
  SaveToIni (_T ("Ignore_All_Capital"), settings ().skipWordsHavingOnlyCapitals, 1);
  SaveToIni (_T ("Ignore_With_"), settings ().skipWordsWithUnderline, 1);
  SaveToIni (_T ("Ignore_That_Start_or_End_with_'"), settings ().skipWordsStartingOrFinishingWithApostrophe , 0);
  SaveToIni (_T ("Ignore_One_Letter"), settings ().skipOneLetterWords , 0);
  SaveToIni (_T ("Underline_Color"), settings ().underlineColor, 0x0000ff);
  SaveToIni (_T ("Underline_Style"), settings ().underlineStyle, INDIC_SQUIGGLE);
  TCHAR *Path = 0;
  GetDefaultAspellPath (Path);
  SaveToIni (_T ("Aspell_Path"), m_settings->get ().spellerSettings[SpellerType::aspell].path.data (), Path);
  CLEAN_AND_ZERO_ARR (Path);
  GetDefaultHunspellPath (Path);
  SaveToIni (_T ("User_Hunspell_Path"), m_settings->get ().spellerSettings[SpellerType::hunspell].path.data (), Path);
  SaveToIni (_T ("System_Hunspell_Path"), m_settings->get ().spellerSettings[SpellerType::hunspell].additionalPath.data (), _T (".\\plugins\\config\\Hunspell"));
  CLEAN_AND_ZERO_ARR (Path);
  SaveToIni (_T ("Suggestions_Number"), settings ().suggestionCount, 5);
  SaveToIni (_T ("Delimiters"), settings ().delimiters.c_str (), DEFAULT_DELIMITERS, TRUE);
  SaveToIni (_T ("Find_Next_Buffer_Size"), settings ().bufferSizeForSearch, 4);
  SaveToIni (_T ("Suggestions_Button_Size"), settings ().suggestionBoxSize, 15);
  SaveToIni (_T ("Suggestions_Button_Opacity"), settings ().suggestionBoxTransparency, 70);
  SaveToIni (_T ("Library"), static_cast<int> (m_settings->get ().activeSpellerType), 1);
  PreserveCurrentAddressIndex ();
  SaveToIni (_T ("Last_Used_Address_Index"), LastUsedAddress, 0);
  SaveToIni (_T ("Decode_Language_Names"), settings ().useLanguageNameAliases, TRUE);
  SaveToIni (_T ("United_User_Dictionary(Hunspell)"), settings ().unifiedUserDictionary, false);

  SaveToIni (_T ("Use_Proxy"), UseProxy, FALSE);
  SaveToIni (_T ("Proxy_User_Name"), ProxyUserName, _T("anonymous"));
  SaveToIni (_T ("Proxy_Host_Name"), ProxyHostName, _T(""));
  SaveToIni (_T ("Proxy_Password"), ProxyPassword, _T(""));
  SaveToIni (_T ("Proxy_Port"), ProxyPort, 808);
  SaveToIni (_T ("Proxy_Is_Anonymous"), ProxyAnonymous, TRUE);
  SaveToIni (_T ("Proxy_Type"), ProxyType, 0);
  std::map<TCHAR *, DWORD, bool ( *)(TCHAR *, TCHAR *)>::iterator it = SettingsToSave->begin ();
  for (; it != SettingsToSave->end (); ++it)
  {
    SaveToIni ((*it).first, LOWORD ((*it).second), HIWORD ((*it).second));
  }
}

void SpellChecker::setNewSettings(const SettingsData &settingsArg)
{
  // Mostly settings are used automatically but some of them are heavy to change (i.e. speller settings)
  // so we gotta change them only in case if they are really changed
  // code looks kinda clumsy right now
  auto oldSettings = settingsArg;
  m_settings->exchange (oldSettings);

  auto oldType = oldSettings.activeSpellerType;
  auto newType = settings ().activeSpellerType;
  auto &oldS = oldSettings.spellerSettings[newType];
  auto &newS = settings ().spellerSettings[newType];

  if (oldType != newType || oldS.path != newS.path)
    m_spellers[newType]->setPath (newS.path.data ());

  if (oldType != newType || oldS.path != newS.path)
    m_spellers[newType]->setAdditionalPath (newS.additionalPath.data ());

  if (oldType != newType)
    ReinitLanguageLists (FALSE);

  if (oldType != newType || oldS.activeLanguage != newS.activeLanguage)
    {
       if (newS.activeLanguage != L"<MULTIPLE>")
         m_spellers[newType]->setLanguage(newS.activeLanguage.data ());

       m_spellers[newType]->setMultiMode (newS.activeLanguage == L"<MULTIPLE>");
    }

  if (newS.activeLanguage == L"<MULTIPLE>" && (oldType != newType || oldS.activeMultiLanguage != newS.activeMultiLanguage))
    SetMultipleLanguages(newS.activeMultiLanguage.data (), m_spellers[newType].get ());

  if (firstTime || settings ().delimiters != oldSettings.delimiters)
    setDelimiters(settings ().delimiters.c_str ());

  if (firstTime || settings ().unifiedUserDictionary != oldSettings.unifiedUserDictionary)
    static_cast<HunspellController &> (*m_spellers[SpellerType::hunspell]).SetUseOneDic (settings ().unifiedUserDictionary);

  if (firstTime || settings ().suggestionControlType != oldSettings.suggestionControlType)
    hideSuggestionBox ();

  if (firstTime || settings ().suggestionBoxTransparency != oldSettings.suggestionBoxTransparency)
    {
      // Don't sure why but this helps to fix a bug with notepad++ window resizing
      // TODO: Fix it normal way
      SetLayeredWindowAttributes (SuggestionsInstance->getHSelf (), 0, static_cast<BYTE> ((255 * settings ().suggestionBoxTransparency) / 100), LWA_ALPHA);
    }

  bool needRefreshUnderlineStyle = false;
  if (firstTime || m_settings->get ().underlineColor != oldSettings.underlineColor || settings ().underlineStyle != oldSettings.underlineStyle)
    needRefreshUnderlineStyle = true;
    RefreshUnderlineStyle ();

  if (firstTime || settings ().fileMask != oldSettings.fileMask || settings ().fileMaskOption != oldSettings.fileMaskOption)
    CheckFileName();

  PostMessage (NppDataInstance->_nppHandle,
    getCustomGUIMessageId (CustomGUIMessage::CONFIGURATION_CHANGED),
    0, 0);

  RecheckVisibleBothViews ();
  firstTime = false;
};

void SpellChecker::LoadSettings ()
{
  char *BufUtf8 = 0;
  TCHAR *Path = 0;
  SettingsLoaded = TRUE;
  GetDefaultAspellPath (Path);
  auto settings = m_settings->change ();
  SettingsData newSettings;
  LoadFromIni (settings->spellerSettings[SpellerType::aspell].path, _T ("Aspell_Path"), Path);
  CLEAN_AND_ZERO_ARR (Path);
  GetDefaultHunspellPath (Path);
  LoadFromIni (settings->spellerSettings[SpellerType::hunspell].path, _T ("User_Hunspell_Path"), Path);
  CLEAN_AND_ZERO_ARR (Path);

  LoadFromIni (settings->spellerSettings[SpellerType::hunspell].additionalPath, _T ("System_Hunspell_Path"), _T (".\\plugins\\config\\Hunspell"));

  int val;
  LoadFromIni (val, _T ("Suggestions_Control"), 1);
  settings->suggestionControlType = static_cast<SuggestionControlType> (val);
  LoadFromIni (AutoCheckText, _T ("Autocheck"), 1);
  UpdateAutocheckStatus (0);
  LoadFromIni (settings->spellerSettings[SpellerType::aspell].activeMultiLanguage, _T ("Aspell_Multiple_Languages"), _T (""));
  LoadFromIni (settings->spellerSettings[SpellerType::hunspell].activeMultiLanguage, _T ("Hunspell_Multiple_Languages"), _T (""));
  LoadFromIni (settings->spellerSettings[SpellerType::aspell].activeLanguage, _T ("Aspell_Language"), _T ("en"));
  LoadFromIni (settings->spellerSettings[SpellerType::hunspell].activeLanguage, _T ("Hunspell_Language"), _T ("en_GB"));

  LoadFromIni (settings->delimiters, _T ("Delimiters"), DEFAULT_DELIMITERS);
  LoadFromIni (settings->suggestionCount, _T ("Suggestions_Number"), 5);
  LoadFromIni (settings->treatYoAsYe, _T ("Ignore_Yo"), 0);
  LoadFromIni (settings->convertUnicodeQuotes, _T ("Convert_Single_Quotes_To_Apostrophe"), 1);
  LoadFromIni (settings->skipStartingFinishingApostrophe, _T ("Remove_Ending_And_Beginning_Apostrophe"), 1);
  int i = 0;
  LoadFromIni (i , _T ("Check_Those_\\_Not_Those"), 1);
  settings->fileMaskOption = static_cast<FileMaskOption> (i);
  LoadFromIni (settings->fileMask, _T ("File_Types"), _T ("*.*"));
  LoadFromIni (settings->skipCode, _T ("Check_Only_Comments_And_Strings"), 1);
  LoadFromIni (settings->underlineColor, _T ("Underline_Color"), 0x0000ff);
  LoadFromIni (settings->underlineStyle, _T ("Underline_Style"), INDIC_SQUIGGLE);
  LoadFromIni (settings->skipNumbers, _T ("Ignore_Having_Number"), 1);
  LoadFromIni (settings->skipCapitalizedWords, _T ("Ignore_Start_Capital"), 0);
  LoadFromIni (settings->skipWordsHavingCapitals, _T ("Ignore_Have_Capital"), 1);
  LoadFromIni (settings->skipWordsHavingOnlyCapitals, _T ("Ignore_All_Capital"), 1);
  LoadFromIni (settings->skipOneLetterWords, _T ("Ignore_One_Letter"), 0);
  LoadFromIni (settings->skipWordsWithUnderline, _T ("Ignore_With_"), 1);
  LoadFromIni (settings->unifiedUserDictionary, _T ("United_User_Dictionary(Hunspell)"), false);
  LoadFromIni (settings->skipWordsStartingOrFinishingWithApostrophe, _T ("Ignore_That_Start_or_End_with_'"), 0);

  static_cast<AspellController &> (*m_spellers[SpellerType::aspell]).setPath (settings->spellerSettings[SpellerType::aspell].path.data ());
  LoadFromIni (i, _T ("Library"), 1);
  settings->activeSpellerType = static_cast<SpellerType> (i);
  LoadFromIni (settings->suggestionBoxSize, _T ("Suggestions_Button_Size"), 15);
  LoadFromIni (settings->suggestionBoxTransparency, _T ("Suggestions_Button_Opacity"), 70);
  LoadFromIni (settings->bufferSizeForSearch, _T ("Find_Next_Buffer_Size"), 4);
  CLEAN_AND_ZERO_ARR (BufUtf8);
  LoadFromIni (ShowOnlyKnown, _T ("Show_Only_Known"), TRUE);
  LoadFromIni (InstallSystem, _T ("Install_Dictionaries_For_All_Users"), FALSE);
  TCHAR Buf[DEFAULT_BUF_SIZE];
  for (int i = 0; i < countof (ServerNames); i++)
  {
    _stprintf (Buf, _T ("Server_Address[%d]"), i);
    LoadFromIni (ServerNames[i], Buf, _T (""));
  }
  LoadFromIni (LastUsedAddress, _T ("Last_Used_Address_Index"), 0);
  LoadFromIni (RemoveUserDics, _T ("Remove_User_Dics_On_Dic_Remove"), 0);
  LoadFromIni (RemoveSystem, _T ("Remove_Dics_For_All_Users"), 0);
  LoadFromIni (settings->useLanguageNameAliases, _T ("Decode_Language_Names"), true);

  LoadFromIni (UseProxy, _T ("Use_Proxy"), FALSE);
  LoadFromIni (ProxyUserName, _T ("Proxy_User_Name"), _T("anonymous"));
  LoadFromIni (ProxyHostName, _T ("Proxy_Host_Name"), _T(""));
  LoadFromIni (ProxyPassword, _T ("Proxy_Password"), _T(""));
  LoadFromIni (ProxyPort, _T ("Proxy_Port"), 808);
  LoadFromIni (ProxyAnonymous, _T ("Proxy_Is_Anonymous"), TRUE);
  LoadFromIni (ProxyType, _T ("Proxy_Type"), 0);
}

void SpellChecker::CreateWordUnderline (HWND ScintillaWindow, int start, int end)
{
  PostMsgToActiveEditor (ScintillaWindow, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
  PostMsgToActiveEditor (ScintillaWindow, SCI_INDICATORFILLRANGE, start, (end - start + 1));
}

void SpellChecker::RemoveUnderline (HWND ScintillaWindow, int start, int end)
{
  if (end < start)
    return;
  PostMsgToActiveEditor (ScintillaWindow, SCI_SETINDICATORCURRENT, SCE_ERROR_UNDERLINE);
  PostMsgToActiveEditor (ScintillaWindow, SCI_INDICATORCLEARRANGE, start, (end - start + 1));
}


void SpellChecker::GetVisibleLimits (long &Start, long &Finish)
{
  BOOL ok;
  long top = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETFIRSTVISIBLELINE);
  if (!ok) goto ErrorBranch;
  long bottom = top + SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINESONSCREEN);
  if (!ok) goto ErrorBranch;
  top = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_DOCLINEFROMVISIBLE, top);
  if (!ok) goto ErrorBranch;
  bottom = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_DOCLINEFROMVISIBLE, bottom);
  if (!ok) goto ErrorBranch;
  long LineCount = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLINECOUNT);
  if (!ok) goto ErrorBranch;
  Start = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, top);
  if (!ok) goto ErrorBranch;
  // Not using end of line position cause utf-8 symbols could be more than one char
  // So we use next line start as the end of our visible text
  if (bottom + 1 < LineCount)
    {
      Finish = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, bottom + 1);
      if (!ok) goto ErrorBranch;
    }
  else
    {
      Finish = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTLENGTH);
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
  int length = SendMsgToActiveEditor(&ok, GetCurrentScintilla (), SCI_GETLENGTH);
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
  CLEAN_AND_ZERO_ARR (DelimUtf8Converted);
  CLEAN_AND_ZERO_ARR (DelimConverted);
}

void SpellChecker::SaveToIni (const TCHAR *Name, const TCHAR *Value, const TCHAR *DefaultValue, BOOL InQuotes)
{
  if (!Name || !Value)
    return;

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
  if (!Name)
    return;

  if (Value == DefaultValue)
    return;

  TCHAR Buf[DEFAULT_BUF_SIZE];
  _itot_s (Value, Buf, 10);
  SaveToIni (Name, Buf, 0);
}

void SpellChecker::SaveToIniUtf8 (const TCHAR *Name, const char *Value,  const char *DefaultValue, BOOL InQuotes)
{
  if (!Name || !Value)
    return;

  if (DefaultValue && strcmp (Value, DefaultValue) == 0)
    return;

  TCHAR *Buf = 0;
  SetStringSUtf8 (Buf, Value);
  SaveToIni (Name, Buf, 0, InQuotes);
  CLEAN_AND_ZERO_ARR (Buf);
}

void SpellChecker::LoadFromIni (TCHAR *&Value, const TCHAR *Name, const TCHAR *DefaultValue, BOOL InQuotes)
{
  if (!Name || !DefaultValue)
    return;

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

void SpellChecker::LoadFromIni (std::wstring &value, const TCHAR *name, const TCHAR *defaultValue, BOOL inQuotes)
{
  wchar_t *str = nullptr;
  LoadFromIni (str, name, defaultValue, inQuotes);
  value = str;
}

void SpellChecker::LoadFromIni (int &Value, const TCHAR *Name, int DefaultValue)
{
  if (!Name)
    return;

  TCHAR BufDefault[DEFAULT_BUF_SIZE];
  TCHAR *Buf = 0;
  _itot_s (DefaultValue, BufDefault, 10);
  LoadFromIni (Buf, Name, BufDefault);
  Value = _ttoi (Buf);
  CLEAN_AND_ZERO_ARR (Buf);
}


void SpellChecker::LoadFromIni (bool &Value, const TCHAR *Name, bool DefaultValue)
{
  int i;
  LoadFromIni (i, Name, static_cast<int> (DefaultValue));
  Value = (i != 0);
}

void SpellChecker::LoadFromIniUtf8 (char *&Value, const TCHAR *Name, const char *DefaultValue, BOOL InQuotes)
{
  if (!Name || !DefaultValue)
    return;

  TCHAR *BufDefault = 0;
  TCHAR *Buf = 0;
  SetStringSUtf8 (BufDefault, DefaultValue);
  LoadFromIni (Buf, Name, BufDefault, InQuotes);
  SetStringDUtf8 (Value, Buf);
  CLEAN_AND_ZERO_ARR (Buf);
  CLEAN_AND_ZERO_ARR (BufDefault);
}

// Here parameter is in UTF-8
void SpellChecker::setDelimiters(const wchar_t *str)
{
  TCHAR *DestBuf = 0;
  TCHAR *SrcBuf = 0;
  setString (SrcBuf, str);
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

void SpellChecker::SetMultipleLanguages (const TCHAR *MultiString, SpellerController *Speller)
{
  TCHAR *Context = 0;
  std::vector<TCHAR *> *MultiLangList = new std::vector<TCHAR *>;
  TCHAR *MultiStringCopy = 0;
  TCHAR *Token = 0;
  TCHAR *TBuf = 0;

  setString (MultiStringCopy, MultiString);
  Token = _tcstok_s (MultiStringCopy, _T ("|"), &Context);
  while (Token)
  {
    TBuf = 0;
    setString (TBuf, Token);
    MultiLangList->push_back (TBuf);
    Token = _tcstok_s (NULL, _T ("|"), &Context);
  }

  Speller->SetMultipleLanguages (MultiLangList);
  CLEAN_AND_ZERO_ARR (MultiStringCopy);
  CLEAN_AND_ZERO_STRING_VECTOR (MultiLangList);
}

void SpellChecker::ApplyConversions (char *Word) // In Utf-8, Maybe shortened during conversion
{
  const char *ConvertFrom [3];
  const char *ConvertTo [3];
  int Apply[3] = {settings ().treatYoAsYe, settings ().treatYoAsYe, settings ().convertUnicodeQuotes};

  if (currentEncoding == ENCODING_ANSI)
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
    int Diff = strlen (ConvertFrom[i]) - strlen (ConvertTo[i]);
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
  if (!activeSpeller()->IsWorking () || !Word || !*Word)
    return TRUE;
  // Well Numbers have same codes for ANSI and Unicode I guess, so
  // If word contains number then it's probably just a number or some crazy name
  int Style = GetStyle (Start);
  if (settings ().skipCode && !CheckWordInCommentOrString (Style))
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

  TCHAR *Ts = 0;
  long SymbolsNum = (currentEncoding == ENCODING_UTF8) ? Utf8Length (Word) : strlen (Word);
  if (SymbolsNum == 0)
  {
    res = TRUE;
    goto CleanUp;
  }

  if (settings().skipOneLetterWords && SymbolsNum == 1)
  {
    res = TRUE;
    goto CleanUp;
  }

  if (settings ().skipNumbers && (currentEncoding == ENCODING_UTF8 ? Utf8pbrk (Word, "0123456789") : strpbrk (Word, "0123456789")) != 0) // Same for UTF-8 and not
  {
    res = TRUE;
    goto CleanUp;
  }

  if (settings ().skipCapitalizedWords || settings ().skipWordsHavingCapitals || settings ().skipWordsHavingOnlyCapitals)
  {
    if (currentEncoding == ENCODING_UTF8)
      SetStringSUtf8 (Ts, Word);
    else
      setString (Ts, Word);
    if (settings ().skipCapitalizedWords && IsCharUpper (Ts[0]))
    {
      res = TRUE;
      goto CleanUp;
    }
    if (settings ().skipWordsHavingCapitals || settings ().skipWordsHavingOnlyCapitals)
    {
      BOOL AllUpper = IsCharUpper (Ts[0]);
      for (unsigned int i = 1; i < _tcslen (Ts); i++)
      {
        if (IsCharUpper (Ts[i]))
        {
          if (settings ().skipWordsHavingCapitals)
          {
            res = TRUE;
            goto CleanUp;
          }
        }
        else
          AllUpper = FALSE;
      }

      if (AllUpper && settings ().skipWordsHavingOnlyCapitals)
      {
        res = TRUE;
        goto CleanUp;
      }
    }
  }

  if (settings ().skipWordsWithUnderline && strchr (Word, '_') != 0) // I guess the same for UTF-8 and ANSI
  {
    res = TRUE;
    goto CleanUp;
  }

  unsigned int Len = strlen (Word);

  if (settings ().skipWordsStartingOrFinishingWithApostrophe)
  {
    if (Word[0] == '\'' || Word[Len - 1] == '\'')
    {
      res = TRUE;
      goto CleanUp;
    }
  }

  res = activeSpeller()->CheckWord (Word);

CleanUp:
  CLEAN_AND_ZERO_ARR (Ts);

  return res;
}

void SpellChecker::CheckSpecialDelimeters (char *&Word, const char *TextStart, long &WordStart, long &WordEnd)
{
  if (settings ().skipStartingFinishingApostrophe)
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
  long TextLen = strlen (TextToCheck);
  std::vector<long> UnderlineBuffer;
  long WordStart = 0;
  long WordEnd = 0;

  if (!DelimUtf8Converted)
    return CheckTextDefaultAnswer (Mode);

  if (currentEncoding == ENCODING_UTF8)
    token = Utf8strtok (TextToCheck, DelimUtf8Converted, &Context);
  else
    token = strtok_s (TextToCheck, DelimConverted, &Context);

  while (token)
    {
      if (token)
        {
          WordStart = Offset + token - TextToCheck;
          WordEnd = Offset + token - TextToCheck + strlen (token);
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
      if (currentEncoding == ENCODING_UTF8)
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
      RemoveUnderline (ScintillaWindow, PrevPos, Offset + TextLen - 1);
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
  int length = SendMsgToActiveEditor(&ok, GetCurrentScintilla (), SCI_GETLENGTH);
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
    currentEncoding = ENCODING_UTF8;
    // SetEncoding ("utf-8");
    break;
  default:
    {
      currentEncoding = ENCODING_ANSI;
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
  for (auto &speller : m_spellers)
    speller->setEncoding (currentEncoding);
}

void SpellChecker::SwitchAutoCheck ()
{
  AutoCheckText = !AutoCheckText;
  UpdateAutocheckStatus ();
  RecheckVisibleBothViews ();
}

void SpellChecker::RecheckModified ()
{
  if (!activeSpeller ()->IsWorking ())
    {
      ClearAllUnderlines ();
      return;
    }

  BOOL ok;
  long FirstModifiedLine = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, ModifiedStart);
  if (!ok) return;
  long LastModifiedLine = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_LINEFROMPOSITION, ModifiedEnd);
  if (!ok) return;
  long LineCount = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLINECOUNT);
  if (!ok) return;
  long FirstPossiblyModifiedPos = SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_POSITIONFROMLINE, FirstModifiedLine);
  if (!ok) return;
  long LastPossiblyModifiedPos = 0;
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
  Range.chrg.cpMin = FirstPossiblyModifiedPos;
  Range.chrg.cpMax = LastPossiblyModifiedPos;
  Range.lpstrText = new char [Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1];
  SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETTEXTRANGE, 0, (LPARAM) &Range);

  CheckText (Range.lpstrText, FirstPossiblyModifiedPos, UNDERLINE_ERRORS);
  CLEAN_AND_ZERO_ARR (Range.lpstrText);
}

void SpellChecker::RecheckVisible (BOOL NotIntersectionOnly)
{
  int CodepageId = 0;
  if (!activeSpeller ()->IsWorking ())
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

void SpellChecker::ErrorMsgBox (const TCHAR *message)
{
  TCHAR buf [DEFAULT_BUF_SIZE];
  _stprintf_s (buf, _T ("%s"), "DSpellCheck Error:", message, _tcslen (message));
  MessageBox (NppDataInstance->_nppHandle, message, _T("Error Happened!"), MB_OK | MB_ICONSTOP);
}

void SpellChecker::CopyMisspellingsToClipboard ()
{
  BOOL ok;
  int lengthDoc = (SendMsgToActiveEditor (&ok, GetCurrentScintilla (), SCI_GETLENGTH) + 1);
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

  TCHAR *wchar_str = 0;

  switch (currentEncoding)
  {
  case ENCODING_UTF8:
    SetStringSUtf8 (wchar_str, str.c_str ());
    break;
  case ENCODING_ANSI:
    setString (wchar_str, str.c_str ());
    break;
  }

  const size_t len = (_tcslen (wchar_str) + 1) * 2;
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

HunspellController * SpellChecker::GetHunspellSpeller ()
{
  return static_cast<HunspellController *> (m_spellers[SpellerType::hunspell].get ());
}

const wchar_t * SpellChecker::currentLanguage ()
{
  return m_settings->get ().spellerSettings[m_settings->get ().activeSpellerType].activeLanguage.data ();
}

const SettingsData & SpellChecker::settings () const
{
  return m_settings->get ();
}

std::shared_ptr<const SettingsData> SpellChecker::getSettings ()
{
  return m_settings->cloneData ();
}

void SpellChecker::onSettingsChanged (const SettingsData &settingsData)
{
  setNewSettings (settingsData);
}

void SpellChecker::onHunspellDictionariesChange ()
{
  m_spellers[SpellerType::hunspell]->setPath (m_settings->get ().spellerSettings[SpellerType::hunspell].path.data ());
  m_spellers[SpellerType::hunspell]->setAdditionalPath (m_settings->get ().spellerSettings[SpellerType::hunspell].additionalPath.data ());
  ReinitLanguageLists (TRUE);
  DoPluginMenuInclusion ();
  RecheckVisibleBothViews ();
}

SuggestionsMenuItem::SuggestionsMenuItem (TCHAR *TextArg, BYTE IdArg, BOOL SeparatorArg /*= FALSE*/)
{
  Text = 0;
  setString (Text, TextArg);
  Id = IdArg;
  Separator = SeparatorArg;
}
