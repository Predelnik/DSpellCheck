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
#include <io.h>
#include <fcntl.h>
#include <unordered_set>

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
  DicList = new std::set <AvailableLangInfo>;
  Spellers = new std::vector<DicInfo>;
  memset (&Empty, 0, sizeof (Empty));
  Ignored = new WordSet (0, HashCharString, EquivCharStrings);
  Memorized = new WordSet (0, HashCharString, EquivCharStrings);
  SingularSpeller = Empty;
  DicDir = 0;
  SysDicDir = 0;
  LastSelectedSpeller = Empty;
  AllHunspells = new std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)> (SortCompare);
  IsHunspellWorking = FALSE;
  TemporaryBuffer = new char[DEFAULT_BUF_SIZE];
  UserDicPath = 0;
}

void HunspellInterface::UpdateOnDicRemoval (TCHAR *Path)
{
  std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)>::iterator it = AllHunspells->find (Path);
  if (it != AllHunspells->end ())
  {
    delete []((*it).first);
    CLEAN_AND_ZERO ((*it).second.Speller);
    WriteUserDic ((*it).second.LocalDic, (*it).second.LocalDicPath);
    CLEAN_AND_ZERO ((*it).second.LocalDicPath)
      iconv_close ((*it).second.Converter);
    iconv_close ((*it).second.BackConverter);
    iconv_close ((*it).second.ConverterANSI);
    iconv_close ((*it).second.BackConverterANSI);
    AllHunspells->erase (it);
  }
}

static void CleanAndZeroWordList (WordSet *&WordListInstance)
{
  WordSet::iterator it = WordListInstance->begin ();
  for (; it != WordListInstance->end (); ++it)
  {
    delete [] (*it);
  }
  CLEAN_AND_ZERO (WordListInstance);
}

void HunspellInterface::SetUseOneDic (BOOL Value)
{
  UseOneDic = Value;
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
      WriteUserDic ((*it).second.LocalDic, (*it).second.LocalDicPath);
      CLEAN_AND_ZERO ((*it).second.LocalDicPath)
        iconv_close ((*it).second.Converter);
      iconv_close ((*it).second.BackConverter);
      iconv_close ((*it).second.ConverterANSI);
      iconv_close ((*it).second.BackConverterANSI);
    }
    CLEAN_AND_ZERO (AllHunspells);
  }

  if (InitialReadingBeenDone)
    WriteUserDic (Memorized, UserDicPath);

  CleanAndZeroWordList (Memorized);
  CleanAndZeroWordList (Ignored);

  CLEAN_AND_ZERO (Spellers);
  CLEAN_AND_ZERO_ARR (DicDir);
  CLEAN_AND_ZERO_ARR (SysDicDir);
  CLEAN_AND_ZERO_ARR (TemporaryBuffer);
  CLEAN_AND_ZERO_ARR (UserDicPath);

  std::set <AvailableLangInfo>::iterator it;
  it = DicList->begin ();
  for (; it != DicList->end (); ++it)
  {
    delete [] ((*it).Name);
  }
  CLEAN_AND_ZERO (DicList);
}

std::vector<TCHAR*> *HunspellInterface::GetLanguageList ()
{
  std::vector<TCHAR *> *List = new std::vector<TCHAR *>;
  // Just copying vector
  std::set <AvailableLangInfo>::iterator it = DicList->begin ();
  for (; it != DicList->end (); ++it)
  {
    TCHAR *Buf = 0;
    SetString (Buf, (*it).Name);
    List->push_back (Buf);
  }
  return List;
}

