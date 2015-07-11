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
#include "HunspellController.h"
#include "plugin.h"

#include <locale>
#include <io.h>
#include <fcntl.h>

static BOOL ListFiles(const wchar_t *path, wchar_t *mask, std::vector<TCHAR *>& files, TCHAR *Filter)
{
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA ffd;
  TCHAR *spec = 0;
  auto *directories = new std::stack<const TCHAR *>;
  BOOL Result = TRUE;

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
      Result = FALSE;
      goto cleanup;
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
      FindClose(hFind);
      Result = FALSE;
      goto cleanup;
    }

    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;
  }

cleanup:
  CLEAN_AND_ZERO_ARR (spec);
  CLEAN_AND_ZERO_STRING_STACK (directories);

  return Result;
}

HunspellController::HunspellController (HWND NppWindowArg)
{
  NppWindow = NppWindowArg;
  DicList = new std::set <AvailableLangInfo>;
  Spellers = new std::vector<DicInfo>;
  memset (&Empty, 0, sizeof (Empty));
  Ignored = new WordSet;
  Memorized = new WordSet;
  SingularSpeller = Empty;
  DicDir = 0;
  SysDicDir = 0;
  LastSelectedSpeller = Empty;
  AllHunspells = new std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)> (SortCompare);
  IsHunspellWorking = FALSE;
  TemporaryBuffer = new char[DEFAULT_BUF_SIZE];
  UserDicPath = 0;
  SystemWrongDicPath = 0;
}

