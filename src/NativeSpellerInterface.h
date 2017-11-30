#pragma once

#include "SpellerInterface.h"

#ifdef DSPELLCHECK_NEW_SDK
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
    std::vector<bool> check_words(const std::vector<const wchar_t*>& words) override;
    void add_to_dictionary(const wchar_t* word) override;
    void ignore_all(const wchar_t* word) override;
    bool is_working() const override;
    std::vector<LanguageInfo> get_language_list() const override;
    void set_multiple_languages(const std::vector<std::wstring>& list) override;
    std::vector<std::wstring> get_suggestions(const wchar_t* word) override;
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
    ISpellChecker *last_used_speller = nullptr;
    bool m_ok = false;
    bool m_inited = false;
};
#else
class NativeSpellerInterface : public DummySpeller
{
    void cleanup ();
};
#endif
