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
    catch (const ComException &e)
    {
        if (e.code != RPC_E_CHANGED_MODE)
            throw e;
        
        m_com_initialized = false;
    }

    HR(CoCreateInstance(__uuidof(SpellCheckerFactory),
                        nullptr,
                        CLSCTX_INPROC_SERVER,
                        __uuidof(m_ptrs->m_factory),
                        reinterpret_cast<void **>(m_ptrs->m_factory.GetAddressOf())));
}

void NativeSpellerInterface::init()
{
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
    m_ptrs = std::make_unique<ptrs> ();
}

void NativeSpellerInterface::set_language(const wchar_t* lang)
{
  if (!m_ok)
      return;
  try
  {
    HR (m_ptrs->m_factory->CreateSpellChecker(lang, m_ptrs->m_speller.GetAddressOf()));
  }
  catch (const std::exception &e)
  {
      std::cerr << e.what () << '\n';
  }
}

bool NativeSpellerInterface::check_word(const wchar_t *word)
{
    OutputDebugString(L"check_word\n");
    if (!m_ok || !m_ptrs->m_speller)
      return true;
    m_ptrs->m_speller->Check(word, m_ptrs->m_err.GetAddressOf());
    Microsoft::WRL::ComPtr<ISpellingError> dummy;
    return m_ptrs->m_err->Next(dummy.GetAddressOf()) != S_OK; // Check that error list is empty
}

void NativeSpellerInterface::add_to_dictionary(const wchar_t* /*word*/)
{
}

void NativeSpellerInterface::ignore_all(const wchar_t* /*word*/)
{
}

bool NativeSpellerInterface::is_working() const
{
    return m_ok;
}

std::vector<LanguageInfo> NativeSpellerInterface::get_language_list() const
{
    Microsoft::WRL::ComPtr<IEnumString> ptr;
    m_ptrs->m_factory->get_SupportedLanguages(ptr.GetAddressOf());
    ULONG fetched;
    wchar_t *ws;
    std::vector<LanguageInfo> res;
    while (true)
    {
        ptr->Next(1, &ws, &fetched);
        if (fetched == 0)
            break;
        res.push_back (LanguageInfo (ws));
    }
    return res;
}

void NativeSpellerInterface::set_multiple_languages(const std::vector<std::wstring>& /*list*/)
{
}

std::vector<std::wstring> NativeSpellerInterface::get_suggestions(const wchar_t* /*word*/)
{
    return {};
}

void NativeSpellerInterface::cleanup()
{
    // This is slightly annoying detail that we cannot remove comptrs on program exit
    // and have to do it earlier
    // In the future it could be resolved other way if all the notifications from NPP
    // will be processed as signals which will be disconnected as the first cleanup step
    m_ok = false;
    m_ptrs.reset ();
}