void HunspellController::UpdateOnDicRemoval (TCHAR *Path, BOOL &NeedSingleLangReset, BOOL &NeedMultiLangReset)
{
  std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)>::iterator it = AllHunspells->find (Path);
  NeedSingleLangReset = FALSE;
  NeedMultiLangReset = FALSE;
  if (it != AllHunspells->end ())
  {
    if (SingularSpeller.Speller == (*it).second.Speller)
      NeedSingleLangReset = TRUE;

    if (Spellers)
    {
      for (unsigned int i = 0; i < Spellers->size (); i++)
        if (Spellers->at (i).Speller == (*it).second.Speller)
        {
          NeedMultiLangReset = TRUE;
        }
    }

    delete []((*it).first);
    CLEAN_AND_ZERO ((*it).second.Speller);
    WriteUserDic ((*it).second.LocalDic, (*it).second.LocalDicPath);
    CLEAN_AND_ZERO ((*it).second.LocalDicPath);
    if ((*it).second.Converter != (iconv_t) -1)
      iconv_close ((*it).second.Converter);

    if ((*it).second.BackConverter != (iconv_t) -1)
      iconv_close ((*it).second.BackConverter);

    if ((*it).second.ConverterANSI != (iconv_t) -1)
      iconv_close ((*it).second.ConverterANSI);

    if ((*it).second.BackConverterANSI != (iconv_t) -1)
      iconv_close ((*it).second.BackConverterANSI);

    CLEAN_AND_ZERO ((*it).second.LocalDic);
    CLEAN_AND_ZERO ((*it).second.LocalDicPath);
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

void HunspellController::SetUseOneDic (BOOL Value)
{
  UseOneDic = Value;
}

BOOL ArePathsEqual (TCHAR *path1, TCHAR *path2)
{
  BY_HANDLE_FILE_INFORMATION bhfi1,bhfi2;
  HANDLE h1, h2 = NULL;
  DWORD access = 0;
  DWORD share = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;

  h1 = CreateFile(path1,access,share,NULL,OPEN_EXISTING,(GetFileAttributes(path1)&FILE_ATTRIBUTE_DIRECTORY)?FILE_FLAG_BACKUP_SEMANTICS:0,NULL);
  if (INVALID_HANDLE_VALUE != h1)
  {
    if (!GetFileInformationByHandle(h1,&bhfi1)) bhfi1.dwVolumeSerialNumber = 0;
    h2 = CreateFile(path2,access,share,NULL,OPEN_EXISTING,(GetFileAttributes(path2)&FILE_ATTRIBUTE_DIRECTORY)?FILE_FLAG_BACKUP_SEMANTICS:0,NULL);
    if (!GetFileInformationByHandle(h2,&bhfi2)) bhfi2.dwVolumeSerialNumber = bhfi1.dwVolumeSerialNumber + 1;
  }
  else
    return FALSE;

  CloseHandle(h1);
  CloseHandle(h2);
  return INVALID_HANDLE_VALUE != h1 && INVALID_HANDLE_VALUE != h2
    && bhfi1.dwVolumeSerialNumber==bhfi2.dwVolumeSerialNumber
    && bhfi1.nFileIndexHigh==bhfi2.nFileIndexHigh
    && bhfi1.nFileIndexLow==bhfi2.nFileIndexLow ;
}

HunspellController::~HunspellController ()
{
  IsHunspellWorking = FALSE;
  {
    std::map <TCHAR *, DicInfo, bool (*)(TCHAR *, TCHAR *)>::iterator it = AllHunspells->begin ();
    for (; it != AllHunspells->end (); ++it)
    {
      delete []((*it).first);
      CLEAN_AND_ZERO ((*it).second.Speller);
      WriteUserDic ((*it).second.LocalDic, (*it).second.LocalDicPath);
      WordSet::iterator NestedIt;
      for (NestedIt = (*it).second.LocalDic->begin (); NestedIt != (*it).second.LocalDic->end (); ++NestedIt)
        delete[] (*NestedIt);

      CLEAN_AND_ZERO ((*it).second.LocalDic);
      CLEAN_AND_ZERO ((*it).second.LocalDicPath);
      if ((*it).second.Converter != (iconv_t) -1)
        iconv_close ((*it).second.Converter);

      if ((*it).second.BackConverter != (iconv_t) -1)
        iconv_close ((*it).second.BackConverter);

      if ((*it).second.ConverterANSI != (iconv_t) -1)
        iconv_close ((*it).second.ConverterANSI);

      if ((*it).second.BackConverterANSI != (iconv_t) -1)
        iconv_close ((*it).second.BackConverterANSI);
    }
    CLEAN_AND_ZERO (AllHunspells);
  }

  if (SystemWrongDicPath && UserDicPath && !ArePathsEqual (SystemWrongDicPath, UserDicPath))
  {
    SetFileAttributes (SystemWrongDicPath, FILE_ATTRIBUTE_NORMAL);
    DeleteFile (SystemWrongDicPath);
  }

  if (InitialReadingBeenDone && UserDicPath)
    WriteUserDic (Memorized, UserDicPath);

  CleanAndZeroWordList (Memorized);
  CleanAndZeroWordList (Ignored);

  CLEAN_AND_ZERO (Spellers);
  CLEAN_AND_ZERO_ARR (DicDir);
  CLEAN_AND_ZERO_ARR (SysDicDir);
  CLEAN_AND_ZERO_ARR (TemporaryBuffer);
  CLEAN_AND_ZERO_ARR (UserDicPath);
  CLEAN_AND_ZERO_ARR (SystemWrongDicPath);

  std::set <AvailableLangInfo>::iterator it;
  it = DicList->begin ();
  for (; it != DicList->end (); ++it)
  {
    delete [] ((*it).Name);
  }
  CLEAN_AND_ZERO (DicList);
}

std::vector<TCHAR*> *HunspellController::getLanguageList ()
{
  std::vector<TCHAR *> *List = new std::vector<TCHAR *>;
  // Just copying vector
  std::set <AvailableLangInfo>::iterator it = DicList->begin ();
  for (; it != DicList->end (); ++it)
  {
    TCHAR *Buf = 0;
    setString (Buf, (*it).Name);
    List->push_back (Buf);
  }
  return List;
}

DicInfo HunspellController::CreateHunspell (TCHAR *Name, int Type)
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
  setString (AffBufAnsi, AffBuf);
  setString (DicBufAnsi, DicBuf);

  Hunspell *NewHunspell = new Hunspell (AffBufAnsi, DicBufAnsi);
  TCHAR *NewName = 0;
  AffBuf[_tcslen (AffBuf) - 4] = _T ('\0');
  setString (NewName, AffBuf); // Without aff and dic
  DicInfo NewDic;
  char *DicEnconding = NewHunspell->get_dic_encoding ();
  if (stricmp (DicEnconding, "Microsoft-cp1251") == 0)
    DicEnconding = "cp1251"; // Queer fix for encoding which isn't being guessed correctly by libiconv TODO: Find other possible such failures
  NewDic.Converter = iconv_open (DicEnconding, "UTF-8");
  NewDic.BackConverter = iconv_open ("UTF-8", DicEnconding);
  NewDic.ConverterANSI = iconv_open (DicEnconding, "");
  NewDic.BackConverterANSI = iconv_open ("", DicEnconding);
  NewDic.LocalDic = new WordSet ();
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

void HunspellController::setLanguage (const wchar_t *Lang)
{
  if (DicList->size () == 0)
  {
    SingularSpeller = Empty;
    return;
  }
  AvailableLangInfo Temp;
  Temp.Name = nullptr;
  setString (Temp.Name, Lang);
  Temp.Type = 0;
  std::set <AvailableLangInfo>::iterator SearchResult = DicList->find (Temp);
  if (SearchResult == DicList->end ())
  {
    SearchResult = DicList->begin ();
  }
  SingularSpeller = CreateHunspell ((*SearchResult).Name, (*SearchResult).Type);
}

void HunspellController::SetMultipleLanguages (std::vector<TCHAR *> *List)
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

char *HunspellController::GetConvertedWord (const char *Source, iconv_t Converter)
{
  if (Converter == iconv_t (-1))
  {
    *TemporaryBuffer = '\0';
    return TemporaryBuffer;
  }
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

BOOL HunspellController::SpellerCheckWord (DicInfo Dic, char *Word, EncodingType Encoding)
{
  char *WordToCheck = GetConvertedWord (Word, Encoding == (ENCODING_UTF8) ? Dic.Converter : Dic.ConverterANSI);
  if (!*WordToCheck)
    return FALSE;

  // No additional check for memorized is needed since all words are already in dictionary

  return Dic.Speller->spell (WordToCheck);
}

BOOL HunspellController::CheckWord (char *Word)
{
  /*
  if (Memorized->find (Word) != Memorized->end ()) // This check is for dictionaries which are in other than utf-8 encoding
  return TRUE;                                   // Though it slows down stuff a little, I hope it will do
  */

  if (Ignored->find (Word) != Ignored->end ())
    return TRUE;

  BOOL res = FALSE;
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

void HunspellController::WriteUserDic (WordSet *Target, TCHAR *Path)
{
  FILE *Fp = 0;

  if (Target->size () == 0)
  {
    SetFileAttributes (Path, FILE_ATTRIBUTE_NORMAL);
    DeleteFile (Path);
    return;
  }

  SortedWordSet TemporarySortedWordSet (SortCompareChars); // Wordset itself being removed here as local variable, all strings are not copied

  TCHAR *LastSlashPos = GetLastSlashPosition (Path);
  if (!LastSlashPos)
    return;
  *LastSlashPos = _T ('\0');
  CheckForDirectoryExistence (Path);
  *LastSlashPos = _T ('\\');

  SetFileAttributes (Path, FILE_ATTRIBUTE_NORMAL);

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

void HunspellController::ReadUserDic (WordSet *Target, TCHAR *Path)
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
          setString (Word, Buf);
          Target->insert (Word);
        }
      }
      else
        break;
    }
  }

  fclose (Fp);
}

