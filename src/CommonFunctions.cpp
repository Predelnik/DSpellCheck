// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "CommonFunctions.h"
#include "MainDefs.h"
#include "MappedWString.h"
#include "Plugin.h"
#include "Settings.h"
#include "iconv.h"
#include "resource.h"
#include "utils/string_utils.h"
#include "utils/utf8.h"
#include "utils/winapi.h"
#include <cassert>
#include <numeric>

static std::vector<char> convert(const char *source_enc, const char *target_enc, const void *source_data_ptr, size_t source_len, size_t max_dest_len) {
  std::vector<char> buf(max_dest_len);
  auto out_buf = reinterpret_cast<char *>(buf.data());
  iconv_t converter = iconv_open(target_enc, source_enc);
  auto char_ptr = static_cast<const char *>(source_data_ptr);
  size_t res = iconv(converter, static_cast<const char **>(&char_ptr), &source_len, &out_buf, &max_dest_len);
  iconv_close(converter);

  if (res == static_cast<size_t>(-1))
    return {};
  return buf;
}

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

std::wstring to_wstring(std::string_view source) {
  auto bytes = convert("CHAR", "UCS-2LE//IGNORE", source.data(), source.length(), sizeof(wchar_t) * (source.length() + 1));
  if (bytes.empty())
    return {};
  return reinterpret_cast<wchar_t *>(bytes.data());
}

std::string to_string(std::wstring_view source) {
  auto bytes = convert("UCS-2LE", "CHAR//IGNORE", source.data(), (source.length()) * sizeof(wchar_t), sizeof(wchar_t) * (source.length() + 1));
  if (bytes.empty())
    return {};
  return bytes.data();
}

std::string to_utf8_string(std::string_view source) {
  auto bytes = convert("CHAR", "UTF-8//IGNORE", source.data(), source.length(), max_utf8_char_length * (source.length() + 1));
  if (bytes.empty())
    return {};
  return bytes.data();
}

std::string to_utf8_string(std::wstring_view source) {
  auto bytes = convert("UCS-2LE", "UTF-8//IGNORE", source.data(), source.length() * sizeof(wchar_t), max_utf8_char_length * (source.length() + 1));
  if (bytes.empty())
    return {};
  return bytes.data();
}

std::wstring utf8_to_wstring(const char *source) {
  auto bytes = convert("UTF-8", "UCS-2LE//IGNORE", source, strlen(source), (utf8_length(source) + 1) * sizeof(wchar_t));
  if (bytes.empty())
    return {};
  return reinterpret_cast<const wchar_t *>(bytes.data());
}

std::string utf8_to_string(const char *source) {
  auto bytes = convert("UTF-8", "CHAR//IGNORE", source, strlen(source), utf8_length(source) + 1);
  if (bytes.empty())
    return {};
  return bytes.data();
}

// This function is more or less transferred from gcc source
bool match_special_char(wchar_t *dest, const wchar_t *&source) {
  wchar_t c;

  bool m = true;
  auto assign = [dest](wchar_t c) {
    if (dest != nullptr)
      *dest = c;
  };

  switch (c = *(source++)) {
  case L'a':
    assign(L'\a');
    break;
  case L'b':
    assign(L'\b');
    break;
  case L't':
    assign(L'\t');
    break;
  case L'f':
    assign(L'\f');
    break;
  case L'n':
    assign(L'\n');
    break;
  case L'r':
    assign(L'\r');
    break;
  case L'v':
    assign(L'\v');
    break;
  case L'\\':
    assign(L'\\');
    break;
  case L'0':
    assign(L'\0');
    break;

  case L'x':
  case L'u':
  case L'U': {
    /* Hexadecimal form of wide characters.  */
    int len = (c == L'x' ? 2 : (c == L'u' ? 4 : 8));
    wchar_t n = 0;
#ifndef UNICODE
    len = 2;
#endif
    for (int i = 0; i < len; i++) {
      wchar_t buf[2] = {L'\0', L'\0'};

      c = *(source++);
      if (c > UCHAR_MAX || !((L'0' <= c && c <= L'9') || (L'a' <= c && c <= L'f') || (L'A' <= c && c <= L'F')))
        return false;

      buf[0] = static_cast<wchar_t>(c);
      n = n << 4;
      n += static_cast<wchar_t>(wcstol(buf, nullptr, 16));
    }
    assign(n);
    break;
  }

  default:
    /* Unknown backslash codes are simply not expanded.  */
    m = false;
    break;
  }
  return m;
}

