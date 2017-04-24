#include "LanguageName.h"
#include "Settings.h"

std::wstring LanguageName::getLanguageName(const SettingsData &settings, bool forRemoveDialog /*= false*/) const
{
  return (settings.useLanguageNameAliases ? aliasName : originalName) + (forRemoveDialog ? (systemOnly ? L" [!For All Users]" : L"") : L"");
}