void HunspellController::MessageBoxWordCannotBeAdded ()
{
  MessageBoxInfo MsgBox (NppWindow, _T ("Sadly, this word contains symbols out of current dictionary encoding, thus it cannot be added to user dictionary. You can convert this dictionary to UTF-8 or choose the different one with appropriate encoding."), _T ("Word cannot be added"), MB_OK | MB_ICONWARNING);
  SendMessage (NppWindow, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
}

void HunspellController::AddToDictionary (char *Word)
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
    TCHAR *LastSlashPos = GetLastSlashPosition (DicPath);
    if (!LastSlashPos)
      return;
    *LastSlashPos = _T ('\0');
    CheckForDirectoryExistence (DicPath);
    *LastSlashPos = _T ('\\');
    // If there's no file then we're checking if we can create it, there's no harm in it
    int LocalDicFileHandle = _topen (DicPath, _O_CREAT | _O_BINARY | _O_WRONLY);
    if (LocalDicFileHandle == -1)
    {
      MessageBoxInfo MsgBox (NppWindow, _T ("User dictionary cannot be written, please check if you have access for writing into your dictionary directory, otherwise you can change it or run Notepad++ as administrator."), _T ("User dictionary cannot be saved"), MB_OK | MB_ICONWARNING);
      SendMessage (NppWindow, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
    }
    else
    {
      _close (LocalDicFileHandle);
    }
    SetFileAttributes (DicPath, FILE_ATTRIBUTE_NORMAL);
  }
  else
  {
    SetFileAttributes (DicPath, FILE_ATTRIBUTE_NORMAL);
    int LocalDicFileHandle = _topen (DicPath, _O_APPEND | _O_BINARY | _O_WRONLY);
    if (LocalDicFileHandle == -1)
    {
      MessageBoxInfo MsgBox (NppWindow, _T ("User dictionary cannot be written, please check if you have access for writing into your dictionary directory, otherwise you can change it or run Notepad++ as administrator."), _T ("User dictionary cannot be saved"), MB_OK | MB_ICONWARNING);
      SendMessage (NppWindow, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
    }
    else
      _close (LocalDicFileHandle);
  }

  char *Buf = 0;
  if (CurrentEncoding == ENCODING_UTF8)
    setString (Buf, Word);
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
    setString (WordCopy, ConvWord);
    LastSelectedSpeller.LocalDic->insert (WordCopy);
    if (*ConvWord)
      LastSelectedSpeller.Speller->add (ConvWord);
    else
      MessageBoxWordCannotBeAdded ();
  }
}

