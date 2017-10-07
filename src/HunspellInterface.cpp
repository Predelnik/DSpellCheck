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

static std::vector<std::wstring> ListFiles(const wchar_t* path, const wchar_t* mask,
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
    LastSelectedSpeller = {};
    IsHunspellWorking = false;
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

bool ArePathsEqual(const wchar_t* path1, const wchar_t* path2) {
    BY_HANDLE_FILE_INFORMATION bhfi1, bhfi2;
    HANDLE h1, h2 = nullptr;
    DWORD access = 0;
    DWORD share = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;

    h1 = CreateFile(path1, access, share, nullptr, OPEN_EXISTING,
                    (GetFileAttributes(path1) & FILE_ATTRIBUTE_DIRECTORY)
                        ? FILE_FLAG_BACKUP_SEMANTICS
                        : 0,
                    nullptr);
    if (INVALID_HANDLE_VALUE != h1) {
        if (!GetFileInformationByHandle(h1, &bhfi1))
            bhfi1.dwVolumeSerialNumber = 0;
        h2 = CreateFile(path2, access, share, nullptr, OPEN_EXISTING,
                        (GetFileAttributes(path2) & FILE_ATTRIBUTE_DIRECTORY)
                            ? FILE_FLAG_BACKUP_SEMANTICS
                            : 0,
                        nullptr);
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

static void WriteUserDic(WordSet* Target, std::wstring Path) {
    FILE* Fp = nullptr;

    if (Target->size() == 0) {
        SetFileAttributes(Path.c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFile(Path.c_str());
        return;
    }

    SortedWordSet TemporarySortedWordSet;
    // Wordset itself
    // being removed here as local variable, all strings are not copied

    auto slashPos = Path.rfind(L'\\');
    if (slashPos == std::string::npos)
        return;
    CheckForDirectoryExistence(Path.substr(0, slashPos).c_str());

    SetFileAttributes(Path.c_str(), FILE_ATTRIBUTE_NORMAL);

    Fp = _wfopen(Path.c_str(), L"w");
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

HunspellInterface::~HunspellInterface() {
    IsHunspellWorking = false;

    if (!SystemWrongDicPath.empty () && !UserDicPath.empty() &&
        !ArePathsEqual(SystemWrongDicPath.c_str (), UserDicPath.c_str())) {
        SetFileAttributes(SystemWrongDicPath.c_str (), FILE_ATTRIBUTE_NORMAL);
        DeleteFile(SystemWrongDicPath.c_str ());
    }

    if (!UserDicPath.empty ())
        WriteUserDic(&Memorized, UserDicPath.c_str());
    for (auto &p : AllHunspells)
        {
            auto &hs = p.second;
            if (!hs.LocalDicPath.empty ())
                WriteUserDic (&hs.LocalDic, hs.LocalDicPath);
        }
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
    {
        for (auto word : Memorized) {
            auto ConvWord = GetConvertedWord(word.c_str(), NewDic.Converter.get());
            if (!ConvWord.empty ())
                NewHunspell->add(ConvWord.c_str ()); // Adding all already memorized words to
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
    NewDic.hunspell = std::move(NewHunspell);
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

void HunspellInterface::SetMultipleLanguages(const std::vector<std::wstring>& List) {
    m_spellers.clear();

    if (dicList.empty())
        return;

    for (auto &lang : List) {
        AvailableLangInfo Temp;
        Temp.name = lang;
        Temp.type = 0;
        auto it = dicList.find(Temp);
        if (it == dicList.end())
            continue;
        auto ptr =
            CreateHunspell(it->name.c_str(), it->type);
        m_spellers.push_back(ptr);
    }
}

std::string HunspellInterface::GetConvertedWord(const char* Source,
                                                iconv_t Converter) {
    static std::vector<char> buf;
    buf.resize(strlen(Source) * 6);
    if (Converter == iconv_t(-1)) {
        buf.front() = '\0';
        return {};
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

bool HunspellInterface::SpellerCheckWord(const DicInfo& Dic, const char* Word,
                                         EncodingType Encoding) {
    auto WordToCheck = GetConvertedWord(
        Word, Encoding == (ENCODING_UTF8) ? Dic.Converter.get() : Dic.ConverterANSI.get());
    if (WordToCheck.empty ())
      return true;
    // No additional check for memorized is needed since all words are already in
    // dictionary

    return Dic.hunspell->spell(WordToCheck.c_str ());
}

bool HunspellInterface::CheckWord(const char* Word) {
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

void HunspellInterface::ReadUserDic(WordSet* Target, const wchar_t* Path) {
    FILE* Fp = nullptr;
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
                    Target->insert(Buf);
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

void HunspellInterface::AddToDictionary(const char* Word) {
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

   std::string buf;
    if (CurrentEncoding == ENCODING_UTF8)
        buf = Word;
    else
        buf = utf8_to_string(Word);

    if (UseOneDic) {
        Memorized.insert(buf);
        for (auto& p : AllHunspells) {
            auto ConvWord = GetConvertedWord(buf.c_str (), p.second.Converter.get());
            if (!ConvWord.empty ())
                p.second.hunspell->add(ConvWord.c_str ());
            else if (p.second.hunspell == LastSelectedSpeller->hunspell)
                MessageBoxWordCannotBeAdded();
            // Adding word to all currently loaded dictionaries and in memorized list
            // to save it.
        }
    }
    else {
        auto ConvWord = GetConvertedWord(buf.c_str (), LastSelectedSpeller->Converter.get());
        LastSelectedSpeller->LocalDic.insert(ConvWord);
        if (!ConvWord.empty ())
            LastSelectedSpeller->hunspell->add(ConvWord.c_str ());
        else
            MessageBoxWordCannotBeAdded();
    }
}

void HunspellInterface::IgnoreAll(const char* Word) {
    if (!LastSelectedSpeller)
        return;

    Ignored.insert(Word);
}

namespace
{
    class HunspellSuggestions {
        using self = HunspellSuggestions;
    public:
        HunspellSuggestions(Hunspell* hunspell, const char* word) :
            m_hunspell(hunspell) {
            m_count = hunspell->suggest(&m_list, word);
            if (m_list)
                m_flag.make_valid();
        }

        HunspellSuggestions() {
        }

        HunspellSuggestions(self&&) = default;
        self& operator=(self&&) = default;

        ~HunspellSuggestions() {
            if (m_flag.is_valid() && m_list)
                m_hunspell->free_list(&m_list, m_count);
        }

        const char* word(int index) const { return m_list[index]; }
        int count() const { return m_count; }

    private:
        char** m_list = nullptr;
        int m_count = 0;
        move_only_flag m_flag;
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
                                                                        : SingularSpeller->ConverterANSI.get()).c_str ()
        };
    }
    else {
        // In this mode we're finding maximum by length list from selected languages
        HunspellSuggestions curList;
        for (auto speller : m_spellers) {
            curList = {
                speller->hunspell.get(), GetConvertedWord(Word, (CurrentEncoding == ENCODING_UTF8)
                                                                    ? speller->Converter.get()
                                                                    : speller->ConverterANSI.get()).c_str ()
            };

            if (curList.count() > list.count()) {
                list = std::move(curList);
                LastSelectedSpeller = speller;
            }
        }
    }

    for (int i = 0; i < list.count(); i++) {
        SuggList.push_back(GetConvertedWord(list.word(i),
                                            LastSelectedSpeller->BackConverter.get()));
    }
    return SuggList;
}

void HunspellInterface::SetDirectory(const wchar_t* Dir) {
    UserDicPath = Dir;
    if (UserDicPath.back() != L'\\')
        UserDicPath += L"\\";
    UserDicPath += L"UserDic.dic"; // Should be tunable really
    ReadUserDic(&Memorized, UserDicPath.c_str()); // We should load user dictionary first.

    DicDir = Dir;

    dicList.clear();
    IsHunspellWorking = true;

    auto files = ListFiles(Dir, L"*.*", L"*.aff");
    if (files.empty()) {
        return;
    }

    for (auto& filePath : files) {
        auto dicFilePath = filePath.substr(0, filePath.length() - 4) + L".dic";
        if (PathFileExists(dicFilePath.c_str())) {
            auto dicName = filePath.substr(dicFilePath.rfind(L'\\') + 1);
            dicName = dicName.substr(0, dicName.length() - 4);
            AvailableLangInfo newX;
            newX.type = 0;
            newX.name = dicName;
            dicList.insert(newX);
        }
    }
}

void HunspellInterface::SetAdditionalDirectory(const wchar_t* Dir) {
    SysDicDir = Dir;
    IsHunspellWorking = true;

    auto files = ListFiles(Dir, L"*.*", L"*.aff");
    if (files.empty())
        return;

    for (auto& file : files) {
        auto fileNameWithoutExt = file.substr(0, file.rfind (L'.'));
        if (PathFileExists((fileNameWithoutExt + L".dic").c_str ())) {
            AvailableLangInfo NewX;
            NewX.type = 1;
            NewX.name = fileNameWithoutExt.substr(fileNameWithoutExt.rfind (L'\\') + 1);
            if (dicList.count(NewX) == 0)
                dicList.insert(NewX);
        }
    }
    // Now we have 2 dictionaries on our hands

    // Reading system path unified dic too
    SystemWrongDicPath = Dir;
    if (SystemWrongDicPath.back () != L'\\')
        SystemWrongDicPath += L"\\";
    SystemWrongDicPath += L"UserDic.dic"; // Should be tunable really
    ReadUserDic(&Memorized,
                SystemWrongDicPath.c_str ()); // We should load user dictionary first.
}

bool HunspellInterface::GetLangOnlySystem(const wchar_t* Lang) {
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
