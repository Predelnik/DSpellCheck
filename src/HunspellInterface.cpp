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
// To avoid problems with windows.h and hunspell
#ifdef near
#undef near
#endif // near
#define HUNSPELL_STATIC // We're using static build so ...
#include "CommonFunctions.h"
#include "hunspell/hunspell.hxx"
#include "HunspellInterface.h"

#include <locale>

static BOOL ListFiles(TCHAR *path, TCHAR *mask, std::vector<TCHAR *>& files, TCHAR *Filter)
{
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA ffd;
  TCHAR *spec = 0;
  std::stack<TCHAR *> *directories = new std::stack<TCHAR *>;

  directories->push(path);
  files.clear();

  while (!directories->empty()) {
    path = directories->top();
    CLEAN_AND_ZERO_ARR (spec);
    spec = new TCHAR [_tcslen (path) + 1 + _tcslen (mask) + 1];
    _tcscpy (spec, path);
    _tcscat (spec, _T ("\\"));
    _tcscat (spec, mask);
    directories->pop();

    hFind = FindFirstFile(spec, &ffd);
    if (hFind == INVALID_HANDLE_VALUE)  {
      return FALSE;
    }

    do {
      if (wcscmp(ffd.cFileName, L".") != 0 &&
        wcscmp(ffd.cFileName, L"..") != 0) {
          TCHAR *buf = new TCHAR [_tcslen (path) + 1 + _tcslen (ffd.cFileName) + 1];
          _tcscpy (buf, path );
          _tcscat (buf, _T ("\\"));
          _tcscat (buf, ffd.cFileName);
          if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            directories->push(buf);
          }
          else {
            if (PathMatchSpec (buf, Filter))
              files.push_back(buf);
            else
              CLEAN_AND_ZERO_ARR (buf);
          }
      }
    } while (FindNextFile(hFind, &ffd) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
      CLEAN_AND_ZERO_STRING_STACK (directories);
      FindClose(hFind);
      return FALSE;
    }

    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;
  }

  CLEAN_AND_ZERO_ARR (spec);
  CLEAN_AND_ZERO_STRING_STACK (directories);

  return TRUE;
}

HunspellInterface::HunspellInterface ()
{
  DicList = new std::vector <TCHAR *>;
  Spellers = new std::vector<DicInfo>;
  memset (&Empty, 0, sizeof (Empty));
  Ignored = new std::set <char *, bool (*) (char *, char *)> (SortCompareChars);
  Memorized = new std::set <char *, bool (*) (char *, char *)> (SortCompareChars);
  SingularSpeller = Empty;
  DicDir = 0;
  LastSelectedSpeller = Empty;
  AllHunspells = new std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)> (SortCompare);
  IsHunspellWorking = FALSE;
  TemporaryBuffer = new char[DEFAULT_BUF_SIZE];
  UserDicPath = 0;
  InitialReadingBeenDone = FALSE;
}

HunspellInterface::~HunspellInterface ()
{
  IsHunspellWorking = FALSE;
  {
    std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)>::iterator it = AllHunspells->begin ();
    for (; it != AllHunspells->end (); ++it)
    {
      delete []((*it).first);
      CLEAN_AND_ZERO ((*it).second.Speller);
      iconv_close ((*it).second.Converter);
      iconv_close ((*it).second.BackConverter);
      iconv_close ((*it).second.ConverterANSI);
      iconv_close ((*it).second.BackConverterANSI);
    }
    CLEAN_AND_ZERO (AllHunspells);
  }

  WriteUserDic ();

  {
    std::set <char *, bool (*)(char *, char *)>::iterator it = Ignored->begin ();
    for (; it != Ignored->end (); ++it)
    {
      delete [] (*it);
    }
    CLEAN_AND_ZERO (Ignored);
  }

  {
    std::set <char *, bool (*)(char *, char *)>::iterator it = Memorized->begin ();
    for (; it != Memorized->end (); ++it)
    {
      delete [] (*it);
    }
    CLEAN_AND_ZERO (Memorized);
  }

  CLEAN_AND_ZERO (Spellers);
  CLEAN_AND_ZERO_STRING_VECTOR (DicList);
  CLEAN_AND_ZERO_ARR (DicDir);
  CLEAN_AND_ZERO_ARR (TemporaryBuffer);
  CLEAN_AND_ZERO_ARR (UserDicPath);
}