DicInfo HunspellInterface::CreateHunspell (TCHAR *Name, int Type)
{
  int size = (Type ? _tcslen (SysDicDir) : _tcslen (DicDir)) + 1 + _tcslen (Name) + 1 + 3 + 1; // + . + aff/dic + /0
  TCHAR *AffBuf = new TCHAR [size];
  char *AffBufAnsi = 0;
  char *DicBufAnsi = 0;
  _tcscpy (AffBuf, (Type ? SysDicDir : DicDir));
  _tcscat (AffBuf, _T ("\\"));
  _tcscat (AffBuf, Name);
  {
    std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)>::iterator it = AllHunspells->find (AffBuf);
    if (it != AllHunspells->end ())
    {
      CLEAN_AND_ZERO_ARR (AffBuf);
      return (*it).second;
    }
  }
  TCHAR *DicBuf = new TCHAR [size];
  _tcscat (AffBuf, _T (".aff"));
  _tcscpy (DicBuf, Type ? SysDicDir : DicDir);
  _tcscat (DicBuf, _T ("\\"));
  _tcscat (DicBuf, Name);
  _tcscat (DicBuf, _T (".dic"));
  SetString (AffBufAnsi, AffBuf);
  SetString (DicBufAnsi, DicBuf);

  Hunspell *NewHunspell = new Hunspell (AffBufAnsi, DicBufAnsi);
  TCHAR *NewName = 0;
  AffBuf[_tcslen (AffBuf) - 4] = _T ('\0');
  SetString (NewName, AffBuf); // Without aff and dic
  DicInfo NewDic;
  NewDic.Converter = iconv_open (NewHunspell->get_dic_encoding (), "UTF-8");
  NewDic.BackConverter = iconv_open ("UTF-8", NewHunspell->get_dic_encoding ());
  NewDic.ConverterANSI = iconv_open (NewHunspell->get_dic_encoding (), "");
  NewDic.BackConverterANSI = iconv_open ("", NewHunspell->get_dic_encoding ());
  NewDic.LocalDic = new WordSet (0, HashCharString, EquivCharStrings);
  NewDic.LocalDicPath = new TCHAR [_tcslen (DicDir) + 1 + _tcslen (Name) + 1 + 3 + 1]; // Local Dic path always points to non-system directory
  NewDic.LocalDicPath[0] = '\0';
  _tcscat (NewDic.LocalDicPath, DicDir);
  _tcscat (NewDic.LocalDicPath, _T ("\\"));
  _tcscat (NewDic.LocalDicPath,  Name);
  _tcscat (NewDic.LocalDicPath,  _T (".usr"));

  ReadUserDic (NewDic.LocalDic, NewDic.LocalDicPath);
  NewDic.Speller = NewHunspell;
  {
    WordSet::iterator it = Memorized->begin ();
    for (; it != Memorized->end (); ++it)
    {
      char *ConvWord = GetConvertedWord (*it, NewDic.Converter);
      if (*ConvWord)
        NewHunspell->add (ConvWord); // Adding all already memorized words to newly loaded Hunspell instance
    }
  }
  {
    WordSet::iterator it = NewDic.LocalDic->begin ();
    for (; it != NewDic.LocalDic->end (); ++it)
    {
      NewHunspell->add (*it); // Adding all already memorized words from local dictionary to Hunspell instance, local dictionaries are in dictionary encoding
    }
  }
  (*AllHunspells)[NewName] = NewDic;
  CLEAN_AND_ZERO_ARR (AffBuf);
  CLEAN_AND_ZERO_ARR (DicBuf);
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
  AvailableLangInfo Temp;
  Temp.Name = Lang;
  Temp.Type = 0;
  std::set <AvailableLangInfo>::iterator SearchResult = DicList->find (Temp);
  if (SearchResult == DicList->end ())
  {
    SearchResult = DicList->begin ();
  }
  SingularSpeller = CreateHunspell ((*SearchResult).Name, (*SearchResult).Type);
}

