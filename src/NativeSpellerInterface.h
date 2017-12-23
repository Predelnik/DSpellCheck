#pragma once

#include "SpellerInterface.h"

#if defined (DSPELLCHECK_NEW_SDK) && !defined (__clang__)
#include <atlbase.h>

struct ISpellCheckerFactory;
struct ISpellChecker;
struct IEnumSpellingError;

class NativeSpellerInterface : public SpellerInterface
{
public:
    void init (); // init should be done after initialization, not after construction
    NativeSpellerInterface ();
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
    struct ptrs
    {
    CComPtr<ISpellCheckerFactory> m_factory;
    CComPtr<ISpellChecker> m_speller;
    std::vector<CComPtr<ISpellChecker>> m_spellers;
    };
    std::unique_ptr<ptrs> m_ptrs;
    mutable ISpellChecker *m_last_used_speller = nullptr;
    bool m_ok = false;
    bool m_inited = false;
};
#else
class NativeSpellerInterface : public DummySpeller
{
public:
    void cleanup () {}
    void init () {}
};
#endif // defined (DSPELLCHECK_NEW_SDK) && !defined (__clang__)