std::vector<TCHAR*> *HunspellInterface::GetLanguageList ()
{
  std::vector<TCHAR *> *List = new std::vector<TCHAR *>;
  // Just copying vector
  for (unsigned int i = 0; i < DicList->size (); i++)
  {
    TCHAR *Buf = 0;
    SetString (Buf, DicList->at (i));
    List->push_back (Buf);
  }
  return List;
}

DicInfo HunspellInterface::CreateHunspell (TCHAR *Name)
{
  {
    std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)>::iterator it = AllHunspells->find (Name);
    if (it != AllHunspells->end ())
      return (*it).second;
  }

  int size = _tcslen (DicDir) + 1 + _tcslen (Name) + 1 + 3 + 1; // + . + aff/dic + /0
  TCHAR *AffBuf = new TCHAR [size];
  TCHAR *DicBuf = new TCHAR [size];
  char *AffBufAnsi = 0;
  char *DicBufAnsi = 0;
  _tcscpy (AffBuf, DicDir);
  _tcscat (AffBuf, _T ("\\"));
  _tcscat (AffBuf, Name);
  _tcscat (AffBuf, _T (".aff"));
  _tcscpy (DicBuf, DicDir);
  _tcscat (DicBuf, _T ("\\"));
  _tcscat (DicBuf, Name);
  _tcscat (DicBuf, _T (".dic"));
  SetString (AffBufAnsi, AffBuf);
  SetString (DicBufAnsi, DicBuf);
  CLEAN_AND_ZERO_ARR (AffBuf);
  CLEAN_AND_ZERO_ARR (DicBuf);

  Hunspell *NewHunspell = new Hunspell (AffBufAnsi, DicBufAnsi);
  TCHAR *NewName = 0;
  SetString (NewName, Name);
  DicInfo NewDic;
  NewDic.Converter = iconv_open (NewHunspell->get_dic_encoding (), "UTF-8");
  NewDic.BackConverter = iconv_open ("UTF-8", NewHunspell->get_dic_encoding ());
  NewDic.ConverterANSI = iconv_open (NewHunspell->get_dic_encoding (), "");
  NewDic.BackConverterANSI = iconv_open ("", NewHunspell->get_dic_encoding ());
  NewDic.Speller = NewHunspell;
  {
    std::set <char *, bool (*)(char *, char *)>::iterator it = Memorized->begin ();
    for (; it != Memorized->end (); ++it)
    {
      char *ConvWord = GetConvertedWord (*it, NewDic.Converter);
      if (*ConvWord)
        NewHunspell->add (ConvWord); // Adding all already memorized words to newly loaded Hunspell instance
    }
  }
  (*AllHunspells)[NewName] = NewDic;
  CLEAN_AND_ZERO_ARR (AffBufAnsi);
  CLEAN_AND_ZERO_ARR (DicBufAnsi);
  return NewDic;
}

void HunspellInterface::SetLanguage (TCHAR *Lang)
{
  if (DicList->size () == 0)
  {
    SingularSpeller = Empty;
    return;
  }

  if (!std::binary_search (DicList->begin (), DicList->end (), Lang,  SortCompare))
  {
    Lang = DicList->at (0);
  }
  SingularSpeller = CreateHunspell (Lang);
}

void HunspellInterface::SetMultipleLanguages (std::vector<TCHAR *> *List)
{
  if (DicList->size () == 0)
    return;

  Spellers->clear ();
  for (unsigned int i = 0; i < List->size (); i++)
  {
    if (!std::binary_search (DicList->begin (), DicList->end (), List->at (i),  SortCompare))
      continue;
    DicInfo Instance = CreateHunspell (List->at (i));
    Spellers->push_back (Instance);
  }
}

char *HunspellInterface::GetConvertedWord (const char *Source, iconv_t Converter)
{
  size_t InSize = strlen (Source) + 1;
  size_t OutSize = DEFAULT_BUF_SIZE;
  char *OutBuf = TemporaryBuffer;
  size_t res = iconv (Converter, &Source, &InSize, &OutBuf, &OutSize);
  if (res == (size_t)(-1))
  {
    *TemporaryBuffer = '\0';
  }
  return TemporaryBuffer;
}

