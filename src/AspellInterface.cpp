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

#include "AspellInterface.h"
#include "Aspell.h"
#include "CommonFunctions.h"
#include "MainDef.h"
#include "plugin.h"

AspellInterface::AspellInterface (HWND NppWindowArg)
{
  NppWindow = NppWindowArg;
  SingularSpeller = 0;
  LastSelectedSpeller = 0;
  Spellers = new std::vector<AspellSpeller *>;
  AspellLoaded = 0;
}

AspellInterface::~AspellInterface ()
{
  if (!AspellLoaded)
  {
    CLEAN_AND_ZERO (Spellers); // Probably in that case nothing should be created
    return;
  }

  delete_aspell_speller (SingularSpeller);
  SingularSpeller = 0;
  for (unsigned int i = 0; i < Spellers->size (); i++)
    delete_aspell_speller (Spellers->at (i));

  CLEAN_AND_ZERO (Spellers);
}

std::vector<TCHAR *> *AspellInterface::GetLanguageList ()
{
  if (!AspellLoaded)
    return 0;
  AspellConfig *aspCfg;
  const AspellDictInfo *entry;
  AspellDictInfoList *dlist;
  AspellDictInfoEnumeration *dels;
  std::vector<TCHAR *> *Names = new std::vector<TCHAR *>;

  aspCfg = new_aspell_config();

  /* the returned pointer should _not_ need to be deleted */
  dlist = get_aspell_dict_info_list(aspCfg);

  /* config is no longer needed */
  delete_aspell_config(aspCfg);

  dels = aspell_dict_info_list_elements(dlist);

  if (aspell_dict_info_enumeration_at_end(dels) == TRUE)
  {
    delete_aspell_dict_info_enumeration(dels);
    return 0;
  }

  while ((entry = aspell_dict_info_enumeration_next(dels)) != 0)
  {
    TCHAR *TBuf = 0;
    SetString (TBuf, entry->name);
    Names->push_back (TBuf);
  }

  std::sort (Names->begin (), Names->end (), SortCompare);
  return Names;
}

BOOL AspellInterface::IsWorking ()
{
  return AspellLoaded;
}

