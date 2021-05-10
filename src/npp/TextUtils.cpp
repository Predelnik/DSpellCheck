#include "npp/TextUtils.h"

#include "common/CommonFunctions.h"
#include "common/utf8.h"

MappedWstring utf8_to_mapped_wstring(std::string_view str) {
  if (str.empty())
    return {};
  ptrdiff_t len = str.length();
  std::vector<wchar_t> buf;
  std::vector<TextPosition> mapping;
  buf.reserve(len);
  mapping.reserve(len);
  auto it = str.data();
  // sadly this garbage skipping is required due to bad find prev mistake algorithm
  while (utf8_is_cont(*it))
    ++it;
  size_t char_cnt = 1;
  while (it - str.data() < len) {
    auto next = utf8_inc(it);
    buf.resize(char_cnt);
    MultiByteToWideChar(CP_UTF8, 0, it, static_cast<int>(next - it), buf.data() + char_cnt - 1, 1);
    mapping.push_back(static_cast<TextPosition>(it - str.data()));
    ++char_cnt;
    it = next;
  }
  mapping.push_back(static_cast<TextPosition>(it - str.data()));
  return {std::wstring{buf.begin(), buf.end()}, std::move(mapping)};
}

MappedWstring to_mapped_wstring(std::string_view str) {
  if (str.empty())
    return {};
  std::vector<TextPosition> mapping(str.length() + 1);
  std::iota(mapping.begin(), mapping.end(), 0);
  return {to_wstring(str), std::move(mapping)};
}
