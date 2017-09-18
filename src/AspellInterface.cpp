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
#include "aspell.h"
#include "CommonFunctions.h"
#include "MainDef.h"
#include "Plugin.h"

AspellInterface::~AspellInterface() {
}

namespace
{
    auto wrapSpeller(AspellSpeller* rawPtr) {
        return std::unique_ptr<AspellSpeller, void (*)(AspellSpeller*)>
            {rawPtr, [](auto ptr) { delete_aspell_speller(ptr); }};
    }

    auto wrapConfig(AspellConfig* rawPtr) {
        return std::unique_ptr<AspellConfig, void (*)(AspellConfig*)>
            {rawPtr, [](auto ptr) { delete_aspell_config(ptr); }};
    }
}

AspellInterface::AspellInterface(HWND NppWindowArg) : singleSpeller(wrapSpeller(nullptr)) {
    NppWindow = NppWindowArg;
    LastSelectedSpeller = nullptr;
    AspellLoaded = false;
}

std::vector<std::wstring> AspellInterface::GetLanguageList() {
    if (!AspellLoaded)
        return {};

    auto aspCfg = wrapConfig(new_aspell_config());
    /* the returned pointer should _not_ need to be deleted */
    auto dlist = get_aspell_dict_info_list(aspCfg.get ());

    auto dels = aspell_dict_info_list_elements(dlist);

    if (aspell_dict_info_enumeration_at_end(dels)) {
        delete_aspell_dict_info_enumeration(dels);
        return {};
    }

    const AspellDictInfo* entry;
    std::vector<std::wstring> names;
    while ((entry = aspell_dict_info_enumeration_next(dels)) != nullptr) {
        names.push_back(to_wstring (entry->name));
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool AspellInterface::IsWorking() {
    return AspellLoaded;
}

void AspellInterface::SendAspellError(AspellCanHaveError* Error) {
    wchar_t* ErrorMsg = 0;
    SetString(ErrorMsg, aspell_error_message(Error));
    MessageBox(NppWindow, ErrorMsg, L"Aspell Error", MB_OK | MB_ICONEXCLAMATION);
}

void AspellInterface::SetMultipleLanguages(std::vector<wchar_t *>* List) {
    if (!AspellLoaded)
        return;

    spellers.clear();
    for (unsigned int i = 0; i < List->size(); i++) {
        AspellConfig* SpellConfig = new_aspell_config();
        aspell_config_replace(SpellConfig, "encoding", "utf-8");
        char* Buf = 0;
        SetString(Buf, List->at(i));
        aspell_config_replace(SpellConfig, "lang", Buf);
        CLEAN_AND_ZERO_ARR (Buf);
        AspellCanHaveError* PossibleErr = new_aspell_speller(SpellConfig);
        if (aspell_error_number(PossibleErr) == 0) {
            spellers.push_back(wrapSpeller(to_aspell_speller(PossibleErr)));
        }
        else
            SendAspellError(PossibleErr);

        delete_aspell_config(SpellConfig);
    }
}

std::vector<std::string> AspellInterface::GetSuggestions(const char* Word) {
    const AspellWordList* wordList = 0;
    std::string TargetWord;

    if (CurrentEncoding == ENCODING_UTF8)
        TargetWord = Word;
    else
        TargetWord = to_utf8_string (Word);

    std::vector<std::string> SuggList;
    if (!MultiMode) {
        LastSelectedSpeller = singleSpeller.get();
        wordList = aspell_speller_suggest(singleSpeller.get(), TargetWord.c_str (), -1);
    }
    else {
        unsigned int maxSize = 0;
        for (auto& speller : spellers) {
            const auto curWordList = aspell_speller_suggest(speller.get(), TargetWord.c_str (), -1);

            auto size = aspell_word_list_size(curWordList);
            if (size > maxSize) {
                maxSize = size;
                LastSelectedSpeller = speller.get();
                wordList = curWordList;
            }
        }
    }
    if (!wordList)
        return {};

    AspellStringEnumeration* els = aspell_word_list_elements(wordList);
    const char* Suggestion;

    while ((Suggestion = aspell_string_enumeration_next(els)) != nullptr) {
        SuggList.push_back(Suggestion);
    }
    return SuggList;
}

void AspellInterface::AddToDictionary(char* Word) {
    char* TargetWord = 0;

    if (CurrentEncoding == ENCODING_UTF8)
        TargetWord = Word;
    else
        SetStringDUtf8(TargetWord, Word);

    if (!LastSelectedSpeller)
        return;
    aspell_speller_add_to_personal(LastSelectedSpeller, Word, static_cast<int>(strlen(Word)) + 1);
    aspell_speller_save_all_word_lists(LastSelectedSpeller);
    if (aspell_speller_error(LastSelectedSpeller) != 0) {
        wchar_t* ErrorMsg = 0;
        SetString(ErrorMsg, aspell_speller_error_message(LastSelectedSpeller));
        MessageBox(NppWindow, ErrorMsg, L"Aspell Error", MB_OK | MB_ICONEXCLAMATION);
        CLEAN_AND_ZERO_ARR (ErrorMsg);
    }
    LastSelectedSpeller = 0;

    if (CurrentEncoding == ENCODING_ANSI)
        CLEAN_AND_ZERO_ARR (TargetWord);
}

void AspellInterface::IgnoreAll(char* Word) {
    if (!LastSelectedSpeller)
        return;

    char* TargetWord = 0;

    if (CurrentEncoding == ENCODING_UTF8)
        TargetWord = Word;
    else
        SetStringDUtf8(TargetWord, Word);

    aspell_speller_add_to_session(LastSelectedSpeller, TargetWord, static_cast<int>(strlen(TargetWord)) + 1);
    aspell_speller_save_all_word_lists(LastSelectedSpeller);
    if (aspell_speller_error(LastSelectedSpeller) != 0) {
        AspellErrorMsgBox(0, aspell_speller_error_message(LastSelectedSpeller));
    }
    LastSelectedSpeller = 0;
    if (CurrentEncoding == ENCODING_ANSI)
        CLEAN_AND_ZERO_ARR (TargetWord);
}

bool AspellInterface::CheckWord(char* Word) {
    if (!AspellLoaded)
        return true;

    char* DstWord = 0;
    bool res = false;
    if (CurrentEncoding == ENCODING_UTF8)
        DstWord = Word;
    else
        SetStringDUtf8(DstWord, Word);

    auto Len = static_cast<int>(strlen(DstWord));
    if (!MultiMode) {
        if (!singleSpeller)
            return true;

        res = aspell_speller_check(singleSpeller.get(), DstWord, Len);
    }
    else {
        if (spellers.empty())
            return true;

        for (auto& speller : spellers) {
            res = res || aspell_speller_check(speller.get(), DstWord, Len);
            if (res)
                break;
        }
    }
    if (CurrentEncoding != ENCODING_UTF8)
        CLEAN_AND_ZERO_ARR (DstWord);

    return res;
}

bool AspellInterface::Init(wchar_t* PathArg) {
    return (AspellLoaded = LoadAspell(PathArg));
}

void AspellInterface::SetLanguage(const wchar_t* Lang) {
    if (!AspellLoaded)
        return;

    auto spellConfig = wrapConfig(new_aspell_config());
    aspell_config_replace(spellConfig.get(), "encoding", "utf-8");
    char* Buf = 0;
    SetString(Buf, Lang);
    aspell_config_replace(spellConfig.get(), "lang", Buf);
    CLEAN_AND_ZERO_ARR (Buf);
    singleSpeller.reset();

    AspellCanHaveError* PossibleErr = new_aspell_speller(spellConfig.get());

    if (aspell_error_number(PossibleErr) != 0) {
        auto LangList = GetLanguageList();
        if (!LangList.empty () && (!Lang || Lang != LangList.front ())) {
            SetLanguage(LangList.front ().c_str ());
        }
        else {
            singleSpeller.reset ();
            SendAspellError(PossibleErr);
        }
    }
    else
        singleSpeller = wrapSpeller(to_aspell_speller(PossibleErr));
}