void AspellInterface::SendAspellErorr (AspellCanHaveError *Error)
{
  TCHAR *ErrorMsg = 0;
  SetString (ErrorMsg, aspell_error_message (Error));
  MessageBoxInfo MsgBox (NppWindow, ErrorMsg, _T ("Aspell Error"), MB_OK | MB_ICONEXCLAMATION);
  SendMessage (NppWindow, GetCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
  CLEAN_AND_ZERO_ARR (ErrorMsg);
}

void AspellInterface::SetMultipleLanguages (std::vector<TCHAR *> *List)
{
  if (!AspellLoaded)
    return;

  Spellers->clear ();
  for (unsigned int i = 0; i < List->size (); i++)
  {
    AspellConfig *SpellConfig = new_aspell_config();
    aspell_config_replace (SpellConfig, "encoding", "utf-8");
    char *Buf = 0;
    SetString (Buf, List->at (i));
    aspell_config_replace(SpellConfig, "lang", Buf);
    CLEAN_AND_ZERO_ARR (Buf);
    AspellCanHaveError *PossibleErr = new_aspell_speller(SpellConfig);
    if (aspell_error_number(PossibleErr) == 0)
    {
      Spellers->push_back (to_aspell_speller(PossibleErr));
    }
    else
      SendAspellErorr (PossibleErr);

    delete_aspell_config (SpellConfig);
  }
}

std::vector<char *> *AspellInterface::GetSuggestions (char *Word)
{
  const AspellWordList *WordList = 0, *CurWordList = 0;

  char *TargetWord = 0;

  if (CurrentEncoding == ENCODING_UTF8)
    TargetWord = Word;
  else
    SetStringDUtf8 (TargetWord, Word);

  std::vector<char *> *SuggList = new std::vector<char *>;
  if (!MultiMode)
  {
    LastSelectedSpeller = SingularSpeller;
    WordList = aspell_speller_suggest (SingularSpeller, TargetWord, -1);
  }
  else
  {
    int MaxSize = -1;
    int Size;
    // In this mode we're finding maximum by length list from selected languages
    CurWordList = 0;
    for (int i = 0; i < (int) Spellers->size (); i++)
    {
      CurWordList = aspell_speller_suggest (Spellers->at (i), TargetWord, -1);

      Size = aspell_word_list_size (CurWordList);

      if (Size > MaxSize)
      {
        MaxSize = Size;
        LastSelectedSpeller = Spellers->at (i);
        WordList = CurWordList;
      }
    }
  }
  if (!WordList)
    return 0;

  AspellStringEnumeration *els = aspell_word_list_elements(WordList);
  const char *Suggestion;

  while ((Suggestion = aspell_string_enumeration_next(els)) != 0)
  {
    char *Buf = 0;
    SetString (Buf, Suggestion);
    SuggList->push_back (Buf);
  }

  if (CurrentEncoding == ENCODING_ANSI)
    CLEAN_AND_ZERO_ARR (TargetWord);

  return SuggList;
}

void AspellInterface::AddToDictionary (char *Word)
{
  char *TargetWord = 0;

  if (CurrentEncoding == ENCODING_UTF8)
    TargetWord = Word;
  else
    SetStringDUtf8 (TargetWord, Word);

  if (!LastSelectedSpeller)
    return;
  aspell_speller_add_to_personal(LastSelectedSpeller, Word, static_cast<int> (strlen (Word)) + 1);
  aspell_speller_save_all_word_lists (LastSelectedSpeller);
  if (aspell_speller_error(LastSelectedSpeller) != 0)
  {
    TCHAR *ErrorMsg = 0;
    SetString (ErrorMsg, aspell_speller_error_message (LastSelectedSpeller));
    MessageBoxInfo MsgBox (NppWindow, ErrorMsg, _T ("Aspell Error"), MB_OK | MB_ICONEXCLAMATION);
    SendMessage (NppWindow, GetCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
    CLEAN_AND_ZERO_ARR (ErrorMsg);
  }
  LastSelectedSpeller = 0;

  if (CurrentEncoding == ENCODING_ANSI)
    CLEAN_AND_ZERO_ARR (TargetWord);
}

void AspellInterface::IgnoreAll (char *Word)
{
  if (!LastSelectedSpeller)
    return;

  char *TargetWord = 0;

  if (CurrentEncoding == ENCODING_UTF8)
    TargetWord = Word;
  else
    SetStringDUtf8 (TargetWord, Word);

  aspell_speller_add_to_session (LastSelectedSpeller, TargetWord, static_cast<int> (strlen (TargetWord)) + 1);
  aspell_speller_save_all_word_lists (LastSelectedSpeller);
  if (aspell_speller_error(LastSelectedSpeller) != 0)
  {
    AspellErrorMsgBox (0, aspell_speller_error_message (LastSelectedSpeller));
  }
  LastSelectedSpeller = 0;
  if (CurrentEncoding == ENCODING_ANSI)
    CLEAN_AND_ZERO_ARR (TargetWord);
}

BOOL AspellInterface::CheckWord (char *Word)
{
  if (!AspellLoaded)
    return TRUE;

  char *DstWord = 0;
  BOOL res = FALSE;
  if (CurrentEncoding == ENCODING_UTF8)
    DstWord = Word;
  else
    SetStringDUtf8 (DstWord, Word);

  auto Len = static_cast<int> (strlen (DstWord));
  if (!MultiMode)
  {
    if (!SingularSpeller)
      return TRUE;

    res = aspell_speller_check(SingularSpeller, DstWord, Len);
  }
  else
  {
    if (!Spellers || Spellers->size () == 0)
      return TRUE;

    for (int i = 0; i < (int )Spellers->size () && !res; i++)
    {
      res = res || aspell_speller_check(Spellers->at (i), DstWord, Len);
    }
  }
  if (CurrentEncoding != ENCODING_UTF8)
    CLEAN_AND_ZERO_ARR (DstWord);

  return res;
}

BOOL AspellInterface::Init (TCHAR *PathArg)
{
  return (AspellLoaded = LoadAspell (PathArg));
}

void AspellInterface::SetLanguage (TCHAR *Lang)
{
  if (!AspellLoaded)
    return;

  AspellConfig *spell_config = new_aspell_config();
  aspell_config_replace (spell_config, "encoding", "utf-8");
  char *Buf = 0;
  SetString (Buf, Lang);
  aspell_config_replace(spell_config, "lang", Buf);
  CLEAN_AND_ZERO_ARR (Buf);
  if (SingularSpeller)
  {
    delete_aspell_speller (SingularSpeller);
    SingularSpeller = 0;
  }

  AspellCanHaveError *PossibleErr = new_aspell_speller (spell_config);

  if (aspell_error_number(PossibleErr) != 0)
  {
    delete_aspell_config (spell_config);
    std::vector<TCHAR *> *LangList = GetLanguageList ();
    if (LangList && (!Lang || _tcscmp (Lang, LangList->at (0)) != 0))
    {
      SetLanguage (LangList->at (0));
      CLEAN_AND_ZERO_STRING_VECTOR (LangList);
      return;
    }
    else
    {
      SingularSpeller = 0;
      SendAspellErorr (PossibleErr);
      return;
    }
  }
  else
    SingularSpeller = to_aspell_speller(PossibleErr);

  delete_aspell_config (spell_config);
}