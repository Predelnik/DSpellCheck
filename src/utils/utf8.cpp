#include "utf8.h"

bool utf8_is_lead(char c) {
  return (((c & 0x80) == 0)                          // 0xxxxxxx
          || ((c & 0xC0) == 0xC0 && (c & 0x20) == 0) // 110xxxxx
          || ((c & 0xE0) == 0xE0 && (c & 0x10) == 0) // 1110xxxx
          || ((c & 0xF0) == 0xF0 && (c & 0x08) == 0) // 11110xxx
          || ((c & 0xF8) == 0xF8 && (c & 0x04) == 0) // 111110xx
          || ((c & 0xFC) == 0xFC && (c & 0x02) == 0));
}

bool utf8_is_cont(char c) {
  return ((c & 0x80) == 0x80 && (c & 0x40) == 0); // 10xxxxx
}

char *utf8_dec(const char *string, const char *current) {
  if (string >= current)
    return nullptr;

  const char *temp = current - 1;
  while (string <= temp && (!utf8_is_lead(*temp)))
    temp--;

  return const_cast<char *>(temp);
}

const char *utf8_inc(const char *string) {
  return string + utf8_symbol_len(*string);
}

char *utf8_inc(char *string) { return string + utf8_symbol_len(*string); }

int utf8_symbol_len(char c) {
  if ((c & 0x80) == 0)
    return 1;
  if ((c & 0xC0) > 0 && (c & 0x20) == 0)
    return 2;
  if ((c & 0xE0) > 0 && (c & 0x10) == 0)
    return 3;
  if ((c & 0xF0) > 0 && (c & 0x08) == 0)
    return 4;
  if ((c & 0xF8) > 0 && (c & 0x04) == 0)
    return 5;
  if ((c & 0xFC) > 0 && (c & 0x02) == 0)
    return 6;
  return 0;
}

bool utf8_first_chars_equal(const char *str1, const char *str2) {
  int char1_size = utf8_symbol_len(*str1);
  int char2_size = utf8_symbol_len(*str2);
  if (char1_size != char2_size)
    return false;
  return (strncmp(str1, str2, char1_size) == 0);
}

const char *utf8_pbrk(const char *s, const char *set) {
  for (; *s != 0; s = utf8_inc(s))
    for (auto x = set; *x != 0; x = utf8_inc(x))
      if (utf8_first_chars_equal(s, x))
        return s;
  return nullptr;
}

char *utf8_chr(const char *s, const char *sfc) // Char is first from the string
// sfc (string with first char)
{
  while (*s != 0) {
    if ((s != nullptr) && utf8_first_chars_equal(s, sfc))
      return const_cast<char *>(s);
    s = utf8_inc(s);
  }
  return nullptr;
}

size_t utf8_length(const char *string) {
  auto it = string;
  size_t size = 0;
  while (*it != 0) {
    size++;
    it = utf8_inc(it);
  }
  return size;
}
