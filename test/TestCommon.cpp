#include "MockSpeller.h"

void setup_speller(MockSpeller &speller) {
  speller.set_inner_dict({{L"English",
                           {L"This", L"is", L"test", L"document", L"Please",
                            L"bear", L"with", L"me", L"И", L"ещё", L"немного", L"слов"}}});
}