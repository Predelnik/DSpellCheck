/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "CommonFunctions.h"
#include "iconv.h"
#include "MainDef.h"
#include "Plugin.h"
#include <cassert>
#include "utils/utf8.h"

static std::vector<char> convert(const char* source_enc, const char* target_enc, const void* source_data_ptr,
                                 size_t source_len, size_t max_dest_len) {
    std::vector<char> buf(max_dest_len);
    char* out_buf = reinterpret_cast<char *>(buf.data());
    iconv_t converter = iconv_open(target_enc, source_enc);
    auto char_ptr = static_cast<const char *>(source_data_ptr);
    size_t res =
        iconv(converter, static_cast<const char **>(&char_ptr), &source_len, &out_buf, &max_dest_len);
    iconv_close(converter);

    if (res == static_cast<size_t>(-1))
        return {};
    return buf;
}

MappedWstring utf8_to_mapped_wstring(std::string_view str) {
   ptrdiff_t len = str.length ();
   std::vector<wchar_t> buf;
   std::vector<ptrdiff_t> mapping;
   buf.reserve (len);
   mapping.reserve (len);
   auto it = str.data ();
   size_t char_cnt = 1;
   while (it - str.data () < len) {
      auto next = utf8_inc(it);
      buf.resize (char_cnt);
      MultiByteToWideChar(CP_UTF8, 0, it, static_cast<int> (next - it), buf.data () + char_cnt - 1, 1);
      mapping.push_back (it - str.data ());
      ++char_cnt;
      it = next;
   }
   mapping.push_back (it - str.data ());
   return {std::wstring {buf.begin (), buf.end ()}, std::move (mapping)};
}

MappedWstring to_mapped_wstring(std::string_view str) {
   return {to_wstring (str), {}};
}

std::wstring to_wstring(std::string_view source) {
    auto bytes = convert("CHAR", "UCS-2LE//IGNORE", source.data(), source.length(),
                         sizeof(wchar_t) * (source.length() + 1));
    if (bytes.empty()) return {};
    return reinterpret_cast<wchar_t *>(bytes.data());
}

std::string to_string(std::wstring_view source) {
    auto bytes = convert("UCS-2LE", "CHAR//IGNORE", source.data(), (source.length()) * sizeof(wchar_t),
                         sizeof(wchar_t) * (source.length() + 1));
    if (bytes.empty()) return {};
    return bytes.data();
}

constexpr size_t max_utf8_char_length = 6;

std::string to_utf8_string(std::string_view source) {
    auto bytes = convert("CHAR", "UTF-8//IGNORE", source.data(), source.length(),
                         max_utf8_char_length * (source.length() + 1));
    if (bytes.empty()) return {};
    return bytes.data();
}

std::string to_utf8_string(std::wstring_view source) {
    auto bytes = convert("UCS-2LE", "UTF-8//IGNORE", source.data(), source.length() * sizeof (wchar_t),
                         max_utf8_char_length * (source.length() + 1));
    if (bytes.empty()) return {};
    return bytes.data();
}

std::wstring utf8_to_wstring(const char* source) {
    auto bytes = convert("UTF-8", "UCS-2LE//IGNORE", source, strlen(source),
                         (utf8_length(source) + 1) * sizeof (wchar_t));
    if (bytes.empty()) return {};
    return reinterpret_cast<const wchar_t *>(bytes.data());
}

std::string utf8_to_string(const char* source) {
    auto bytes = convert("UTF-8", "CHAR//IGNORE", source, strlen(source),
                         utf8_length(source) + 1);
    if (bytes.empty()) return {};
    return bytes.data();
}

