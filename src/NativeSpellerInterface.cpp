#ifdef DSPELLCHECK_NEW_SDK
#include "NativeSpellerInterface.h"
#include "CommonFunctions.h"
#include "LanguageInfo.h"

#include <comdef.h>
#include <SpellCheck.h>
#include <iostream>

class ComException : public std::runtime_error
{
public:
    ComException(HRESULT code) : std::runtime_error(to_string(_com_error(code).ErrorMessage())), code(code)
    {
    }

public:
    HRESULT code;
};

inline void HR(HRESULT const result)
{
    if (S_OK != result) throw ComException{result};
}

void NativeSpellerInterface::init_impl()
{
    try
    {
        HR(CoInitializeEx(nullptr, // reserved
                          COINIT_MULTITHREADED));
    }
    catch (const ComException& e)
    {
        if (e.code != RPC_E_CHANGED_MODE)
            throw e;
    }

    HR(CoCreateInstance(__uuidof(SpellCheckerFactory),
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        __uuidof(m_ptrs->m_factory),
                        reinterpret_cast<void **>(&m_ptrs->m_factory)));
}

void NativeSpellerInterface::init()
{
    if (m_inited)
        return;

    m_inited = true;

    try
    {
        init_impl();
    }
    catch (std::exception e)
    {
        return;
    }
    m_ok = true;
}

NativeSpellerInterface::NativeSpellerInterface()
{
    m_ptrs = std::make_unique<ptrs>();
}

void NativeSpellerInterface::set_language(const wchar_t* lang)
{
    if (!m_ok)
        return;
    try
    {
        m_ptrs->m_speller.Release();
        HR(m_ptrs->m_factory->CreateSpellChecker(lang, &m_ptrs->m_speller));
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

std::vector<bool> NativeSpellerInterface::check_words(const std::vector<const wchar_t*>& words)
{
    if (!m_ok || !m_ptrs->m_speller)
        return {};
    std::wstring w;
    std::vector<bool> ret(words.size(), true);
    std::vector<std::array<int, 2>> coords;
    for (int i = 0; i < static_cast<int>(words.size()); ++i)
    {
        coords.push_back({static_cast<int>(w.length()), i});
        w += words[i];
        w += L" ";
    }
    CComPtr<IEnumSpellingError> err_enum;
    if (m_ptrs->m_speller->Check(w.c_str(), &err_enum) != S_OK)
        return {};
    auto it = coords.begin();
    while (true)
    {
        CComPtr<ISpellingError> err;
        if (err_enum->Next(&err) != S_OK)
            break;
        ULONG si;
        err->get_StartIndex(&si);
        it = std::lower_bound(it, coords.end(), static_cast<int>(si), [](auto& arr, auto& i) { return arr[0] < i; });
        ret[it - coords.begin()] = false;
    }
    return ret;
}

void NativeSpellerInterface::add_to_dictionary(const wchar_t* word)
{
    if (!m_ok || !m_ptrs->m_speller)
      return;

    m_ptrs->m_speller->Add(word);
}

void NativeSpellerInterface::ignore_all(const wchar_t* word)
{
   if (!m_ok || !m_ptrs->m_speller)
      return;

    m_ptrs->m_speller->Ignore(word);
}

bool NativeSpellerInterface::is_working() const
{
    return m_ok;
}

std::vector<LanguageInfo> NativeSpellerInterface::get_language_list() const
{
    CComPtr<IEnumString> ptr;
    m_ptrs->m_factory->get_SupportedLanguages(&ptr);
    ULONG fetched;
    wchar_t* ws;
    std::vector<LanguageInfo> res;
    while (true)
    {
        ptr->Next(1, &ws, &fetched);
        if (fetched == 0)
            break;
        res.push_back(LanguageInfo(ws));
    }
    return res;
}

void NativeSpellerInterface::set_multiple_languages(const std::vector<std::wstring>& /*list*/)
{
}

std::vector<std::wstring> NativeSpellerInterface::get_suggestions(const wchar_t* word)
{
    if (!m_ok || !m_ptrs->m_speller)
        return {};

    CComPtr<IEnumString> suggestions;
    m_ptrs->m_speller->Suggest(word, &suggestions);
    std::vector<std::wstring> ret;
    wchar_t* ws;
    ULONG fetched;
    while (true)
    {
        suggestions->Next(1, &ws, &fetched);
        if (fetched == 0)
            break;
        ret.push_back (ws);
    }
    return ret;
}

void NativeSpellerInterface::cleanup()
{
    // This is slightly annoying detail that we cannot remove comptrs on program exit
    // and have to do it earlier
    // In the future it could be resolved other way if all the notifications from NPP
    // will be processed as signals which will be disconnected as the first cleanup step
    m_ok = false;
    m_ptrs.reset();
}
#endif // DSPELLCHECK_NEW_SDK