BOOL HunspellInterface::SpellerCheckWord (DicInfo Dic, char *Word, EncodingType Encoding)
{
  char *WordToCheck = GetConvertedWord (Word, Encoding == (ENCODING_UTF8) ? Dic.Converter : Dic.ConverterANSI);
  if (!*WordToCheck)
    return FALSE;

  // No additional check for memorized is needed since all words are already in dictionary

  return Dic.Speller->spell (WordToCheck);
}

BOOL HunspellInterface::CheckWord (char *Word)
{
  if (Memorized->find (Word) != Memorized->end ()) // This check is for dictionaries which are in other than utf-8 encoding
    return TRUE;                                   // Though it slows down stuff a little, I hope it will do

  if (Ignored->find (Word) != Ignored->end ())
    return TRUE;

  BOOL res = FALSE;
  unsigned int Len = strlen (Word);
  if (!MultiMode)
  {
    if (SingularSpeller.Speller)
      res = SpellerCheckWord (SingularSpeller, Word, CurrentEncoding);
    else
      res = TRUE;
  }
  else
  {
    if (!Spellers || Spellers->size () == 0)
      return TRUE;

    for (int i = 0; i < (int )Spellers->size () && !res; i++)
    {
      res = res || SpellerCheckWord (Spellers->at (i), Word, CurrentEncoding);
    }
  }
  return res;
}

void HunspellInterface::WriteUserDic ()
{
  FILE *Fp = 0;
  if (!InitialReadingBeenDone)
    return;
  Fp = _tfopen (UserDicPath, _T ("w"));
  if (!Fp)
    return;
  {
    std::set <char *, bool (*)(char *, char *)>::iterator it = Memorized->begin ();
    fprintf (Fp, "%d\n", Memorized->size ());
    for (; it != Memorized->end (); ++it)
    {
      fprintf (Fp, "%s\n", *it);
    }
  }

  fclose (Fp);
}

void HunspellInterface::ReadUserDic ()
{
  FILE *Fp = 0;
  int WordNum = 0;
  Fp = _tfopen (UserDicPath, _T ("r"));
  if (!Fp)
  {
    InitialReadingBeenDone = TRUE;
    return;
  }
  {
    char Buf [DEFAULT_BUF_SIZE];
    if (fscanf (Fp, "%d\n", &WordNum) != 1)
    {
      InitialReadingBeenDone = TRUE;
      return;
    }
    for (int i = 0; i < WordNum; i++)
    {
      if (fgets (Buf, DEFAULT_BUF_SIZE, Fp))
      {
        Buf[strlen (Buf) - 1] = 0;
        if (Memorized->find (Buf) == Memorized->end ())
        {
          char *Word = 0;
          SetString (Word, Buf);
          Memorized->insert (Word);
        }
      }
    }
  }

  fclose (Fp);
  InitialReadingBeenDone = TRUE;
}

void HunspellInterface::AddToDictionary (char *Word)
{
  if (!LastSelectedSpeller.Speller)
    return;

  char *Buf = 0;
  if (CurrentEncoding == ENCODING_UTF8)
    SetString (Buf, Word);
  else
    SetStringDUtf8 (Buf, Word);
  Memorized->insert (Buf);
  std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)>::iterator it;
  it = AllHunspells->begin ();
  for (; it != AllHunspells->end (); ++it)
  {
    char *ConvWord = GetConvertedWord (Buf, (*it).second.Converter);
    if (*ConvWord)
      (*it).second.Speller->add (ConvWord);

    // Adding word to all currently loaded dictionaries and in memorized list to save it.
  }
}

void HunspellInterface::IgnoreAll (char *Word)
{
  if (!LastSelectedSpeller.Speller)
    return;

  char *Buf = 0;
  SetString (Buf, Word);
  Ignored->insert (Buf);
}