// This function is more or less transferred from gcc source
bool match_special_char(wchar_t* dest, const wchar_t*& source) {
    wchar_t c;

    bool m = true;
    auto assign = [dest](wchar_t c) { if (dest) *dest = c; };

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
    case L'U':
        {
            /* Hexadecimal form of wide characters.  */
            int len = (c == L'x' ? 2 : (c == L'u' ? 4 : 8));
            wchar_t n = 0;
#ifndef UNICODE
    len = 2;
#endif
            for (int i = 0; i < len; i++) {
                wchar_t buf[2] = {L'\0', L'\0'};

                c = *(source++);
                if (c > UCHAR_MAX ||
                    !((L'0' <= c && c <= L'9') || (L'a' <= c && c <= L'f') ||
                        (L'A' <= c && c <= L'F')))
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

static size_t do_parse_string(wchar_t* dest, const wchar_t* source) {
    bool dry_run = dest == nullptr;
    wchar_t* res_string = dest;
    while (*source) {
        const wchar_t* last_pos = source;
        if (*source == '\\') {
            ++source;
            if (!match_special_char(!dry_run ? dest : nullptr, source)) {
                source = last_pos;
                if (!dry_run)
                    *dest = *source;
                ++source;
                ++dest;
            }
            else {
                ++dest;
            }
        }
        else {
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

std::wstring parse_string(const wchar_t* source) {
    auto size = do_parse_string(nullptr, source);
    std::vector<wchar_t> buf(size);
    do_parse_string(buf.data(), source);
    assert (size == buf.size ());
    return buf.data();
}

// These functions are mostly taken from http://research.microsoft.com

static const std::unordered_map<std::wstring_view, std::wstring_view> alias_map = {
    {L"af_Za", L"Afrikaans"},
    {L"ak_GH", L"Akan"},
    {L"bg_BG", L"Bulgarian"},
    {L"ca_ANY", L"Catalan (Any)"},
    {L"ca_ES", L"Catalan (Spain)"},
    {L"cop_EG", L"Coptic (Bohairic dialect)"},
    {L"cs_CZ", L"Czech"},
    {L"cy_GB", L"Welsh"},
    {L"da_DK", L"Danish"},
    {L"de_AT", L"German (Austria)"},
    {L"de_CH", L"German (Switzerland)"},
    {L"de_DE", L"German (Germany)"},
    {L"de_DE_comb", L"German (Old and New Spelling)"},
    {L"de_DE_frami", L"German (Additional)"},
    {L"de_DE_neu", L"German (New Spelling)"},
    {L"el_GR", L"Greek"},
    {L"en_AU", L"English (Australia)"},
    {L"en_CA", L"English (Canada)"},
    {L"en_GB", L"English (Great Britain)"},
    {L"en_GB-oed", L"English (Great Britain) [OED]"},
    {L"en_NZ", L"English (New Zealand)"},
    {L"en_US", L"English (United States)"},
    {L"en_ZA", L"English (South Africa)"},
    {L"eo_EO", L"Esperanto"},
    {L"es_AR", L"Spanish (Argentina)"},
    {L"es_BO", L"Spanish (Bolivia)"},
    {L"es_CL", L"Spanish (Chile)"},
    {L"es_CO", L"Spanish (Colombia)"},
    {L"es_CR", L"Spanish (Costa Rica)"},
    {L"es_CU", L"Spanish (Cuba)"},
    {L"es_DO", L"Spanish (Dominican Republic)"},
    {L"es_EC", L"Spanish (Ecuador)"},
    {L"es_ES", L"Spanish (Spain)"},
    {L"es_GT", L"Spanish (Guatemala)"},
    {L"es_HN", L"Spanish (Honduras)"},
    {L"es_MX", L"Spanish (Mexico)"},
    {L"es_NEW", L"Spanish (New)"},
    {L"es_NI", L"Spanish (Nicaragua)"},
    {L"es_PA", L"Spanish (Panama)"},
    {L"es_PE", L"Spanish (Peru)"},
    {L"es_PR", L"Spanish (Puerto Rico)"},
    {L"es_PY", L"Spanish (Paraguay)"},
    {L"es_SV", L"Spanish (El Salvador)"},
    {L"es_UY", L"Spanish (Uruguay)"},
    {L"es_VE", L"Spanish (Bolivarian Republic of Venezuela)"},
    {L"et_EE", L"Estonian"},
    {L"fo_FO", L"Faroese"},
    {L"fr_FR", L"French"},
    {L"fr_FR-1990", L"French (1990)"},
    {L"fr_FR-1990_1-3-2", L"French (1990)"},
    {L"fr_FR-classique", L"French (Classique)"},
    {L"fr_FR-classique_1-3-2", L"French (Classique)"},
    {L"fr_FR_1-3-2", L"French"},
    {L"fy_NL", L"Frisian"},
    {L"ga_IE", L"Irish"},
    {L"gd_GB", L"Scottish Gaelic"},
    {L"gl_ES", L"Galician"},
    {L"gu_IN", L"Gujarati"},
    {L"he_IL", L"Hebrew"},
    {L"hi_IN", L"Hindi"},
    {L"hil_PH", L"Filipino"},
    {L"hr_HR", L"Croatian"},
    {L"hu_HU", L"Hungarian"},
    {L"ia", L"Interlingua"},
    {L"id_ID", L"Indonesian"},
    {L"is_IS", L"Icelandic"},
    {L"it_IT", L"Italian"},
    {L"ku_TR", L"Kurdish"},
    {L"la", L"Latin"},
    {L"lt_LT", L"Lithuanian"},
    {L"lv_LV", L"Latvian"},
    {L"mg_MG", L"Malagasy"},
    {L"mi_NZ", L"Maori"},
    {L"mk_MK", L"Macedonian (FYROM)"},
    {L"mos_BF", L"Mossi"},
    {L"mr_IN", L"Marathi"},
    {L"ms_MY", L"Malay"},
    {L"nb_NO", L"Norwegian (Bokmal)"},
    {L"ne_NP", L"Nepali"},
    {L"nl_NL", L"Dutch"},
    {L"nn_NO", L"Norwegian (Nynorsk)"},
    {L"nr_ZA", L"Ndebele"},
    {L"ns_ZA", L"Northern Sotho"},
    {L"ny_MW", L"Chichewa"},
    {L"oc_FR", L"Occitan"},
    {L"pl_PL", L"Polish"},
    {L"pt_BR", L"Portuguese (Brazil)"},
    {L"pt_PT", L"Portuguese (Portugal)"},
    {L"ro_RO", L"Romanian"},
    {L"ru_RU", L"Russian"},
    {L"ru_RU_ie", L"Russian (without io)"},
    {L"ru_RU_ye", L"Russian (without io)"},
    {L"ru_RU_yo", L"Russian (with io)"},
    {L"rw_RW", L"Kinyarwanda"},
    {L"si_SI", L"Slovenian"},
    {L"sk_SK", L"Slovak"},
    {L"sq_AL", L"Alban"},
    {L"ss_ZA", L"Swazi"},
    {L"st_ZA", L"Northern Sotho"},
    {L"sv_SE", L"Swedish"},
    {L"sw_KE", L"Kiswahili"},
    {L"tet_ID", L"Tetum"},
    {L"th_TH", L"Thai"},
    {L"tl_PH", L"Tagalog"},
    {L"tn_ZA", L"Setswana"},
    {L"ts_ZA", L"Tsonga"},
    {L"uk_UA", L"Ukrainian"},
    {L"ur_PK", L"Urdu"},
    {L"ve_ZA", L"Venda"},
    {L"vi-VN", L"Vietnamese"},
    {L"xh_ZA", L"isiXhosa"},
    {L"zu_ZA", L"isiZulu"}
};
// Language Aliases
std::pair<std::wstring_view, bool> apply_alias(std::wstring_view str) {
    auto it = alias_map.find(str);
    if (it != alias_map.end())
        return {it->second, true};
    else
        return {str, false};
}

static bool try_to_create_dir(const wchar_t* path, bool silent, HWND npp_window) {
    if (!CreateDirectory(path, nullptr)) {
        if (!silent) {
            if (!npp_window)
                return false;

            wchar_t message[DEFAULT_BUF_SIZE];
            swprintf(message, L"Can't create directory %s", path);
            MessageBox(npp_window, message, L"Error in directory creation",
                       MB_OK | MB_ICONERROR);
        }
        return false;
    }
    return true;
}

bool check_for_directory_existence(std::wstring path, bool silent, HWND npp_window) {
    for (auto& c : path) {
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

void replace_all(std::string& str, std::string_view from, std::string_view to) {
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::wstring read_ini_value(const wchar_t* app_name, const wchar_t* key_name, const wchar_t* default_value,
                            const wchar_t* file_name) {
    constexpr int initial_buffer_size = 64;
    std::vector<wchar_t> buffer(initial_buffer_size);
    while (true) {
        auto size_written = GetPrivateProfileString(app_name, key_name, default_value, buffer.data(),
                                                    static_cast<DWORD>(buffer.size()), file_name);
        if (size_written < buffer.size() - 1)
            return buffer.data();
        buffer.resize(buffer.size() * 2);
    }
}