void HunspellInterface::SetMultipleLanguages (std::vector<TCHAR *> *List)
{
  Spellers->clear ();

  if (DicList->size () == 0)
    return;

  for (unsigned int i = 0; i < List->size (); i++)
  {
    AvailableLangInfo Temp;
    Temp.Name = List->at (i);
    Temp.Type = 0;
    std::set <AvailableLangInfo>::iterator SearchResult = DicList->find (Temp);
    if (SearchResult == DicList->end ())
      continue;
    DicInfo Instance = CreateHunspell ((*SearchResult).Name, (*SearchResult).Type);
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
  /*
  if (Memorized->find (Word) != Memorized->end ()) // This check is for dictionaries which are in other than utf-8 encoding
  return TRUE;                                   // Though it slows down stuff a little, I hope it will do
  */

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

void HunspellInterface::WriteUserDic (WordSet *Target, TCHAR *Path)
{
  FILE *Fp = 0;

  if (Target->size () == 0)
  {
    SetFileAttributes (Path, FILE_ATTRIBUTE_NORMAL);
    DeleteFile (Path);
    return;
  }

  SortedWordSet TemporarySortedWordSet (SortCompareChars); // Wordset itself being removed here as local variable, all strings are not copied

  Fp = _tfopen (Path, _T ("w"));
  if (!Fp)
    return;
  {
    WordSet::iterator it = Target->begin ();
    fprintf (Fp, "%d\n", Target->size ());
    for (; it != Target->end (); ++it)
    {
      TemporarySortedWordSet.insert (*it);
    }
  }
  SortedWordSet::iterator it = TemporarySortedWordSet.begin ();
  for (; it != TemporarySortedWordSet.end (); ++it)
  {
    fprintf (Fp, "%s\n", *it);
  }

  fclose (Fp);
}

void HunspellInterface::ReadUserDic (WordSet *Target, TCHAR *Path)
{
  FILE *Fp = 0;
  int WordNum = 0;
  Fp = _tfopen (Path, _T ("r"));
  if (!Fp)
  {
    return;
  }
  {
    char Buf [DEFAULT_BUF_SIZE];
    if (fscanf (Fp, "%d\n", &WordNum) != 1)
    {
      return;
    }
    for (int i = 0; i < WordNum; i++)
    {
      if (fgets (Buf, DEFAULT_BUF_SIZE, Fp))
      {
        Buf[strlen (Buf) - 1] = 0;
        if (Target->find (Buf) == Target->end ())
        {
          char *Word = 0;
          SetString (Word, Buf);
          Target->insert (Word);
        }
      }
      else
        break;
    }
  }

  fclose (Fp);
}

static void MessageBoxWordCannotBeAdded ()
{
  MessageBox (0, _T ("Sadly, this word contains symbols out of current dictionary encoding, thus it cannot be added to user dictionary. You can convert this dictionary to UTF-8 or choose the different one with appropriate encoding."), _T ("Word cannot be added"), MB_OK | MB_ICONWARNING);
}

void HunspellInterface::AddToDictionary (char *Word)
{
  if (!LastSelectedSpeller.Speller)
    return;

  TCHAR *DicPath = 0;
  if (UseOneDic)
    DicPath = UserDicPath;
  else
    DicPath = LastSelectedSpeller.LocalDicPath;

  if (!PathFileExists (DicPath))
  {
    // If there's no life then we're checking if we can create it, there's no harm in it
    int LocalDicFileHandle = _topen (DicPath, _O_CREAT | _O_BINARY | _O_WRONLY);
    if (LocalDicFileHandle == -1)
    {
      MessageBox (0, _T ("User dictionary cannot be written, please check if you have access for writing into your dictionary directory, otherwise you can change it or run Notepad++ as administrator."), _T ("User dictionary cannot be saved"), MB_OK | MB_ICONWARNING);
    }
    else
      _close (LocalDicFileHandle);
  }
  else
  {
    // Otherwise we're using _taccess to check for permission to write there.
    SetFileAttributes (DicPath, FILE_ATTRIBUTE_NORMAL);
    int LocalDicFileHandle = _topen (DicPath, _O_APPEND | _O_BINARY | _O_WRONLY);
    if (LocalDicFileHandle == -1)
    {
      MessageBox (0, _T ("User dictionary cannot be written, please check if you have access for writing into your dictionary directory, otherwise you can change it or run Notepad++ as administrator."), _T ("User dictionary cannot be saved"), MB_OK | MB_ICONWARNING);
    }
    else
      _close (LocalDicFileHandle);
  }

  char *Buf = 0;
  if (CurrentEncoding == ENCODING_UTF8)
    SetString (Buf, Word);
  else
    SetStringDUtf8 (Buf, Word);

  if (UseOneDic)
  {
    std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)>::iterator it;
    Memorized->insert (Buf);
    it = AllHunspells->begin ();
    for (; it != AllHunspells->end (); ++it)
    {
      char *ConvWord = GetConvertedWord (Buf, (*it).second.Converter);
      if (*ConvWord)
        (*it).second.Speller->add (ConvWord);
      else if ((*it).second.Speller == LastSelectedSpeller.Speller)
        MessageBoxWordCannotBeAdded ();
      // Adding word to all currently loaded dictionaries and in memorized list to save it.
    }
  }
  else
  {
    char *ConvWord = GetConvertedWord (Buf, LastSelectedSpeller.Converter);
    char *WordCopy = 0;
    SetString (WordCopy, ConvWord);
    LastSelectedSpeller.LocalDic->insert (WordCopy);
    if (*ConvWord)
      LastSelectedSpeller.Speller->add (ConvWord);
    else
      MessageBoxWordCannotBeAdded ();
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
  InitialReadingBeenDone = FALSE;
  std::vector<TCHAR *> *FileList = new std::vector<TCHAR *>;
  SetString (DicDir, Dir);

  std::set <AvailableLangInfo>::iterator it;
  it = DicList->begin ();
  for (; it != DicList->end (); ++it)
  {
    delete [] ((*it).Name);
  }
  CLEAN_AND_ZERO (DicList);

  DicList = new std::set <AvailableLangInfo>;
  IsHunspellWorking = TRUE;

  BOOL Res = ListFiles (Dir, _T ("*.*"), *FileList, _T ("*.aff"));
  if (!Res)
  {
    CLEAN_AND_ZERO_STRING_VECTOR (FileList);
    return;
  }

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
      AvailableLangInfo NewX;
      NewX.Type = 0;
      NewX.Name = TBuf;
      DicList->insert (NewX);
    }
    CLEAN_AND_ZERO_ARR (Buf);
  }

  CLEAN_AND_ZERO_ARR (UserDicPath);
  UserDicPath = new TCHAR [DEFAULT_BUF_SIZE];
  _tcscpy (UserDicPath, Dir);
  if (UserDicPath[_tcslen (UserDicPath) - 1] != _T ('\\'))
    _tcscat (UserDicPath, _T ("\\"));
  _tcscat (UserDicPath, _T ("UserDic.dic")); // Should be tunable really
  ReadUserDic (Memorized, UserDicPath); // We should load user dictionary first.
  CLEAN_AND_ZERO_STRING_VECTOR (FileList);
}