static size_t do_parse_string(wchar_t *dest, const wchar_t *source) {
  bool dry_run = dest == nullptr;
  wchar_t *res_string = dest;
  while (*source != L'\0') {
    const wchar_t *last_pos = source;
    if (*source == '\\') {
      ++source;
      if (!match_special_char(!dry_run ? dest : nullptr, source)) {
        source = last_pos;
        if (!dry_run)
          *dest = *source;
        ++source;
        ++dest;
      } else {
        ++dest;
      }
    } else {
      if (!dry_run)
        *dest = *source;
      ++source;
      ++dest;
    }
  }
  if (!dry_run)
    *dest = L'\0';
  return dest - res_string + 1;
}

std::wstring parse_string(const wchar_t *source) {
  auto size = do_parse_string(nullptr, source);
  std::vector<wchar_t> buf(size);
  do_parse_string(buf.data(), source);
  assert(size == buf.size());
  return buf.data();
}

// These functions are mostly taken from http://research.microsoft.com

static const std::unordered_map<std::wstring_view, std::wstring> alias_map = {{L"AF-ZA", L"Afrikaans"},
                                                                              {L"AK-GH", L"Akan"},
                                                                              {L"AN-ES", L"Aragonese"},
                                                                              {L"AR", L"Arabic"},
                                                                              {L"BE-BY", L"Belarusian"},
                                                                              {L"BN-BD", L"Bengali"},
                                                                              {L"BO", L"Classical Tibetan"},
                                                                              {L"BG-BG", L"Bulgarian"},
                                                                              {L"BR-FR", L"Breton"},
                                                                              {L"BS-BA", L"Bosnian"},
                                                                              {L"CA", L"Catalan"},
                                                                              {L"CA-VALENCIA", L"Catalan (Valencia)"},
                                                                              {L"CA-ANY", L"Catalan (Any)"},
                                                                              {L"CA-ES", L"Catalan (Spain)"},
                                                                              {L"COP-EG", L"Coptic (Bohairic dialect)"},
                                                                              {L"CS-CZ", L"Czech"},
                                                                              {L"CY-GB", L"Welsh"},
                                                                              {L"DA-DK", L"Danish"},
                                                                              {L"DE-AT", L"German (Austria)"},
                                                                              {L"DE-AT-FRAMI", L"German (Austria) [frami]"},
                                                                              {L"DE-CH", L"German (Switzerland)"},
                                                                              {L"DE-CH-FRAMI", L"German (Switzerland) [frami]"},
                                                                              {L"DE-DE", L"German (Germany)"},
                                                                              {L"DE-DE-COMB", L"German (Old and New Spelling)"},
                                                                              {L"DE-DE-FRAMI", L"German (Germany) [frami]"},
                                                                              {L"DE-DE-NEU", L"German (New Spelling)"},
                                                                              {L"EL-GR", L"Greek"},
                                                                              {L"EN-AU", L"English (Australia)"},
                                                                              {L"EN-CA", L"English (Canada)"},
                                                                              {L"EN-GB", L"English (Great Britain)"},
                                                                              {L"EN-GB-OED", L"English (Great Britain) [OED]"},
                                                                              {L"EN-NZ", L"English (New Zealand)"},
                                                                              {L"EN-US", L"English (United States)"},
                                                                              {L"EN-ZA", L"English (South Africa)"},
                                                                              {L"EO-EO", L"Esperanto"},
                                                                              {L"ES-ANY", L"Spanish"},
                                                                              {L"ES-AR", L"Spanish (Argentina)"},
                                                                              {L"ES-BO", L"Spanish (Bolivia)"},
                                                                              {L"ES-CL", L"Spanish (Chile)"},
                                                                              {L"ES-CO", L"Spanish (Colombia)"},
                                                                              {L"ES-CR", L"Spanish (Costa Rica)"},
                                                                              {L"ES-CU", L"Spanish (Cuba)"},
                                                                              {L"ES-DO", L"Spanish (Dominican Republic)"},
                                                                              {L"ES-EC", L"Spanish (Ecuador)"},
                                                                              {L"ES-ES", L"Spanish (Spain)"},
                                                                              {L"ES-GT", L"Spanish (Guatemala)"},
                                                                              {L"ES-HN", L"Spanish (Honduras)"},
                                                                              {L"ES-MX", L"Spanish (Mexico)"},
                                                                              {L"ES-NEW", L"Spanish (New)"},
                                                                              {L"ES-NI", L"Spanish (Nicaragua)"},
                                                                              {L"ES-PA", L"Spanish (Panama)"},
                                                                              {L"ES-PE", L"Spanish (Peru)"},
                                                                              {L"ES-PR", L"Spanish (Puerto Rico)"},
                                                                              {L"ES-PY", L"Spanish (Paraguay)"},
                                                                              {L"ES-SV", L"Spanish (El Salvador)"},
                                                                              {L"ES-UY", L"Spanish (Uruguay)"},
                                                                              {L"ES-VE", L"Spanish (Bolivarian Republic of Venezuela)"},
                                                                              {L"ET-EE", L"Estonian"},
                                                                              {L"FO-FO", L"Faroese"},
                                                                              {L"FR", L"French"},
                                                                              {L"FR-FR", L"French"},
                                                                              {L"FR-FR-1990", L"French (1990)"},
                                                                              {L"FR-FR-1990-1-3-2", L"French (1990)"},
                                                                              {L"FR-FR-CLASSIQUE", L"French (Classique)"},
                                                                              {L"FR-FR-CLASSIQUE-1-3-2", L"French (Classique)"},
                                                                              {L"FR-FR-1-3-2", L"French"},
                                                                              {L"FY-NL", L"Frisian"},
                                                                              {L"GA-IE", L"Irish"},
                                                                              {L"GD-GB", L"Scottish Gaelic"},
                                                                              {L"GL-ES", L"Galician"},
                                                                              {L"GU-IN", L"Gujarati"},
                                                                              {L"GUG", L"Guarani"},
                                                                              {L"HE-IL", L"Hebrew"},
                                                                              {L"HI-IN", L"Hindi"},
                                                                              {L"HIl-PH", L"Hiligaynon"},
                                                                              {L"HR-HR", L"Croatian"},
                                                                              {L"HU-HU", L"Hungarian"},
                                                                              {L"IA", L"Interlingua"},
                                                                              {L"ID-ID", L"Indonesian"},
                                                                              {L"IS", L"Icelandic"},
                                                                              {L"IS-IS", L"Icelandic"},
                                                                              {L"IT-IT", L"Italian"},
                                                                              {L"KMR-LATN", L"Kurdish (Latin)"},
                                                                              {L"KU-TR", L"Kurdish"},
                                                                              {L"LA", L"Latin"},
                                                                              {L"LO-LA", L"Lao"},
                                                                              {L"LT", L"Lithuanian"},
                                                                              {L"LV-LV", L"Latvian"},
                                                                              {L"MG-MG", L"Malagasy"},
                                                                              {L"MI-NZ", L"Maori"},
                                                                              {L"MK-MK", L"Macedonian (FYROM)"},
                                                                              {L"MOS-BF", L"Mossi"},
                                                                              {L"MR-IN", L"Marathi"},
                                                                              {L"MS-MY", L"Malay"},
                                                                              {L"NB-NO", L"Norwegian (Bokmal)"},
                                                                              {L"NE-NP", L"Nepali"},
                                                                              {L"NL-NL", L"Dutch"},
                                                                              {L"NN-NO", L"Norwegian (Nynorsk)"},
                                                                              {L"NR-ZA", L"Ndebele"},
                                                                              {L"NS-ZA", L"Northern Sotho"},
                                                                              {L"NY-MW", L"Chichewa"},
                                                                              {L"OC-FR", L"Occitan"},
                                                                              {L"PL-PL", L"Polish"},
                                                                              {L"PT-BR", L"Portuguese (Brazil)"},
                                                                              {L"PT-PT", L"Portuguese (Portugal)"},
                                                                              {L"RO-RO", L"Romanian"},
                                                                              {L"RU-RU", L"Russian"},
                                                                              {L"RU-RU-IE", L"Russian (without io)"},
                                                                              {L"RU-RU-YE", L"Russian (without io)"},
                                                                              {L"RU-RU-YO", L"Russian (with io)"},
                                                                              {L"RW-RW", L"Kinyarwanda"},
                                                                              {L"SI-LK", L"Sinhala"},
                                                                              {L"SI-SI", L"Slovenian"},
                                                                              {L"SK-SK", L"Slovak"},
                                                                              {L"SL-SI", L"Slovenian"},
                                                                              {L"SQ-AL", L"Alban"},
                                                                              {L"SR", L"Serbian (Cyrillic)"},
                                                                              {L"SR-LATN", L"Serbian (Latin)"},
                                                                              {L"SS-ZA", L"Swazi"},
                                                                              {L"ST-ZA", L"Northern Sotho"},
                                                                              {L"SV-SE", L"Swedish"},
                                                                              {L"SV-FI", L"Swedish (Finland)"},
                                                                              {L"SW-KE", L"Kiswahili"},
                                                                              {L"SW-TZ", L"Swahili"},
                                                                              {L"TE-IN", L"Telugu"},
                                                                              {L"TET-ID", L"Tetum"},
                                                                              {L"TH-TH", L"Thai"},
                                                                              {L"TL-PH", L"Tagalog"},
                                                                              {L"TN-ZA", L"Setswana"},
                                                                              {L"TS-ZA", L"Tsonga"},
                                                                              {L"UK-UA", L"Ukrainian"},
                                                                              {L"UR-PK", L"Urdu"},
                                                                              {L"VE-ZA", L"Venda"},
                                                                              {L"VI-VN", L"Vietnamese"},
                                                                              {L"XH-ZA", L"isiXhosa"},
                                                                              {L"ZU-ZA", L"isiZulu"}};