std::vector<char *> *HunspellInterface::GetSuggestions (char *Word)
{
  std::vector<char *> *SuggList = new std::vector<char *>;
  int Num = -1;
  int CurNum;
  char **HunspellList = 0;
  char **CurHunspellList = 0;
  char *WordUtf8 = 0;
  LastSelectedSpeller = SingularSpeller;

  if (!MultiMode)
  {
    Num = SingularSpeller.Speller->suggest (&HunspellList, GetConvertedWord (Word, (CurrentEncoding == ENCODING_UTF8) ? SingularSpeller.Converter : SingularSpeller.ConverterANSI));
  }
  else
  {
    int MaxSize = -1;
    // In this mode we're finding maximum by length list from selected languages
    CurHunspellList = 0;
    for (int i = 0; i < (int) Spellers->size (); i++)
    {
      CurNum = Spellers->at (i).Speller->suggest (&CurHunspellList, GetConvertedWord (Word, (CurrentEncoding == ENCODING_UTF8) ? Spellers->at (i).Converter : Spellers->at (i).ConverterANSI));

      if (CurNum > 0)
      {
        const char *FirstSug = GetConvertedWord (CurHunspellList [0], Spellers->at (i).BackConverter);
        SetStringDUtf8 (WordUtf8, Word);
        if (Utf8GetCharSize (*FirstSug) != Utf8GetCharSize (*WordUtf8))
          continue; // Special Hack to distinguish Cyrillic words from ones written Latin letters
      }

      if (CurNum > MaxSize)
      {
        if (Num != -1)
          Spellers->at (i).Speller->free_list (&HunspellList, Num);

        MaxSize = CurNum;
        LastSelectedSpeller = Spellers->at (i);
        HunspellList = CurHunspellList;
        Num = CurNum;
      }
      else
      {
        Spellers->at (i).Speller->free_list (&CurHunspellList, CurNum);
      }
    }
  }

  TCHAR *Buf = 0;
  int Counter = 0;

  for (int i = 0; i < Num; i++)
  {
    char *Buf = 0;
    SetString (Buf, GetConvertedWord (HunspellList[i], LastSelectedSpeller.BackConverter));
    SuggList->push_back (Buf);
  }

  LastSelectedSpeller.Speller->free_list (&HunspellList, Num);

  CLEAN_AND_ZERO_ARR (WordUtf8);

  return SuggList;
}

void HunspellInterface::SetDirectory (TCHAR *Dir)
{
  std::vector<TCHAR *> *FileList = new std::vector<TCHAR *>;
  SetString (DicDir, Dir);

  BOOL Res = ListFiles (Dir, _T ("*.*"), *FileList, _T ("*.aff"));
  if (!Res)
  {
    CLEAN_AND_ZERO_STRING_VECTOR (FileList);
    return;
  }

  CLEAN_AND_ZERO_STRING_VECTOR (DicList);
  DicList = new std::vector <TCHAR *>;

  for (unsigned int i = 0; i < FileList->size (); i++)
  {
    TCHAR *Buf = 0;
    SetString (Buf, FileList->at (i));
    TCHAR *DotPointer = _tcsrchr (Buf, _T ('.'));
    _tcscpy (DotPointer, _T (".dic"));
    if (PathFileExists (Buf))
    {
      *DotPointer = 0;
      TCHAR *SlashPointer = _tcsrchr (Buf, _T ('\\'));
      TCHAR *TBuf = 0;
      SetString (TBuf, SlashPointer + 1);
      DicList->push_back (TBuf);
    }
    CLEAN_AND_ZERO_ARR (Buf);
  }

  IsHunspellWorking = (DicList->size () > 0);

  std::sort (DicList->begin (), DicList->end (), SortCompare);
  CLEAN_AND_ZERO_ARR (UserDicPath);
  UserDicPath = new TCHAR [DEFAULT_BUF_SIZE];
  _tcscpy (UserDicPath, Dir);
  if (UserDicPath[_tcslen (UserDicPath) - 1] != _T ('\\'))
    _tcscat (UserDicPath, _T ("\\"));
  _tcscat (UserDicPath, _T ("UserDic.dic")); // Should be tunable really
  ReadUserDic (); // We should load user dictionary first.
  CLEAN_AND_ZERO_STRING_VECTOR (FileList);
}

BOOL HunspellInterface::IsWorking ()
{
  return IsHunspellWorking;
}