void HunspellInterface::SetAdditionalDirectory (TCHAR *Dir)
{
  InitialReadingBeenDone = FALSE;
  std::vector<TCHAR *> *FileList = new std::vector<TCHAR *>;
  SetString (SysDicDir, Dir);

  if (!DicList)
    return;
  IsHunspellWorking = TRUE;

  BOOL Res = ListFiles (Dir, _T ("*.*"), *FileList, _T ("*.aff"));
  if (!Res)
  {
    CLEAN_AND_ZERO_STRING_VECTOR (FileList);
    return;
  }

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
      AvailableLangInfo NewX;
      NewX.Type = 1;
      NewX.Name = TBuf;
      if (DicList->find (NewX) == DicList->end ())
        DicList->insert (NewX);
      else
        CLEAN_AND_ZERO_ARR (TBuf);
    }
    CLEAN_AND_ZERO_ARR (Buf);
  }
  // Now we have 2 dictionaries on our hands

  InitialReadingBeenDone = TRUE;
  CLEAN_AND_ZERO_STRING_VECTOR (FileList);
}

BOOL HunspellInterface::GetLangOnlySystem (TCHAR *Lang)
{
  AvailableLangInfo Needle;
  Needle.Name = Lang;
  Needle.Type = 1;
  std::set <AvailableLangInfo>::iterator It = DicList->find (Needle);
  if (It != DicList->end () && (*It).Type == 1)
    return TRUE;
  else
    return FALSE;
}

BOOL HunspellInterface::IsWorking ()
{
  return IsHunspellWorking;
}