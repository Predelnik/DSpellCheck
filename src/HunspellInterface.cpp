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
#include "Plugin.h"

#include <locale>
#include <io.h>
#include <fcntl.h>

static std::vector<std::wstring> ListFiles(wchar_t* path, const wchar_t* mask,
                                           const wchar_t* Filter) {
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;
    std::stack<std::wstring> directories;

    directories.push(path);
    std::vector<std::wstring> out;

    while (!directories.empty()) {
        auto top = std::move(directories.top());
        auto spec = path + L"\\"s + mask;
        directories.pop();

        hFind = FindFirstFile(spec.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE) {
            return {};
        }

        do {
            if (ffd.cFileName != L"."sv &&
                ffd.cFileName != L".."sv) {
                auto buf = path + L"\\"s + ffd.cFileName;
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    directories.push(buf);
                }
                else {
                    if (PathMatchSpec(buf.c_str(), Filter))
                        out.push_back(buf);
                }
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);

        if (GetLastError() != ERROR_NO_MORE_FILES) {
            FindClose(hFind);
            return {};
        }

        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }
    return out;
}

HunspellInterface::HunspellInterface(HWND NppWindowArg) {
    NppWindow = NppWindowArg;
    SingularSpeller = {};
    DicDir = 0;
    SysDicDir = 0;
    LastSelectedSpeller = {};
    IsHunspellWorking = false;
    UserDicPath = 0;
    SystemWrongDicPath = 0;
}

void HunspellInterface::UpdateOnDicRemoval(wchar_t* Path,
                                           bool& NeedSingleLangReset,
                                           bool& NeedMultiLangReset) {
    auto it =
        AllHunspells.find(Path);
    NeedSingleLangReset = false;
    NeedMultiLangReset = false;
    if (it != AllHunspells.end()) {
        if (SingularSpeller && SingularSpeller == &it->second)
            NeedSingleLangReset = true;

        for (auto speller : m_spellers)
            if (speller == &it->second) {
                NeedMultiLangReset = true;
            }
        AllHunspells.erase(it);
    }
}

void HunspellInterface::SetUseOneDic(bool Value) { UseOneDic = Value; }

bool ArePathsEqual(wchar_t* path1, wchar_t* path2) {
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
    }
    else
        return false;

    CloseHandle(h1);
    CloseHandle(h2);
    return INVALID_HANDLE_VALUE != h1 && INVALID_HANDLE_VALUE != h2 &&
        bhfi1.dwVolumeSerialNumber == bhfi2.dwVolumeSerialNumber &&
        bhfi1.nFileIndexHigh == bhfi2.nFileIndexHigh &&
        bhfi1.nFileIndexLow == bhfi2.nFileIndexLow;
}