// Language Aliases
std::pair<std::wstring, bool> apply_alias(std::wstring_view str, LanguageNameStyle style) {
  if (style == LanguageNameStyle::original)
    return {std::wstring(str), true};

  std::wstring str_normalized{str};
  std::replace(str_normalized.begin(), str_normalized.end(), L'_', L'-');
  auto alias = WinApi::get_locale_info(str_normalized.data(), [&]() {
    switch (style) {
    case LanguageNameStyle::english:
      return LOCALE_SENGLISHDISPLAYNAME;
    case LanguageNameStyle::localized:
      return LOCALE_SLOCALIZEDDISPLAYNAME;
    case LanguageNameStyle::native:
      return LOCALE_SNATIVEDISPLAYNAME;
    case LanguageNameStyle::original:
    case LanguageNameStyle::COUNT:
      break;
    }
    return LOCALE_SENGLISHDISPLAYNAME;
  }());
  if (!alias.empty())
    return {alias, true};

  to_upper_inplace(str_normalized);
  auto it = alias_map.find(str_normalized);
  if (it != alias_map.end())
    return {it->second, true};

  return {std::wstring(str), false};
}

static bool try_to_create_dir(const wchar_t *path, bool silent, HWND npp_window) {
  if (!CreateDirectory(path, nullptr)) {
    if (!silent) {
      if (npp_window == nullptr)
        return false;

      auto msg = wstring_printf(rc_str(IDS_CANT_CREATE_DIR_PS).c_str(), path);
      MessageBox(npp_window, msg.c_str (), rc_str(IDS_ERROR_IN_DIR_CREATE).c_str(), MB_OK | MB_ICONERROR);
    }
    return false;
  }
  return true;
}

bool check_for_directory_existence(std::wstring path, bool silent, HWND npp_window) {
  for (auto &c : path) {
    if (c == L'\\') {
      c = L'\0';
      if (!PathFileExists(path.c_str())) {
        if (!try_to_create_dir(path.c_str(), silent, npp_window))
          return false;
      }
      c = L'\\';
    }
  }
  if (!PathFileExists(path.c_str())) {
    if (!try_to_create_dir(path.c_str(), silent, npp_window))
      return false;
  }
  return true;
}

void write_unicode_bom(FILE *fp) {
  WORD bom = 0xFEFF;
  fwrite(&bom, sizeof(bom), 1, fp);
}

std::wstring read_ini_value(const wchar_t *app_name, const wchar_t *key_name, const wchar_t *default_value, const wchar_t *file_name) {
  constexpr int initial_buffer_size = 64;
  std::vector<wchar_t> buffer(initial_buffer_size);
  while (true) {
    auto size_written = GetPrivateProfileString(app_name, key_name, default_value, buffer.data(), static_cast<DWORD>(buffer.size()), file_name);
    if (size_written < buffer.size() - 1)
      return buffer.data();
    buffer.resize(buffer.size() * 2);
  }
}
