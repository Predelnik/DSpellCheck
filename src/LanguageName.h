#ifndef LANGUAGE_NAME_H
#define LANGUAGE_NAME_H
#include "CommonFunctions.h"

struct LanguageName
{
  TCHAR *OrigName;
  TCHAR *AliasName;
  BOOL AliasApplied;
  LanguageName (TCHAR *Name, BOOL UseAlias = TRUE)
  {
    OrigName = 0;
    AliasName = 0;
    AliasApplied = FALSE;
    SetString (OrigName, Name);
    if (UseAlias)
      AliasApplied = SetStringWithAliasApplied (AliasName, Name);
    else
      SetString (OrigName, Name);
  }

  LanguageName (const LanguageName &rhs)
  {
    OrigName = 0;
    AliasName = 0;
    SetString (OrigName, rhs.OrigName);
    SetString (AliasName, rhs.AliasName);
    AliasApplied = rhs.AliasApplied;
  }

  LanguageName &operator = (const LanguageName &rhs)
  {
    if (&rhs == this)
      return *this;

    CLEAN_AND_ZERO_ARR (OrigName);
    CLEAN_AND_ZERO_ARR (AliasName);
    AliasApplied = rhs.AliasApplied;
    SetString (OrigName, rhs.OrigName);
    SetString (AliasName, rhs.AliasName);
    return *this;
  }

  ~LanguageName ()
  {
    CLEAN_AND_ZERO_ARR (OrigName);
    CLEAN_AND_ZERO_ARR (AliasName);
    AliasApplied = FALSE;
  }
};

inline bool operator <(LanguageName &a, LanguageName &b)
{
  return _tcscmp (a.AliasName, b.AliasName) < 0;
}
#endif // LANGUAGE_NAME_H