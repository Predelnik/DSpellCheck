// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#pragma once

#include "SpellerInterface.h"

#if !defined (__clang__)
#include <atlbase.h>

struct ISpellCheckerFactory;
struct ISpellChecker;
struct IEnumSpellingError;

class NativeSpellerInterface : public SpellerInterface
{
public:
    void init (); // init should be done after initialization, not after construction
    NativeSpellerInterface (const Settings &settings);
    void set_language(const wchar_t* lang) override;
    std::vector<bool> check_words(const std::vector<WordForSpeller>& words) const override;
    void add_to_dictionary(const wchar_t* word) override;
    void ignore_all(const wchar_t* word) override;
    bool is_working() const override;
    std::vector<LanguageInfo> get_language_list() const override;
    void set_multiple_languages(const std::vector<std::wstring>& list) override;
    std::vector<std::wstring> get_suggestions(const wchar_t* word) const override;
    void cleanup();

private:
    void init_impl();
private:
    class ptrs
    {
    public:
      CComPtr<ISpellCheckerFactory> m_factory;
      CComPtr<ISpellChecker> m_speller;
      std::vector<CComPtr<ISpellChecker>> m_spellers;
    };
    std::unique_ptr<ptrs> m_ptrs;
    mutable ISpellChecker *m_last_used_speller = nullptr;
    bool m_ok = false;
    bool m_init = false;
    bool m_co_initialize_successful = false;
    const Settings &m_settings;
};
#else
class NativeSpellerInterface : public DummySpeller
{
public:
    NativeSpellerInterface (const Settings &) {}
    void cleanup () {}
    void init () {}
};
#endif // defined (DSPELLCHECK_NEW_SDK) && !defined (__clang__)