HunspellInterface::~HunspellInterface() {
    IsHunspellWorking = false;

    if (SystemWrongDicPath && UserDicPath &&
        !ArePathsEqual(SystemWrongDicPath, UserDicPath)) {
        SetFileAttributes(SystemWrongDicPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(SystemWrongDicPath);
    }

    if (InitialReadingBeenDone && UserDicPath)
        WriteUserDic(&Memorized, UserDicPath);

    CLEAN_AND_ZERO_ARR(DicDir);
    CLEAN_AND_ZERO_ARR(SysDicDir);
    CLEAN_AND_ZERO_ARR(UserDicPath);
    CLEAN_AND_ZERO_ARR(SystemWrongDicPath);

}

std::vector<std::wstring> HunspellInterface::GetLanguageList() {
    std::vector<std::wstring> list;
    for (auto& dic : dicList) {
        list.push_back(dic.name);
    }
    return list;
}

DicInfo* HunspellInterface::CreateHunspell(const wchar_t* Name, int Type) {
    std::wstring AffBuf = (Type ? SysDicDir : DicDir) + L"\\"s + Name;
    {
        auto it =
            AllHunspells.find(AffBuf);
        if (it != AllHunspells.end()) {
            return &it->second;
        }
    }
    std::wstring DicBuf;
    AffBuf += L".aff";
    DicBuf = (Type ? SysDicDir : DicDir) + L"\\"s + Name + L".dic";
    auto AffBufAnsi = to_string(AffBuf.c_str());
    auto DicBufAnsi = to_string(DicBuf.c_str());
    auto NewHunspell = std::make_unique<Hunspell>(AffBufAnsi.c_str(), DicBufAnsi.c_str());
    auto NewName = AffBuf.substr(0, AffBuf.length() - 4);
    DicInfo NewDic;
    const char* DicEnconding = NewHunspell->get_dic_encoding();
    if (stricmp(DicEnconding, "Microsoft-cp1251") == 0)
        DicEnconding = "cp1251"; // Queer fix for encoding which isn't being guessed
    // correctly by libiconv TODO: Find other possible
    // such failures
    NewDic.Converter = {DicEnconding, "UTF-8"};
    NewDic.BackConverter = {"UTF-8", DicEnconding};
    NewDic.ConverterANSI = {DicEnconding, ""};
    NewDic.BackConverterANSI = {"", DicEnconding};
    NewDic.LocalDicPath += DicDir + L"\\"s + Name + L".usr";

    ReadUserDic(&NewDic.LocalDic, NewDic.LocalDicPath.c_str());
    NewDic.hunspell = std::move(NewHunspell);
    {
        for (auto word : Memorized) {
            char* ConvWord = GetConvertedWord(word.c_str(), NewDic.Converter.get());
            if (*ConvWord)
                NewHunspell->add(ConvWord); // Adding all already memorized words to
            // newly loaded Hunspell instance
        }
    }
    {
        for (auto word : NewDic.LocalDic) {
            NewHunspell->add(word.c_str()); // Adding all already memorized words from local
            // dictionary to Hunspell instance, local
            // dictionaries are in dictionary encoding
        }
    }
    auto& target = AllHunspells[NewName];
    target = std::move(NewDic);
    return &target;
}

void HunspellInterface::SetLanguage(const wchar_t* Lang) {
    if (dicList.empty()) {
        SingularSpeller = nullptr;
        return;
    }
    AvailableLangInfo Temp;
    Temp.name = Lang;
    Temp.type = 0;
    std::set<AvailableLangInfo>::iterator it = dicList.find(Temp);
    if (it == dicList.end()) {
        it = dicList.begin();
    }
    SingularSpeller = CreateHunspell(it->name.c_str(), it->type);
}

void HunspellInterface::SetMultipleLanguages(std::vector<wchar_t *>* List) {
    m_spellers.clear();

    if (dicList.empty())
        return;

    for (unsigned int i = 0; i < List->size(); i++) {
        AvailableLangInfo Temp;
        Temp.name = List->at(i);
        Temp.type = 0;
        auto it = dicList.find(Temp);
        if (it == dicList.end())
            continue;
        auto ptr =
            CreateHunspell(it->name.c_str(), it->type);
        m_spellers.push_back(ptr);
    }
}

char* HunspellInterface::GetConvertedWord(const char* Source,
                                          iconv_t Converter) {
    static std::vector<char> buf;
    buf.resize(strlen(Source) * 6);
    if (Converter == iconv_t(-1)) {
        buf.front() = '\0';
        return buf.data();
    }
    size_t InSize = strlen(Source) + 1;
    size_t OutSize = DEFAULT_BUF_SIZE;
    char* OutBuf = buf.data();
    size_t res = iconv(Converter, &Source, &InSize, &OutBuf, &OutSize);
    if (res == (size_t)(-1)) {
        buf.front() = '\0';
    }
    return buf.data();
}

bool HunspellInterface::SpellerCheckWord(const DicInfo& Dic, char* Word,
                                         EncodingType Encoding) {
    char* WordToCheck = GetConvertedWord(
        Word, Encoding == (ENCODING_UTF8) ? Dic.Converter.get() : Dic.ConverterANSI.get());
    if (!*WordToCheck)
        return false;

    // No additional check for memorized is needed since all words are already in
    // dictionary

    return Dic.hunspell->spell(WordToCheck);
}

bool HunspellInterface::CheckWord(char* Word) {
    /*
    if (Memorized->find (Word) != Memorized->end ()) // This check is for
    dictionaries which are in other than utf-8 encoding
    return true;                                   // Though it slows down stuff a
    little, I hope it will do
    */

    if (Ignored.find(Word) != Ignored.end())
        return true;

    bool res = false;
    if (!MultiMode) {
        if (SingularSpeller)
            res = SpellerCheckWord(*SingularSpeller, Word, CurrentEncoding);
        else
            res = true;
    }
    else {
        if (m_spellers.empty())
            return true;

        for (auto& speller : m_spellers) {
            res = res || SpellerCheckWord(*speller, Word, CurrentEncoding);
            if (res)
                break;
        }
    }
    return res;
}

void HunspellInterface::WriteUserDic(WordSet* Target, wchar_t* Path) {
    FILE* Fp = 0;

    if (Target->size() == 0) {
        SetFileAttributes(Path, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(Path);
        return;
    }

    SortedWordSet TemporarySortedWordSet; // Wordset itself
    // being removed here
    // as local variable,
    // all strings are not
    // copied

    wchar_t* LastSlashPos = GetLastSlashPosition(Path);
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
        fprintf(Fp, "%zu\n", Target->size());
        for (auto word : *Target) {
            TemporarySortedWordSet.insert(word);
        }
    }
    SortedWordSet::iterator it = TemporarySortedWordSet.begin();
    for (; it != TemporarySortedWordSet.end(); ++it) {
        fprintf(Fp, "%s\n", it->c_str());
    }

    fclose(Fp);
}

void HunspellInterface::ReadUserDic(WordSet* Target, const wchar_t* Path) {
    FILE* Fp = 0;
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
                    char* Word = 0;
                    SetString(Word, Buf);
                    Target->insert(Word);
                }
            }
            else
                break;
        }
    }

    fclose(Fp);
}

