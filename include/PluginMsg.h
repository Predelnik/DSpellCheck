#pragma once

#define DSPELLCHECK_SETLANG_MSG 1         // See DSpellCheckSetLangMsgInfo
#define DSPELLCHECK_GETLANGUAGELIST_MSG 2 // See DSpellCheckGetLanguageListMsgInfo
#define DSPELLCHECK_ENABLESPELLCHECK_MSG 3 // See DSpellCheckEnableSpellCheckMsgInfo

// Set language to lang_name, if was_success non-zero, it will be set to true in case of success and fales in case of failure
struct DSpellCheckSetLangMsgInfo {
  const wchar_t *lang_name;
  bool *was_success; // optional out param, pointed bool set to true if language was switched
};

// language_callback will be called once for each language with payload provided as a second struct element
struct DSpellCheckGetLanguageListMsgInfo {
  void (*language_callback)(void *payload, const wchar_t *lang_name);
  void *payload;
};

struct DSpellCheckEnableSpellCheckMsgInfo {
  bool enabled;
};