void HunspellController::IgnoreAll (char *Word)
{
  if (!LastSelectedSpeller.Speller)
    return;

  char *Buf = 0;
  setString (Buf, Word);
  Ignored->insert (Buf);
}

std::vector<char *> *HunspellController::GetSuggestions (char *Word)
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

  for (int i = 0; i < Num; i++)
  {
    char *Buf = 0;
    setString (Buf, GetConvertedWord (HunspellList[i], LastSelectedSpeller.BackConverter));
    SuggList->push_back (Buf);
  }

  LastSelectedSpeller.Speller->free_list (&HunspellList, Num);

  CLEAN_AND_ZERO_ARR (WordUtf8);

  return SuggList;
}

void HunspellController::setPath(const wchar_t *dir)
{
  CLEAN_AND_ZERO_ARR (UserDicPath);
  UserDicPath = new TCHAR [DEFAULT_BUF_SIZE];
  _tcscpy (UserDicPath, dir);
  if (UserDicPath[_tcslen (UserDicPath) - 1] != _T ('\\'))
    _tcscat (UserDicPath, _T ("\\"));
  _tcscat (UserDicPath, _T ("UserDic.dic")); // Should be tunable really
  ReadUserDic (Memorized, UserDicPath); // We should load user dictionary first.

  InitialReadingBeenDone = FALSE;
  std::vector<TCHAR *> *FileList = new std::vector<TCHAR *>;
  setString (DicDir, dir);

  std::set <AvailableLangInfo>::iterator it;
  it = DicList->begin ();
  for (; it != DicList->end (); ++it)
  {
    delete [] ((*it).Name);
  }
  CLEAN_AND_ZERO (DicList);

  DicList = new std::set <AvailableLangInfo>;
  IsHunspellWorking = TRUE;

  BOOL Res = ListFiles (dir, _T ("*.*"), *FileList, _T ("*.aff"));
  if (!Res)
  {
    CLEAN_AND_ZERO_STRING_VECTOR (FileList);
    return;
  }

  for (unsigned int i = 0; i < FileList->size (); i++)
  {
    TCHAR *Buf = 0;
    setString (Buf, FileList->at (i));
    TCHAR *DotPointer = _tcsrchr (Buf, _T ('.'));
    _tcscpy (DotPointer, _T (".dic"));
    if (PathFileExists (Buf))
    {
      *DotPointer = 0;
      TCHAR *SlashPointer = _tcsrchr (Buf, _T ('\\'));
      TCHAR *TBuf = 0;
      setString (TBuf, SlashPointer + 1);
      AvailableLangInfo NewX;
      NewX.Type = 0;
      NewX.Name = TBuf;
      DicList->insert (NewX);
    }
    CLEAN_AND_ZERO_ARR (Buf);
  }

  CLEAN_AND_ZERO_STRING_VECTOR (FileList);
}

