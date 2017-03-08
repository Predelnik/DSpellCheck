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
#endif                  // near
#define HUNSPELL_STATIC // We're using static build so ...
#include "CommonFunctions.h"
#include "hunspell/hunspell.hxx"
#include "HunspellInterface.h"
#include "plugin.h"

#include <locale>
#include <io.h>
#include <fcntl.h>

static BOOL ListFiles(wchar_t *path, wchar_t *mask,
                      std::vector<wchar_t *> &files, wchar_t *Filter) {
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATA ffd;
  wchar_t *spec = 0;
  std::stack<wchar_t *> *directories = new std::stack<wchar_t *>;
  BOOL Result = TRUE;

  directories->push(path);
  files.clear();

  while (!directories->empty()) {
    path = directories->top();
    CLEAN_AND_ZERO_ARR(spec);
    spec = new wchar_t[wcslen(path) + 1 + wcslen(mask) + 1];
    wcscpy(spec, path);
    wcscat(spec, L"\\");
    wcscat(spec, mask);
    directories->pop();

    hFind = FindFirstFile(spec, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
      Result = FALSE;
      goto cleanup;
    }

    do {
      if (wcscmp(ffd.cFileName, L".") != 0 &&
          wcscmp(ffd.cFileName, L"..") != 0) {
        wchar_t *buf =
            new wchar_t[wcslen(path) + 1 + wcslen(ffd.cFileName) + 1];
        wcscpy(buf, path);
        wcscat(buf, L"\\");
        wcscat(buf, ffd.cFileName);
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          directories->push(buf);
        } else {
          if (PathMatchSpec(buf, Filter))
            files.push_back(buf);
          else
            CLEAN_AND_ZERO_ARR(buf);
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
  CLEAN_AND_ZERO_ARR(spec);
  CLEAN_AND_ZERO_STRING_STACK(directories);

  return Result;
}

HunspellInterface::HunspellInterface(HWND NppWindowArg) {
  NppWindow = NppWindowArg;
  DicList = new std::set<AvailableLangInfo>;
  Spellers = new std::vector<DicInfo>;
  memset(&Empty, 0, sizeof(Empty));
  Ignored = new WordSet;
  Memorized = new WordSet;
  SingularSpeller = Empty;
  DicDir = 0;
  SysDicDir = 0;
  LastSelectedSpeller = Empty;
  AllHunspells =
      new std::map<wchar_t *, DicInfo, bool (*)(wchar_t *, wchar_t *)>(
          SortCompare);
  IsHunspellWorking = FALSE;
  TemporaryBuffer = new char[DEFAULT_BUF_SIZE];
  UserDicPath = 0;
  SystemWrongDicPath = 0;
}

void HunspellInterface::UpdateOnDicRemoval(wchar_t *Path,
                                           BOOL &NeedSingleLangReset,
                                           BOOL &NeedMultiLangReset) {
  std::map<wchar_t *, DicInfo, bool (*)(wchar_t *, wchar_t *)>::iterator it =
      AllHunspells->find(Path);
  NeedSingleLangReset = FALSE;
  NeedMultiLangReset = FALSE;
  if (it != AllHunspells->end()) {
    if (SingularSpeller.Speller == (*it).second.Speller)
      NeedSingleLangReset = TRUE;

    if (Spellers) {
      for (unsigned int i = 0; i < Spellers->size(); i++)
        if (Spellers->at(i).Speller == (*it).second.Speller) {
          NeedMultiLangReset = TRUE;
        }
    }

    delete[]((*it).first);
    CLEAN_AND_ZERO((*it).second.Speller);
    WriteUserDic((*it).second.LocalDic, (*it).second.LocalDicPath);
    CLEAN_AND_ZERO((*it).second.LocalDicPath);
    if ((*it).second.Converter != (iconv_t)-1)
      iconv_close((*it).second.Converter);

    if ((*it).second.BackConverter != (iconv_t)-1)
      iconv_close((*it).second.BackConverter);

    if ((*it).second.ConverterANSI != (iconv_t)-1)
      iconv_close((*it).second.ConverterANSI);

    if ((*it).second.BackConverterANSI != (iconv_t)-1)
      iconv_close((*it).second.BackConverterANSI);

    CLEAN_AND_ZERO((*it).second.LocalDic);
    CLEAN_AND_ZERO((*it).second.LocalDicPath);
    AllHunspells->erase(it);
  }
}

static void CleanAndZeroWordList(WordSet *&WordListInstance) {
  WordSet::iterator it = WordListInstance->begin();
  for (; it != WordListInstance->end(); ++it) {
    delete[](*it);
  }
  CLEAN_AND_ZERO(WordListInstance);
}

void HunspellInterface::SetUseOneDic(BOOL Value) { UseOneDic = Value; }

BOOL ArePathsEqual(wchar_t *path1, wchar_t *path2) {
  BY_HANDLE_FILE_INFORMATION bhfi1, bhfi2;
  HANDLE h1, h2 = NULL;
  DWORD access = 0;
  DWORD share = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;

  h1 = CreateFile(path1, access, share, NULL, OPEN_EXISTING,
                  (GetFileAttributes(path1) & FILE_ATTRIBUTE_DIRECTORY)
                      ? FILE_FLAG_BACKUP_SEMANTICS
                      : 0,
                  NULL);
  if (INVALID_HANDLE_VALUE != h1) {
    if (!GetFileInformationByHandle(h1, &bhfi1))
      bhfi1.dwVolumeSerialNumber = 0;
    h2 = CreateFile(path2, access, share, NULL, OPEN_EXISTING,
                    (GetFileAttributes(path2) & FILE_ATTRIBUTE_DIRECTORY)
                        ? FILE_FLAG_BACKUP_SEMANTICS
                        : 0,
                    NULL);
    if (!GetFileInformationByHandle(h2, &bhfi2))
      bhfi2.dwVolumeSerialNumber = bhfi1.dwVolumeSerialNumber + 1;
  } else
    return FALSE;

  CloseHandle(h1);
  CloseHandle(h2);
  return INVALID_HANDLE_VALUE != h1 && INVALID_HANDLE_VALUE != h2 &&
         bhfi1.dwVolumeSerialNumber == bhfi2.dwVolumeSerialNumber &&
         bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh &&
         bhfi1.nFileIndexLow == bhfi2.nFileIndexLow;
}

HunspellInterface::~HunspellInterface() {
  IsHunspellWorking = FALSE;
  {
    std::map<wchar_t *, DicInfo, bool (*)(wchar_t *, wchar_t *)>::iterator it =
        AllHunspells->begin();
    for (; it != AllHunspells->end(); ++it) {
      delete[]((*it).first);
      CLEAN_AND_ZERO((*it).second.Speller);
      WriteUserDic((*it).second.LocalDic, (*it).second.LocalDicPath);
      WordSet::iterator NestedIt;
      for (NestedIt = (*it).second.LocalDic->begin();
           NestedIt != (*it).second.LocalDic->end(); ++NestedIt)
        delete[](*NestedIt);

      CLEAN_AND_ZERO((*it).second.LocalDic);
      CLEAN_AND_ZERO((*it).second.LocalDicPath);
      if ((*it).second.Converter != (iconv_t)-1)
        iconv_close((*it).second.Converter);

      if ((*it).second.BackConverter != (iconv_t)-1)
        iconv_close((*it).second.BackConverter);

      if ((*it).second.ConverterANSI != (iconv_t)-1)
        iconv_close((*it).second.ConverterANSI);

      if ((*it).second.BackConverterANSI != (iconv_t)-1)
        iconv_close((*it).second.BackConverterANSI);
    }
    CLEAN_AND_ZERO(AllHunspells);
  }

  if (SystemWrongDicPath && UserDicPath &&
      !ArePathsEqual(SystemWrongDicPath, UserDicPath)) {
    SetFileAttributes(SystemWrongDicPath, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(SystemWrongDicPath);
  }

  if (InitialReadingBeenDone && UserDicPath)
    WriteUserDic(Memorized, UserDicPath);

  CleanAndZeroWordList(Memorized);
  CleanAndZeroWordList(Ignored);

  CLEAN_AND_ZERO(Spellers);
  CLEAN_AND_ZERO_ARR(DicDir);
  CLEAN_AND_ZERO_ARR(SysDicDir);
  CLEAN_AND_ZERO_ARR(TemporaryBuffer);
  CLEAN_AND_ZERO_ARR(UserDicPath);
  CLEAN_AND_ZERO_ARR(SystemWrongDicPath);

  std::set<AvailableLangInfo>::iterator it;
  it = DicList->begin();
  for (; it != DicList->end(); ++it) {
    delete[]((*it).Name);
  }
  CLEAN_AND_ZERO(DicList);
}

std::vector<wchar_t *> *HunspellInterface::GetLanguageList() {
  std::vector<wchar_t *> *List = new std::vector<wchar_t *>;
  // Just copying vector
  std::set<AvailableLangInfo>::iterator it = DicList->begin();
  for (; it != DicList->end(); ++it) {
    wchar_t *Buf = 0;
    SetString(Buf, (*it).Name);
    List->push_back(Buf);
  }
  return List;
}

DicInfo HunspellInterface::CreateHunspell(wchar_t *Name, int Type) {
  auto size = (Type ? wcslen(SysDicDir) : wcslen(DicDir)) + 1 + wcslen(Name) +
              1 + 3 + 1; // + . + aff/dic + /0
  wchar_t *AffBuf = new wchar_t[size];
  char *AffBufAnsi = 0;
  char *DicBufAnsi = 0;
  wcscpy(AffBuf, (Type ? SysDicDir : DicDir));
  wcscat(AffBuf, L"\\");
  wcscat(AffBuf, Name);
  {
    std::map<wchar_t *, DicInfo, bool (*)(wchar_t *, wchar_t *)>::iterator it =
        AllHunspells->find(AffBuf);
    if (it != AllHunspells->end()) {
      CLEAN_AND_ZERO_ARR(AffBuf);
      return (*it).second;
    }
  }
  wchar_t *DicBuf = new wchar_t[size];
  wcscat(AffBuf, L".aff");
  wcscpy(DicBuf, Type ? SysDicDir : DicDir);
  wcscat(DicBuf, L"\\");
  wcscat(DicBuf, Name);
  wcscat(DicBuf, L".dic");
  SetString(AffBufAnsi, AffBuf);
  SetString(DicBufAnsi, DicBuf);

  Hunspell *NewHunspell = new Hunspell(AffBufAnsi, DicBufAnsi);
  wchar_t *NewName = 0;
  AffBuf[wcslen(AffBuf) - 4] = L'\0';
  SetString(NewName, AffBuf); // Without aff and dic
  DicInfo NewDic;
  char *DicEnconding = NewHunspell->get_dic_encoding();
  if (stricmp(DicEnconding, "Microsoft-cp1251") == 0)
    DicEnconding = "cp1251"; // Queer fix for encoding which isn't being guessed
                             // correctly by libiconv TODO: Find other possible
                             // such failures
  NewDic.Converter = iconv_open(DicEnconding, "UTF-8");
  NewDic.BackConverter = iconv_open("UTF-8", DicEnconding);
  NewDic.ConverterANSI = iconv_open(DicEnconding, "");
  NewDic.BackConverterANSI = iconv_open("", DicEnconding);
  NewDic.LocalDic = new WordSet();
  NewDic.LocalDicPath =
      new wchar_t[wcslen(DicDir) + 1 + wcslen(Name) + 1 + 3 +
                  1]; // Local Dic path always points to non-system directory
  NewDic.LocalDicPath[0] = '\0';
  wcscat(NewDic.LocalDicPath, DicDir);
  wcscat(NewDic.LocalDicPath, L"\\");
  wcscat(NewDic.LocalDicPath, Name);
  wcscat(NewDic.LocalDicPath, L".usr");

  ReadUserDic(NewDic.LocalDic, NewDic.LocalDicPath);
  NewDic.Speller = NewHunspell;
  {
    WordSet::iterator it = Memorized->begin();
    for (; it != Memorized->end(); ++it) {
      char *ConvWord = GetConvertedWord(*it, NewDic.Converter);
      if (*ConvWord)
        NewHunspell->add(ConvWord); // Adding all already memorized words to
                                    // newly loaded Hunspell instance
    }
  }
  {
    WordSet::iterator it = NewDic.LocalDic->begin();
    for (; it != NewDic.LocalDic->end(); ++it) {
      NewHunspell->add(*it); // Adding all already memorized words from local
                             // dictionary to Hunspell instance, local
                             // dictionaries are in dictionary encoding
    }
  }
  (*AllHunspells)[NewName] = NewDic;
  CLEAN_AND_ZERO_ARR(AffBuf);
  CLEAN_AND_ZERO_ARR(DicBuf);
  CLEAN_AND_ZERO_ARR(AffBufAnsi);
  CLEAN_AND_ZERO_ARR(DicBufAnsi);
  return NewDic;
}

void HunspellInterface::SetLanguage(wchar_t *Lang) {
  if (DicList->size() == 0) {
    SingularSpeller = Empty;
    return;
  }
  AvailableLangInfo Temp;
  Temp.Name = Lang;
  Temp.Type = 0;
  std::set<AvailableLangInfo>::iterator SearchResult = DicList->find(Temp);
  if (SearchResult == DicList->end()) {
    SearchResult = DicList->begin();
  }
  SingularSpeller = CreateHunspell((*SearchResult).Name, (*SearchResult).Type);
}

void HunspellInterface::SetMultipleLanguages(std::vector<wchar_t *> *List) {
  Spellers->clear();

  if (DicList->size() == 0)
    return;

  for (unsigned int i = 0; i < List->size(); i++) {
    AvailableLangInfo Temp;
    Temp.Name = List->at(i);
    Temp.Type = 0;
    std::set<AvailableLangInfo>::iterator SearchResult = DicList->find(Temp);
    if (SearchResult == DicList->end())
      continue;
    DicInfo Instance =
        CreateHunspell((*SearchResult).Name, (*SearchResult).Type);
    Spellers->push_back(Instance);
  }
}

char *HunspellInterface::GetConvertedWord(const char *Source,
                                          iconv_t Converter) {
  if (Converter == iconv_t(-1)) {
    *TemporaryBuffer = '\0';
    return TemporaryBuffer;
  }
  size_t InSize = strlen(Source) + 1;
  size_t OutSize = DEFAULT_BUF_SIZE;
  char *OutBuf = TemporaryBuffer;
  size_t res = iconv(Converter, &Source, &InSize, &OutBuf, &OutSize);
  if (res == (size_t)(-1)) {
    *TemporaryBuffer = '\0';
  }
  return TemporaryBuffer;
}

BOOL HunspellInterface::SpellerCheckWord(DicInfo Dic, char *Word,
                                         EncodingType Encoding) {
  char *WordToCheck = GetConvertedWord(
      Word, Encoding == (ENCODING_UTF8) ? Dic.Converter : Dic.ConverterANSI);
  if (!*WordToCheck)
    return FALSE;

  // No additional check for memorized is needed since all words are already in
  // dictionary

  return Dic.Speller->spell(WordToCheck);
}

BOOL HunspellInterface::CheckWord(char *Word) {
  /*
  if (Memorized->find (Word) != Memorized->end ()) // This check is for
  dictionaries which are in other than utf-8 encoding
  return TRUE;                                   // Though it slows down stuff a
  little, I hope it will do
  */

  if (Ignored->find(Word) != Ignored->end())
    return TRUE;

  BOOL res = FALSE;
  if (!MultiMode) {
    if (SingularSpeller.Speller)
      res = SpellerCheckWord(SingularSpeller, Word, CurrentEncoding);
    else
      res = TRUE;
  } else {
    if (!Spellers || Spellers->size() == 0)
      return TRUE;

    for (int i = 0; i < (int)Spellers->size() && !res; i++) {
      res = res || SpellerCheckWord(Spellers->at(i), Word, CurrentEncoding);
    }
  }
  return res;
}

void HunspellInterface::WriteUserDic(WordSet *Target, wchar_t *Path) {
  FILE *Fp = 0;

  if (Target->size() == 0) {
    SetFileAttributes(Path, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(Path);
    return;
  }

  SortedWordSet TemporarySortedWordSet(SortCompareChars); // Wordset itself
                                                          // being removed here
                                                          // as local variable,
                                                          // all strings are not
                                                          // copied

  wchar_t *LastSlashPos = GetLastSlashPosition(Path);
  if (!LastSlashPos)
    return;
  *LastSlashPos = L'\0';
  CheckForDirectoryExistence(Path);
  *LastSlashPos = L'\\';

  SetFileAttributes(Path, FILE_ATTRIBUTE_NORMAL);

  Fp = _wfopen(Path, L"w");
  if (!Fp)
    return;
  {
    WordSet::iterator it = Target->begin();
    fprintf(Fp, "%zu\n", Target->size());
    for (; it != Target->end(); ++it) {
      TemporarySortedWordSet.insert(*it);
    }
  }
  SortedWordSet::iterator it = TemporarySortedWordSet.begin();
  for (; it != TemporarySortedWordSet.end(); ++it) {
    fprintf(Fp, "%s\n", *it);
  }

  fclose(Fp);
}

void HunspellInterface::ReadUserDic(WordSet *Target, wchar_t *Path) {
  FILE *Fp = 0;
  int WordNum = 0;
  Fp = _wfopen(Path, L"r");
  if (!Fp) {
    return;
  }
  {
    char Buf[DEFAULT_BUF_SIZE];
    if (fscanf(Fp, "%d\n", &WordNum) != 1) {
      return;
    }
    for (int i = 0; i < WordNum; i++) {
      if (fgets(Buf, DEFAULT_BUF_SIZE, Fp)) {
        Buf[strlen(Buf) - 1] = 0;
        if (Target->find(Buf) == Target->end()) {
          char *Word = 0;
          SetString(Word, Buf);
          Target->insert(Word);
        }
      } else
        break;
    }
  }

  fclose(Fp);
}

void HunspellInterface::MessageBoxWordCannotBeAdded() {
  MessageBoxInfo MsgBox(
      NppWindow, L"Sadly, this word contains symbols out of current dictionary "
                 L"encoding, thus it cannot be added to user dictionary. You "
                 L"can convert this dictionary to UTF-8 or choose the "
                 L"different one with appropriate encoding.",
      L"Word cannot be added", MB_OK | MB_ICONWARNING);
  SendMessage(NppWindow,
              GetCustomGUIMessageId(CustomGUIMessage::DO_MESSAGE_BOX),
              (WPARAM)&MsgBox, 0);
}

void HunspellInterface::AddToDictionary(char *Word) {
  if (!LastSelectedSpeller.Speller)
    return;

  wchar_t *DicPath = 0;
  if (UseOneDic)
    DicPath = UserDicPath;
  else
    DicPath = LastSelectedSpeller.LocalDicPath;

  if (!PathFileExists(DicPath)) {
    wchar_t *LastSlashPos = GetLastSlashPosition(DicPath);
    if (!LastSlashPos)
      return;
    *LastSlashPos = L'\0';
    CheckForDirectoryExistence(DicPath);
    *LastSlashPos = L'\\';
    // If there's no file then we're checking if we can create it, there's no
    // harm in it
    int LocalDicFileHandle = _wopen(DicPath, _O_CREAT | _O_BINARY | _O_WRONLY);
    if (LocalDicFileHandle == -1) {
      MessageBoxInfo MsgBox(
          NppWindow, L"User dictionary cannot be written, please check if you "
                     L"have access for writing into your dictionary directory, "
                     L"otherwise you can change it or run Notepad++ as "
                     L"administrator.",
          L"User dictionary cannot be saved", MB_OK | MB_ICONWARNING);
      SendMessage(NppWindow,
                  GetCustomGUIMessageId(CustomGUIMessage::DO_MESSAGE_BOX),
                  (WPARAM)&MsgBox, 0);
    } else {
      _close(LocalDicFileHandle);
    }
    SetFileAttributes(DicPath, FILE_ATTRIBUTE_NORMAL);
  } else {
    SetFileAttributes(DicPath, FILE_ATTRIBUTE_NORMAL);
    int LocalDicFileHandle = _wopen(DicPath, _O_APPEND | _O_BINARY | _O_WRONLY);
    if (LocalDicFileHandle == -1) {
      MessageBoxInfo MsgBox(
          NppWindow, L"User dictionary cannot be written, please check if you "
                     L"have access for writing into your dictionary directory, "
                     L"otherwise you can change it or run Notepad++ as "
                     L"administrator.",
          L"User dictionary cannot be saved", MB_OK | MB_ICONWARNING);
      SendMessage(NppWindow,
                  GetCustomGUIMessageId(CustomGUIMessage::DO_MESSAGE_BOX),
                  (WPARAM)&MsgBox, 0);
    } else
      _close(LocalDicFileHandle);
  }

  char *Buf = 0;
  if (CurrentEncoding == ENCODING_UTF8)
    SetString(Buf, Word);
  else
    SetStringDUtf8(Buf, Word);

  if (UseOneDic) {
    std::map<wchar_t *, DicInfo, bool (*)(wchar_t *, wchar_t *)>::iterator it;
    Memorized->insert(Buf);
    it = AllHunspells->begin();
    for (; it != AllHunspells->end(); ++it) {
      char *ConvWord = GetConvertedWord(Buf, (*it).second.Converter);
      if (*ConvWord)
        (*it).second.Speller->add(ConvWord);
      else if ((*it).second.Speller == LastSelectedSpeller.Speller)
        MessageBoxWordCannotBeAdded();
      // Adding word to all currently loaded dictionaries and in memorized list
      // to save it.
    }
  } else {
    char *ConvWord = GetConvertedWord(Buf, LastSelectedSpeller.Converter);
    char *WordCopy = 0;
    SetString(WordCopy, ConvWord);
    LastSelectedSpeller.LocalDic->insert(WordCopy);
    if (*ConvWord)
      LastSelectedSpeller.Speller->add(ConvWord);
    else
      MessageBoxWordCannotBeAdded();
  }
}

void HunspellInterface::IgnoreAll(char *Word) {
  if (!LastSelectedSpeller.Speller)
    return;

  char *Buf = 0;
  SetString(Buf, Word);
  Ignored->insert(Buf);
}

std::vector<char *> *HunspellInterface::GetSuggestions(char *Word) {
  std::vector<char *> *SuggList = new std::vector<char *>;
  int Num = -1;
  int CurNum;
  char **HunspellList = 0;
  char **CurHunspellList = 0;
  char *WordUtf8 = 0;
  LastSelectedSpeller = SingularSpeller;

  if (!MultiMode) {
    Num = SingularSpeller.Speller->suggest(
        &HunspellList,
        GetConvertedWord(Word, (CurrentEncoding == ENCODING_UTF8)
                                   ? SingularSpeller.Converter
                                   : SingularSpeller.ConverterANSI));
  } else {
    int MaxSize = -1;
    // In this mode we're finding maximum by length list from selected languages
    CurHunspellList = 0;
    for (int i = 0; i < (int)Spellers->size(); i++) {
      CurNum = Spellers->at(i).Speller->suggest(
          &CurHunspellList,
          GetConvertedWord(Word, (CurrentEncoding == ENCODING_UTF8)
                                     ? Spellers->at(i).Converter
                                     : Spellers->at(i).ConverterANSI));

      if (CurNum > MaxSize) {
        if (Num != -1)
          Spellers->at(i).Speller->free_list(&HunspellList, Num);

        MaxSize = CurNum;
        LastSelectedSpeller = Spellers->at(i);
        HunspellList = CurHunspellList;
        Num = CurNum;
      } else {
        Spellers->at(i).Speller->free_list(&CurHunspellList, CurNum);
      }
    }
  }

  for (int i = 0; i < Num; i++) {
    char *Buf = 0;
    SetString(Buf, GetConvertedWord(HunspellList[i],
                                    LastSelectedSpeller.BackConverter));
    SuggList->push_back(Buf);
  }

  LastSelectedSpeller.Speller->free_list(&HunspellList, Num);

  CLEAN_AND_ZERO_ARR(WordUtf8);

  return SuggList;
}

void HunspellInterface::SetDirectory(wchar_t *Dir) {
  CLEAN_AND_ZERO_ARR(UserDicPath);
  UserDicPath = new wchar_t[DEFAULT_BUF_SIZE];
  wcscpy(UserDicPath, Dir);
  if (UserDicPath[wcslen(UserDicPath) - 1] != L'\\')
    wcscat(UserDicPath, L"\\");
  wcscat(UserDicPath, L"UserDic.dic"); // Should be tunable really
  ReadUserDic(Memorized, UserDicPath); // We should load user dictionary first.

  InitialReadingBeenDone = FALSE;
  std::vector<wchar_t *> *FileList = new std::vector<wchar_t *>;
  SetString(DicDir, Dir);

  std::set<AvailableLangInfo>::iterator it;
  it = DicList->begin();
  for (; it != DicList->end(); ++it) {
    delete[]((*it).Name);
  }
  CLEAN_AND_ZERO(DicList);

  DicList = new std::set<AvailableLangInfo>;
  IsHunspellWorking = TRUE;

  BOOL Res = ListFiles(Dir, L"*.*", *FileList, L"*.aff");
  if (!Res) {
    CLEAN_AND_ZERO_STRING_VECTOR(FileList);
    return;
  }

  for (unsigned int i = 0; i < FileList->size(); i++) {
    wchar_t *Buf = 0;
    SetString(Buf, FileList->at(i));
    wchar_t *DotPointer = wcsrchr(Buf, L'.');
    wcscpy(DotPointer, L".dic");
    if (PathFileExists(Buf)) {
      *DotPointer = 0;
      wchar_t *SlashPointer = wcsrchr(Buf, L'\\');
      wchar_t *TBuf = 0;
      SetString(TBuf, SlashPointer + 1);
      AvailableLangInfo NewX;
      NewX.Type = 0;
      NewX.Name = TBuf;
      DicList->insert(NewX);
    }
    CLEAN_AND_ZERO_ARR(Buf);
  }

  CLEAN_AND_ZERO_STRING_VECTOR(FileList);
}

void HunspellInterface::SetAdditionalDirectory(wchar_t *Dir) {
  InitialReadingBeenDone = FALSE;
  std::vector<wchar_t *> *FileList = new std::vector<wchar_t *>;
  SetString(SysDicDir, Dir);

  if (!DicList)
    return;
  IsHunspellWorking = TRUE;

  BOOL Res = ListFiles(Dir, L"*.*", *FileList, L"*.aff");
  if (!Res) {
    CLEAN_AND_ZERO_STRING_VECTOR(FileList);
    return;
  }

  for (unsigned int i = 0; i < FileList->size(); i++) {
    wchar_t *Buf = 0;
    SetString(Buf, FileList->at(i));
    wchar_t *DotPointer = wcsrchr(Buf, L'.');
    wcscpy(DotPointer, L".dic");
    if (PathFileExists(Buf)) {
      *DotPointer = 0;
      wchar_t *SlashPointer = wcsrchr(Buf, L'\\');
      wchar_t *TBuf = 0;
      SetString(TBuf, SlashPointer + 1);
      AvailableLangInfo NewX;
      NewX.Type = 1;
      NewX.Name = TBuf;
      if (DicList->find(NewX) == DicList->end())
        DicList->insert(NewX);
      else
        CLEAN_AND_ZERO_ARR(TBuf);
    }
    CLEAN_AND_ZERO_ARR(Buf);
  }
  // Now we have 2 dictionaries on our hands

  InitialReadingBeenDone = TRUE;

  CLEAN_AND_ZERO_ARR(SystemWrongDicPath);
  SystemWrongDicPath =
      new wchar_t[DEFAULT_BUF_SIZE]; // Reading system path unified dic too
  wcscpy(SystemWrongDicPath, Dir);
  if (SystemWrongDicPath[wcslen(SystemWrongDicPath) - 1] != L'\\')
    wcscat(SystemWrongDicPath, L"\\");
  wcscat(SystemWrongDicPath, L"UserDic.dic"); // Should be tunable really
  ReadUserDic(Memorized,
              SystemWrongDicPath); // We should load user dictionary first.

  CLEAN_AND_ZERO_STRING_VECTOR(FileList);
}

BOOL HunspellInterface::GetLangOnlySystem(wchar_t *Lang) {
  AvailableLangInfo Needle;
  Needle.Name = Lang;
  Needle.Type = 1;
  std::set<AvailableLangInfo>::iterator It = DicList->find(Needle);
  if (It != DicList->end() && (*It).Type == 1)
    return TRUE;
  else
    return FALSE;
}

BOOL HunspellInterface::IsWorking() { return IsHunspellWorking; }