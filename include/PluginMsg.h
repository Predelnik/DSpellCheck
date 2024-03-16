#pragma once

#define DSPELLCHECK_SETLANG_MSG 1

struct DSpellCheckSetLangMsgInfo
{
  const wchar_t *lang_name;
  bool *was_success; // optional out param, pointed bool set to true if language was switched
};
