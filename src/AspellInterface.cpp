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

AspellInterface::AspellInterface(HWND NppWindowArg) : m_single_speller(wrapSpeller(nullptr)) {
    m_npp_window = NppWindowArg;
    m_last_selected_speller = nullptr;
    m_aspell_loaded = false;
}

std::vector<std::wstring> AspellInterface::get_language_list() {
    if (!m_aspell_loaded)
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

bool AspellInterface::is_working() {
    return m_aspell_loaded;
}

void AspellInterface::SendAspellError(AspellCanHaveError* Error) {
    MessageBox(m_npp_window, to_wstring (aspell_error_message(Error)).c_str (), L"Aspell Error", MB_OK | MB_ICONEXCLAMATION);
}

void AspellInterface::set_multiple_languages(const std::vector<std::wstring>& List) {
    if (!m_aspell_loaded)
        return;

    m_spellers.clear();
    for (auto &lang : List) {
        AspellConfig* SpellConfig = new_aspell_config();
        aspell_config_replace(SpellConfig, "encoding", "utf-8");
        aspell_config_replace(SpellConfig, "lang", to_string (lang.c_str ()).c_str ());
        AspellCanHaveError* PossibleErr = new_aspell_speller(SpellConfig);
        if (aspell_error_number(PossibleErr) == 0) {
            m_spellers.push_back(wrapSpeller(to_aspell_speller(PossibleErr)));
        }
        else
            SendAspellError(PossibleErr);

        delete_aspell_config(SpellConfig);
    }
}

std::vector<std::string> AspellInterface::get_suggestions(const char* Word) {
    const AspellWordList* wordList = nullptr;
    std::string TargetWord;

    if (m_current_encoding == ENCODING_UTF8)
        TargetWord = Word;
    else
        TargetWord = toUtf8String (Word);

    std::vector<std::string> SuggList;
    if (!m_multi_mode) {
        m_last_selected_speller = m_single_speller.get();
        wordList = aspell_speller_suggest(m_single_speller.get(), TargetWord.c_str (), -1);
    }
    else {
        unsigned int maxSize = 0;
        for (auto& speller : m_spellers) {
            const auto curWordList = aspell_speller_suggest(speller.get(), TargetWord.c_str (), -1);

            auto size = aspell_word_list_size(curWordList);
            if (size > maxSize) {
                maxSize = size;
                m_last_selected_speller = speller.get();
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

void AspellInterface::add_to_dictionary(const char* Word) {
    std::string TargetWord;

    if (m_current_encoding == ENCODING_UTF8)
        TargetWord = Word;
    else
        TargetWord = toUtf8String (Word);

    if (!m_last_selected_speller)
        return;
    aspell_speller_add_to_personal(m_last_selected_speller, TargetWord.c_str(), static_cast<int>(TargetWord.length()) + 1);
    aspell_speller_save_all_word_lists(m_last_selected_speller);
    if (aspell_speller_error(m_last_selected_speller) != nullptr) {
        MessageBox(m_npp_window, to_wstring (aspell_speller_error_message(m_last_selected_speller)).c_str (),
          L"Aspell Error", MB_OK | MB_ICONEXCLAMATION);

    }
    m_last_selected_speller = nullptr;

}

void AspellInterface::ignore_all(const char* Word) {
    if (!m_last_selected_speller)
        return;

    std::string TargetWord;

    if (m_current_encoding == ENCODING_UTF8)
        TargetWord = Word;
    else
        TargetWord = toUtf8String(Word);

    aspell_speller_add_to_session(m_last_selected_speller, TargetWord.c_str (), static_cast<int> (TargetWord.length ()) + 1);
    aspell_speller_save_all_word_lists(m_last_selected_speller);
    if (aspell_speller_error(m_last_selected_speller) != nullptr) {
        AspellErrorMsgBox(nullptr, aspell_speller_error_message(m_last_selected_speller));
    }
    m_last_selected_speller = nullptr;
}

bool AspellInterface::check_word(const char* Word) {
    if (!m_aspell_loaded)
        return true;

    std::string DstWord;
    bool res = false;
    if (m_current_encoding == ENCODING_UTF8)
        DstWord = Word;
    else
        DstWord = toUtf8String(Word);

    auto Len = static_cast<int> (DstWord.length());
    if (!m_multi_mode) {
        if (!m_single_speller)
            return true;

        res = aspell_speller_check(m_single_speller.get(), DstWord.c_str (), Len);
    }
    else {
        if (m_spellers.empty())
            return true;

        for (auto& speller : m_spellers) {
            res = res || aspell_speller_check(speller.get(), DstWord.c_str (), Len);
            if (res)
                break;
        }
    }
    return res;
}

bool AspellInterface::Init(const wchar_t* PathArg) {
    return (m_aspell_loaded = LoadAspell(PathArg));
}

void AspellInterface::set_language(const wchar_t* Lang) {
    if (!m_aspell_loaded)
        return;

    auto spellConfig = wrapConfig(new_aspell_config());
    aspell_config_replace(spellConfig.get(), "encoding", "utf-8");
    aspell_config_replace(spellConfig.get(), "lang", to_string (Lang).c_str ());
    m_single_speller.reset();

    AspellCanHaveError* PossibleErr = new_aspell_speller(spellConfig.get());

    if (aspell_error_number(PossibleErr) != 0) {
        auto LangList = get_language_list();
        if (!LangList.empty () && (!Lang || Lang != LangList.front ())) {
            set_language(LangList.front ().c_str ());
        }
        else {
            m_single_speller.reset ();
            SendAspellError(PossibleErr);
        }
    }
    else
        m_single_speller = wrapSpeller(to_aspell_speller(PossibleErr));
}