void HunspellInterface::MessageBoxWordCannotBeAdded() {
    MessageBox(
        NppWindow, L"Sadly, this word contains symbols out of current dictionary "
        L"encoding, thus it cannot be added to user dictionary. You "
        L"can convert this dictionary to UTF-8 or choose the "
        L"different one with appropriate encoding.",
        L"Word cannot be added", MB_OK | MB_ICONWARNING);
}

void HunspellInterface::AddToDictionary(char* Word) {
    if (!LastSelectedSpeller)
        return;

    std::wstring DicPath;
    if (UseOneDic)
        DicPath = UserDicPath;
    else
        DicPath = LastSelectedSpeller->LocalDicPath;

    if (!PathFileExists(DicPath.c_str())) {
        auto last_slash_pos = DicPath.rfind(L"\\");
        if (last_slash_pos == std::wstring::npos)
            return;
        auto dir = DicPath.substr(0, last_slash_pos);
        CheckForDirectoryExistence(dir.c_str());
        // If there's no file then we're checking if we can create it, there's no
        // harm in it
        int LocalDicFileHandle = _wopen(DicPath.c_str(), _O_CREAT | _O_BINARY | _O_WRONLY);
        if (LocalDicFileHandle == -1) {
            MessageBox(
                NppWindow, L"User dictionary cannot be written, please check if you "
                L"have access for writing into your dictionary directory, "
                L"otherwise you can change it or run Notepad++ as "
                L"administrator.",
                L"User dictionary cannot be saved", MB_OK | MB_ICONWARNING);
        }
        else {
            _close(LocalDicFileHandle);
        }
        SetFileAttributes(DicPath.c_str(), FILE_ATTRIBUTE_NORMAL);
    }
    else {
        SetFileAttributes(DicPath.c_str(), FILE_ATTRIBUTE_NORMAL);
        int LocalDicFileHandle = _wopen(DicPath.c_str(), _O_APPEND | _O_BINARY | _O_WRONLY);
        if (LocalDicFileHandle == -1) {
            MessageBox(
                NppWindow, L"User dictionary cannot be written, please check if you "
                L"have access for writing into your dictionary directory, "
                L"otherwise you can change it or run Notepad++ as "
                L"administrator.",
                L"User dictionary cannot be saved", MB_OK | MB_ICONWARNING);
        }
        else
            _close(LocalDicFileHandle);
    }

    char* Buf = 0;
    if (CurrentEncoding == ENCODING_UTF8)
        SetString(Buf, Word);
    else
        SetStringDUtf8(Buf, Word);

    if (UseOneDic) {
        std::map<wchar_t *, DicInfo, bool (*)(wchar_t*, wchar_t*)>::iterator it;
        Memorized.insert(Buf);
        for (auto& p : AllHunspells) {
            char* ConvWord = GetConvertedWord(Buf, p.second.Converter.get());
            if (*ConvWord)
                (*it).second.hunspell->add(ConvWord);
            else if ((*it).second.hunspell == LastSelectedSpeller->hunspell)
                MessageBoxWordCannotBeAdded();
            // Adding word to all currently loaded dictionaries and in memorized list
            // to save it.
        }
    }
    else {
        char* ConvWord = GetConvertedWord(Buf, LastSelectedSpeller->Converter.get());
        char* WordCopy = 0;
        SetString(WordCopy, ConvWord);
        LastSelectedSpeller->LocalDic.insert(WordCopy);
        if (*ConvWord)
            LastSelectedSpeller->hunspell->add(ConvWord);
        else
            MessageBoxWordCannotBeAdded();
    }
}

void HunspellInterface::IgnoreAll(char* Word) {
    if (!LastSelectedSpeller)
        return;

    char* Buf = 0;
    SetString(Buf, Word);
    Ignored.insert(Buf);
}

namespace
{
    class HunspellSuggestions {
    public:
        HunspellSuggestions(Hunspell* hunspell, const char* word) : m_hunspell(hunspell) {
            m_count = hunspell->suggest(&m_list, word);
        }

        HunspellSuggestions() {
        }

        ~HunspellSuggestions() {
            if (m_list)
                m_hunspell->free_list(&m_list, m_count);
        }

        const char* word(int index) const { return m_list[index]; }
        int count() const { return m_count; }

