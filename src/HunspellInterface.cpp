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
#include "hunspell/hunspell.hxx"
#include "HunspellInterface.h"
#include "CommonFunctions.h"
#include "MainDef.h"

static BOOL ListFiles(TCHAR *path, TCHAR *mask, std::vector<TCHAR *>& files)
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
            files.push_back(buf);
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
  SingularSpeller = 0;
  DicDir = 0;
  Spellers = 0;
  LastSelectedSpeller = 0;
}

HunspellInterface::~HunspellInterface ()
{
  CLEAN_AND_ZERO (SingularSpeller);
  CLEAN_AND_ZERO_POINTER_VECTOR (Spellers);
  CLEAN_AND_ZERO_STRING_VECTOR (DicList);
  CLEAN_AND_ZERO_ARR (DicDir);
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

Hunspell *HunspellInterface::CreateHunspell (const TCHAR *Name)
{
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

  return new Hunspell (AffBufAnsi, DicBufAnsi);
  CLEAN_AND_ZERO_ARR (DicBufAnsi);
  CLEAN_AND_ZERO_ARR (DicBufAnsi);
}

void HunspellInterface::SetLanguage (TCHAR *Lang)
{
  if (DicList->size () == 0)
  {
    SingularSpeller = 0;
    return;
  }

  if (!std::binary_search (DicList->begin (), DicList->end (), Lang,  SortCompare))
  {
    Lang = DicList->at (0);
  }
  CLEAN_AND_ZERO (SingularSpeller);
  SingularSpeller = CreateHunspell (Lang);
}

void HunspellInterface::SetMultipleLanguages (std::vector<TCHAR *> *List)
{
  if (DicList->size () == 0)
    return;

  CLEAN_AND_ZERO_POINTER_VECTOR (Spellers);
  Spellers = new std::vector<Hunspell *>;
  for (unsigned int i = 0; i < List->size (); i++)
  {
    if (!std::binary_search (DicList->begin (), DicList->end (), List->at (i),  SortCompare))
      continue;
    Hunspell *Instance = CreateHunspell (List->at (i));
    Spellers->push_back (Instance);
  }
}

BOOL HunspellInterface::CheckWord (char *Word)
{
  BOOL res = FALSE;
  unsigned int Len = strlen (Word);
  if (!MultiMode)
  {
    if (SingularSpeller)
      res = SingularSpeller->spell (Word);
    else
      res = TRUE;
  }
  else
  {
    if (!Spellers || Spellers->size () == 0)
      return TRUE;

    for (int i = 0; i < (int )Spellers->size () && !res; i++)
    {
      res = res || Spellers->at (i)->spell (Word);
    }
  }
  return res;
}

void HunspellInterface::AddToDictionary (char *Word)
{
  if (!LastSelectedSpeller)
    return;
  LastSelectedSpeller->add (Word);
}

void HunspellInterface::IgnoreAll (char *Word)
{
  return; // Dummy, probably should be disabled for Hunspell or written in custom way
}

std::vector<char *> *HunspellInterface::GetSuggestions (char *Word)
{
  std::vector<char *> *SuggList = new std::vector<char *>;
  int Num;
  int CurNum;
  char **HunspellList = 0;
  char **CurHunspellList = 0;
  LastSelectedSpeller = SingularSpeller;
  if (!MultiMode)
  {
    Num = SingularSpeller->suggest (&HunspellList, Word);
  }
  else
  {
    int MaxSize = -1;
    // In this mode we're finding maximum by length list from selected languages
    CurHunspellList = 0;
    for (int i = 0; i < (int) Spellers->size (); i++)
    {
      CurNum = Spellers->at (i)->suggest (&CurHunspellList, Word);

      if (CurNum > 0)
      {
        const char *FirstSug = CurHunspellList [0];
        if (Utf8GetCharSize (*FirstSug) != Utf8GetCharSize (*Word))
          continue; // Special Hack to distinguish Cyrillic words from ones written Latin letters
      }

      if (CurNum > MaxSize)
      {
        for (int j = 0; j < CurNum; j++)
          CLEAN_AND_ZERO_ARR (CurHunspellList[j]);
        CLEAN_AND_ZERO_ARR (CurHunspellList);
        MaxSize = CurNum;
        LastSelectedSpeller = Spellers->at (i);
        HunspellList = CurHunspellList;
        Num = CurNum;
      }
      else
      {
        for (int j = 0; j < CurNum; j++)
          CLEAN_AND_ZERO_ARR (CurHunspellList[j]);
        CLEAN_AND_ZERO_ARR (CurHunspellList);
      }
    }
  }

  TCHAR *Buf = 0;
  int Counter = 0;

  for (int i = 0; i < Num; i++)
  {
    char *Buf = 0;
    SetString (Buf, HunspellList[i]);
    SuggList->push_back (Buf);
  }

  for (int i = 0; i < Num; i++)
    CLEAN_AND_ZERO_ARR (HunspellList[i]);
  CLEAN_AND_ZERO_ARR (HunspellList);

  return SuggList;
}

void HunspellInterface::SetDirectory (TCHAR *Dir)
{
  std::vector<TCHAR *> *FileList = new std::vector<TCHAR *>;
  SetString (DicDir, Dir);
  BOOL Res = ListFiles (Dir, _T ("*.aff"), *FileList);
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
  }

  std::sort (DicList->begin (), DicList->end (), SortCompare);
  CLEAN_AND_ZERO_STRING_VECTOR (FileList);
}

BOOL HunspellInterface::IsWorking ()
{
  return TRUE;
}