void HunspellController::setAdditionalPath(const wchar_t *dir)
{
  InitialReadingBeenDone = FALSE;
  std::vector<TCHAR *> *FileList = new std::vector<TCHAR *>;
  setString (SysDicDir, dir);

  if (!DicList)
    return;
  IsHunspellWorking = TRUE;

  BOOL Res = ListFiles (dir, _T ("*.*"), *FileList, _T ("*.aff"));
  if (!Res)
  {
    CLEAN_AND_ZERO_STRING_VECTOR (FileList);
    return;
  }

  for (unsigned int i = 0; i < FileList->size (); i++)
  {
    TCHAR *Buf = 0;
    setString (Buf, FileList->at (i));
    TCHAR *DotPointer = _tcsrchr (Buf, _T ('.'));
    _tcscpy (DotPointer, _T (".dic"));
    if (PathFileExists (Buf))
    {
      *DotPointer = 0;
      TCHAR *SlashPointer = _tcsrchr (Buf, _T ('\\'));
      TCHAR *TBuf = 0;
      setString (TBuf, SlashPointer + 1);
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

  CLEAN_AND_ZERO_ARR (SystemWrongDicPath);
  SystemWrongDicPath = new TCHAR[DEFAULT_BUF_SIZE]; // Reading system path unified dic too
  _tcscpy (SystemWrongDicPath, dir);
  if (SystemWrongDicPath[_tcslen (SystemWrongDicPath) - 1] != _T ('\\'))
    _tcscat (SystemWrongDicPath, _T ("\\"));
  _tcscat (SystemWrongDicPath, _T ("UserDic.dic")); // Should be tunable really
  ReadUserDic (Memorized, SystemWrongDicPath); // We should load user dictionary first.

  CLEAN_AND_ZERO_STRING_VECTOR (FileList);
}

BOOL HunspellController::GetLangOnlySystem(const wchar_t *Lang)
{
  AvailableLangInfo Needle;
  Needle.Name = const_cast<wchar_t *> (Lang); // TODO: remove const_cast
  Needle.Type = 1;
  std::set <AvailableLangInfo>::iterator It = DicList->find (Needle);
  if (It != DicList->end () && (*It).Type == 1)
    return TRUE;
  else
    return FALSE;
}

BOOL HunspellController::IsWorking ()
{
  return IsHunspellWorking;
}