    private:
        char** m_list = nullptr;
        int m_count = 0;
        Hunspell* m_hunspell = nullptr;
    };
}

std::vector<std::string> HunspellInterface::GetSuggestions(const char* Word) {
    std::vector<std::string> SuggList;
    HunspellSuggestions list;
    LastSelectedSpeller = SingularSpeller;

    if (!MultiMode) {
        list = {
            SingularSpeller->hunspell.get(), GetConvertedWord(Word, (CurrentEncoding == ENCODING_UTF8)
                                                                        ? SingularSpeller->Converter.get()
                                                                        : SingularSpeller->ConverterANSI.get())
        };
    }
    else {
        // In this mode we're finding maximum by length list from selected languages
        HunspellSuggestions curList;
        for (auto speller : m_spellers) {
            curList = {
                speller->hunspell.get(), GetConvertedWord(Word, (CurrentEncoding == ENCODING_UTF8)
                                                                    ? speller->Converter.get()
                                                                    : speller->ConverterANSI.get())
            };

            if (curList.count() > list.count()) {
                list = std::move(curList);
            }
        }
    }

    for (int i = 0; i < list.count(); i++) {
        SuggList.push_back(GetConvertedWord(list.word(i),
                                            LastSelectedSpeller->BackConverter.get()));
    }
    return SuggList;
}

void HunspellInterface::SetDirectory(wchar_t* Dir) {
    CLEAN_AND_ZERO_ARR(UserDicPath);
    UserDicPath = new wchar_t[DEFAULT_BUF_SIZE];
    wcscpy(UserDicPath, Dir);
    if (UserDicPath[wcslen(UserDicPath) - 1] != L'\\')
        wcscat(UserDicPath, L"\\");
    wcscat(UserDicPath, L"UserDic.dic"); // Should be tunable really
    ReadUserDic(&Memorized, UserDicPath); // We should load user dictionary first.

    InitialReadingBeenDone = false;
    SetString(DicDir, Dir);

    std::set<AvailableLangInfo>::iterator it;
    dicList.clear();
    IsHunspellWorking = true;

    auto files = ListFiles(Dir, L"*.*", L"*.aff");
    if (files.empty()) {
        return;
    }

    for (auto& filePath : files) {
        auto affFilePath = filePath.substr(0, filePath.length() - 4) + L".dic";
        if (PathFileExists(affFilePath.c_str())) {
            const auto dicName = filePath.substr(0, affFilePath.rfind(L'\\'));
            AvailableLangInfo newX;
            newX.type = 0;
            newX.name = dicName;
            dicList.insert(newX);
        }
    }
}

void HunspellInterface::SetAdditionalDirectory(wchar_t* Dir) {
    InitialReadingBeenDone = false;
    SetString(SysDicDir, Dir);
    IsHunspellWorking = true;

    auto files = ListFiles(Dir, L"*.*", L"*.aff");
    if (files.empty())
        return;

    for (auto& file : files) {
        wchar_t* Buf = 0;
        SetString(Buf, file.c_str());
        wchar_t* DotPointer = wcsrchr(Buf, L'.');
        wcscpy(DotPointer, L".dic");
        if (PathFileExists(Buf)) {
            *DotPointer = 0;
            wchar_t* SlashPointer = wcsrchr(Buf, L'\\');
            wchar_t* TBuf = 0;
            SetString(TBuf, SlashPointer + 1);
            AvailableLangInfo NewX;
            NewX.type = 1;
            NewX.name = TBuf;
            if (dicList.count(NewX) == 0)
                dicList.insert(NewX);
            else
                CLEAN_AND_ZERO_ARR(TBuf);
        }
        CLEAN_AND_ZERO_ARR(Buf);
    }
    // Now we have 2 dictionaries on our hands

    InitialReadingBeenDone = true;

    CLEAN_AND_ZERO_ARR(SystemWrongDicPath);
    SystemWrongDicPath =
        new wchar_t[DEFAULT_BUF_SIZE]; // Reading system path unified dic too
    wcscpy(SystemWrongDicPath, Dir);
    if (SystemWrongDicPath[wcslen(SystemWrongDicPath) - 1] != L'\\')
        wcscat(SystemWrongDicPath, L"\\");
    wcscat(SystemWrongDicPath, L"UserDic.dic"); // Should be tunable really
    ReadUserDic(&Memorized,
                SystemWrongDicPath); // We should load user dictionary first.
}

bool HunspellInterface::GetLangOnlySystem(wchar_t* Lang) {
    AvailableLangInfo Needle;
    Needle.name = Lang;
    Needle.type = 1;
    std::set<AvailableLangInfo>::iterator it = dicList.find(Needle);
    if (it != dicList.end() && it->type == 1)
        return true;
    else
        return false;
}

bool HunspellInterface::IsWorking() { return IsHunspellWorking; }
