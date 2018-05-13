#include "MappedWstring.h"

void MappedWstring::append(const MappedWstring &other) {
  if (!str.empty() && !other.str.empty())
    str.push_back(L'\n');
  str.insert(str.end(), other.str.begin(), other.str.end());
  mapping.insert(mapping.end(), other.mapping.begin(), other.mapping.end());
}
