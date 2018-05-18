// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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

#if defined(DSPELLCHECK_NEW_SDK) && !defined(__clang__)
#include "NativeSpellerInterface.h"
#include "CommonFunctions.h"
#include "LanguageInfo.h"

#include <SpellCheck.h>
#include <comdef.h>
#include <iostream>

class ComException : public std::runtime_error {
public:
  ComException(HRESULT code)
      : std::runtime_error(to_string(_com_error(code).ErrorMessage())),
        code(code) {}

public:
  HRESULT code;
};

inline void HR(HRESULT const result) {
  if (S_OK != result && S_FALSE != result/* false is not an error */)
    throw ComException{result};
}

void NativeSpellerInterface::init_impl() {
  try {
    HR(CoInitializeEx(nullptr, // reserved
                      COINIT_APARTMENTTHREADED));
    m_co_initialize_succesfull = true;
  } catch (const ComException &e) {
    if (e.code != RPC_E_CHANGED_MODE)
      throw e;
  }

  HR(CoCreateInstance(__uuidof(SpellCheckerFactory), nullptr,
                      CLSCTX_INPROC_SERVER, __uuidof(m_ptrs->m_factory),
                      reinterpret_cast<void **>(&m_ptrs->m_factory)));
}

void NativeSpellerInterface::init() {
  if (m_inited)
    return;

  m_inited = true;

  try {
    init_impl();
  } catch (std::exception e) {
    return;
  }
  m_ok = true;
}

NativeSpellerInterface::NativeSpellerInterface() {
  m_ptrs = std::make_unique<ptrs>();
}

void NativeSpellerInterface::set_language(const wchar_t *lang) {
  if (!m_ok)
    return;
  try {
    m_ptrs->m_speller.Release();
    HR(m_ptrs->m_factory->CreateSpellChecker(lang, &m_ptrs->m_speller));
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
}

static std::vector<bool>
check_words_by_speller(ISpellChecker *speller,
                       const std::vector<WordForSpeller> &words) {
  if (!speller)
    return {};
  std::wstring w;
  std::vector<bool> ret(words.size(), true);
  std::vector<std::array<int, 2>> coords;
  for (int i = 0; i < static_cast<int>(words.size()); ++i) {
    if (i == 0 ||
        lstrcmpi(words[i].str.c_str(), words[i - 1].str.c_str()) != 0) {
      coords.push_back({{static_cast<int>(w.length()), i}});
      w += words[i].str;
      if (words[i].data.ends_with_dot)
        w += L".";
      w += L" ";
    } else
      coords.push_back({{coords.back()[0], i}});
  }
  CComPtr<IEnumSpellingError> err_enum;
  if (speller->Check(w.c_str(), &err_enum) != S_OK)
    return {};
  auto it = coords.begin();
  while (true) {
    CComPtr<ISpellingError> err;
    if (err_enum->Next(&err) != S_OK)
      break;
    ULONG si;
    err->get_StartIndex(&si);
    it = std::lower_bound(it, coords.end(), static_cast<int>(si),
                          [](auto &arr, auto &i) { return arr[0] < i; });
    ret[it - coords.begin()] = false;
  }
  return ret;
}

std::vector<bool> NativeSpellerInterface::check_words(
    const std::vector<WordForSpeller> &words) const {
  if (!m_ok)
    return {};

  switch (m_speller_mode) {
  case SpellerMode::SingleLanguage:
    return check_words_by_speller(m_ptrs->m_speller, words);
  case SpellerMode::MultipleLanguages: {
    std::vector<bool> ret(words.size(), false);
    for (auto speller : m_ptrs->m_spellers) {
      auto cur_res = check_words_by_speller(speller, words);
      if (!cur_res.empty())
        for (int i = 0; i < static_cast<int>(ret.size()); ++i)
          ret[i] = ret[i] || cur_res[i];
    }
    return ret;
  }
  }
  return {};
}

void NativeSpellerInterface::add_to_dictionary(const wchar_t *word) {
  if (!m_ok || !m_last_used_speller)
    return;

  m_last_used_speller->Add(word);
}

void NativeSpellerInterface::ignore_all(const wchar_t *word) {
  if (!m_ok || !m_last_used_speller)
    return;

  m_last_used_speller->Ignore(word);
}

bool NativeSpellerInterface::is_working() const { return m_ok; }

std::vector<LanguageInfo> NativeSpellerInterface::get_language_list() const {
  CComPtr<IEnumString> ptr;
  m_ptrs->m_factory->get_SupportedLanguages(&ptr);
  ULONG fetched;
  wchar_t *ws;
  std::vector<LanguageInfo> res;
  while (true) {
    ptr->Next(1, &ws, &fetched);
    if (fetched == 0)
      break;
    res.push_back(LanguageInfo(ws));
  }
  return res;
}

void NativeSpellerInterface::set_multiple_languages(
    const std::vector<std::wstring> &list) {
  if (!m_ok)
    return;

  try {
    m_ptrs->m_spellers.clear();
    for (auto &lang : list) {
      m_ptrs->m_spellers.emplace_back();
      HR(m_ptrs->m_factory->CreateSpellChecker(lang.c_str(),
                                               &m_ptrs->m_spellers.back()));
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }
}

static std::vector<std::wstring> get_speller_suggestions(ISpellChecker *speller,
                                                         const wchar_t *word) {
  if (!speller)
    return {};

  CComPtr<IEnumString> suggestions;
  speller->Suggest(word, &suggestions);
  std::vector<std::wstring> ret;
  wchar_t *ws;
  ULONG fetched;
  while (true) {
    suggestions->Next(1, &ws, &fetched);
    if (fetched == 0)
      break;
    ret.push_back(ws);
  }
  return ret;
}

std::vector<std::wstring>
NativeSpellerInterface::get_suggestions(const wchar_t *word) const {
  if (!m_ok)
    return {};

  switch (m_speller_mode) {
  case SpellerMode::SingleLanguage:
    m_last_used_speller = m_ptrs->m_speller;
    return get_speller_suggestions(m_ptrs->m_speller, word);
  case SpellerMode::MultipleLanguages: {
    std::vector<std::wstring> longest;
    for (auto speller : m_ptrs->m_spellers) {
      auto list = get_speller_suggestions(speller, word);
      if (list.size() > longest.size()) {
        longest = list;
        m_last_used_speller = speller;
      }
    }
    return longest;
  }
  }
  return {};
}

void NativeSpellerInterface::cleanup() {
  // This is slightly annoying detail that we cannot remove comptrs on program
  // exit and have to do it earlier In the future it could be resolved other way
  // if all the notifications from NPP will be processed as signals which will
  // be disconnected as the first cleanup step
  if (m_ok)
  {
    m_ok = false;
    m_ptrs.reset();
    if (m_co_initialize_succesfull)
      CoUninitialize ();
  }
}

#endif // defined (DSPELLCHECK_NEW_SDK) && !defined (__clang__)
