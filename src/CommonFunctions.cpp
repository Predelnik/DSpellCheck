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
#include "PluginInterface.h"
#include <cassert>

static std::vector<char> convert(const char* sourceEnc, const char* targetEnc, const void* sourceDataPtr,
                                 size_t sourceLen, size_t maxDestLen)
{
    std::vector<char> buf(maxDestLen);
    char* OutBuf = reinterpret_cast<char *>(buf.data());
    iconv_t Converter = iconv_open(targetEnc, sourceEnc);
    auto charPtr = static_cast<const char *>(sourceDataPtr);
    size_t res =
        iconv(Converter, static_cast<const char **>(&charPtr), &sourceLen, &OutBuf, &maxDestLen);
    iconv_close(Converter);

    if (res == static_cast<size_t>(-1))
        return {};
    return buf;
}

std::wstring to_wstring(std::string_view source)
{
    auto bytes = convert("CHAR", "UCS-2LE//IGNORE", source.data(), source.length() + 1,
                         sizeof(wchar_t) * (source.length() + 1));
    if (bytes.empty()) return {};
    return reinterpret_cast<wchar_t *>(bytes.data());
}

std::string to_string(std::wstring_view source)
{
    auto bytes = convert("UCS-2LE", "CHAR//IGNORE", source.data(), (source.length() + 1) * sizeof(wchar_t),
                         sizeof(wchar_t) * (source.length() + 1));
    if (bytes.empty()) return {};
    return bytes.data();
}

constexpr size_t maxUtf8CharLength = 6;

std::string toUtf8String(std::string_view source)
{
    auto bytes = convert("CHAR", "UTF-8//IGNORE", source.data(), source.length() + 1,
                         maxUtf8CharLength * (source.length() + 1));
    if (bytes.empty()) return {};
    return bytes.data();
}

std::string toUtf8String(std::wstring_view source)
{
    auto bytes = convert("UCS-2LE", "UTF-8//IGNORE", source.data(), (source.length() + 1) * sizeof (wchar_t),
                         maxUtf8CharLength * (source.length() + 1));
    if (bytes.empty()) return {};
    return bytes.data();
}

std::wstring utf8_to_wstring(const char* source)
{
    auto bytes = convert("UTF-8", "UCS-2LE//IGNORE", source, strlen (source) + 1,
                         (Utf8Length(source) + 1) * sizeof (wchar_t));
    if (bytes.empty()) return {};
    return reinterpret_cast<const wchar_t *> (bytes.data());
}

std::string utf8_to_string(const char* source)
{
    auto bytes = convert("UTF-8", "CHAR//IGNORE", source, strlen (source) + 1,
                         Utf8Length(source) + 1);
    if (bytes.empty()) return {};
    return bytes.data();
}

// This function is more or less transferred from gcc source
bool MatchSpecialChar(wchar_t* Dest, const wchar_t*& Source)
{
    int len, i;
    wchar_t c, n;
    bool m;

    m = true;
    auto assign = [Dest](wchar_t c){ if (Dest) *Dest = c; };

    switch (c = *(Source++))
    {
    case L'a':
        assign (L'\a');
        break;
    case L'b':
        assign (L'\b');
        break;
    case L't':
        assign (L'\t');
        break;
    case L'f':
        assign (L'\f');
        break;
    case L'n':
        assign (L'\n');
        break;
    case L'r':
        assign (L'\r');
        break;
    case L'v':
        assign (L'\v');
        break;
    case L'\\':
        assign (L'\\');
        break;
    case L'0':
        assign (L'\0');
        break;

    case L'x':
    case L'u':
    case L'U':
        /* Hexadecimal form of wide characters.  */
        len = (c == L'x' ? 2 : (c == L'u' ? 4 : 8));
        n = 0;
#ifndef UNICODE
    len = 2;
#endif
        for (i = 0; i < len; i++)
        {
            wchar_t buf[2] = {L'\0', L'\0'};

            c = *(Source++);
            if (c > UCHAR_MAX ||
                !((L'0' <= c && c <= L'9') || (L'a' <= c && c <= L'f') ||
                    (L'A' <= c && c <= L'F')))
                return false;

            buf[0] = static_cast<wchar_t>(c);
            n = n << 4;
            n += static_cast<wchar_t>(wcstol(buf, nullptr, 16));
        }
        assign (n);
        break;

    default:
        /* Unknown backslash codes are simply not expanded.  */
        m = false;
        break;
    }
    return m;
}

static size_t doParseString(wchar_t *Dest, const wchar_t* Source)
{
    const wchar_t* LastPos = nullptr;
    bool dryRun = Dest == nullptr;
    wchar_t* ResString = Dest;
    while (*Source)
    {
        LastPos = Source;
        if (*Source == '\\')
        {
            ++Source;
            if (!MatchSpecialChar(!dryRun ? Dest : nullptr, Source))
            {
                Source = LastPos;
                if (!dryRun)
                    *Dest = *Source;
                ++Source;
                ++Dest;
            }
            else
            {
                ++Dest;
            }
        }
        else
        {
            if (!dryRun)
                *Dest = *Source;
            ++Source;
            ++Dest;
        }
    }
    if (!dryRun)
        *Dest = L'\0';
    return Dest - ResString + 1;
}

std::wstring parseString(const wchar_t* source)
{
    auto size = doParseString(nullptr, source);
    std::vector<wchar_t> buf (size);
    assert (doParseString (buf.data (), source) == buf.size ());
    return buf.data ();
}

// These functions are mostly taken from http://research.microsoft.com

bool Utf8IsLead(char c)
{
    return (((c & 0x80) == 0) // 0xxxxxxx
        || ((c & 0xC0) == 0xC0 && (c & 0x20) == 0) // 110xxxxx
        || ((c & 0xE0) == 0xE0 && (c & 0x10) == 0) // 1110xxxx
        || ((c & 0xF0) == 0xF0 && (c & 0x08) == 0) // 11110xxx
        || ((c & 0xF8) == 0xF8 && (c & 0x04) == 0) // 111110xx
        || ((c & 0xFC) == 0xFC && (c & 0x02) == 0));
}

bool Utf8IsCont(char c)
{
    return ((c & 0x80) == 0x80 && (c & 0x40) == 0); // 10xxxxx
}

char* Utf8Dec(const char* string, const char* current)
{
    const char* temp;
    if (string >= current)
        return nullptr;

    temp = current - 1;
    while (string <= temp && (!Utf8IsLead(*temp)))
        temp--;

    return (char *)temp;
}

char* Utf8Inc(const char* string)
{
    const char* temp;
    temp = string + 1;
    while (*temp && !Utf8IsLead(*temp))
        temp++;

    return (char *)temp;
}

int Utf8Gewchar_tSize(char c)
{
    if ((c & 0x80) == 0)
        return 1;
    else if ((c & 0xC0) > 0 && (c & 0x20) == 0)
        return 2;
    else if ((c & 0xE0) > 0 && (c & 0x10) == 0)
        return 3;
    else if ((c & 0xF0) > 0 && (c & 0x08) == 0)
        return 4;
    else if ((c & 0xF8) > 0 && (c & 0x04) == 0)
        return 5;
    else if ((c & 0xFC) > 0 && (c & 0x02) == 0)
        return 6;
    return 0;
}

bool Utf8Firswchar_tsAreEqual(const char* Str1, const char* Str2)
{
    int Firswchar_tSize1 = Utf8Gewchar_tSize(*Str1);
    int Firswchar_tSize2 = Utf8Gewchar_tSize(*Str2);
    if (Firswchar_tSize1 != Firswchar_tSize2)
        return false;
    return (strncmp(Str1, Str2, Firswchar_tSize1) == 0);
}

char* Utf8pbrk(const char* s, const char* set)
{
    const char* x;
    for (; *s; s = Utf8Inc(s))
        for (x = set; *x; x = Utf8Inc(x))
            if (Utf8Firswchar_tsAreEqual(s, x))
                return (char *)s;
    return nullptr;
}

std::ptrdiff_t Utf8spn(const char* s, const char* set)
{
    const char* x;
    const char* it = nullptr;
    it = s;

    for (; *it; it = Utf8Inc(it))
    {
        for (x = set; *x; x = Utf8Inc(x))
        {
            if (Utf8Firswchar_tsAreEqual(it, x))
                goto continue_outer;
        }
        break;
    continue_outer:;
    }
    return it - s;
}

char* Utf8chr(const char* s, const char* sfc) // Char is first from the string
// sfc (string with first char)
{
    while (*s)
    {
        if (s && Utf8Firswchar_tsAreEqual(s, sfc))
            return (char *)s;
        s = Utf8Inc(s);
    }
    return nullptr;
}

char* Utf8strtok(char* s1, const char* Delimit, char** Context)
{
    char* tmp;

    /* Skip leading delimiters if new string. */
    if (s1 == nullptr)
    {
        s1 = *Context;
        if (s1 == nullptr) /* End of story? */
            return nullptr;
        else
            s1 += Utf8spn(s1, Delimit);
    }
    else
    {
        s1 += Utf8spn(s1, Delimit);
    }

    /* Find end of segment */
    tmp = Utf8pbrk(s1, Delimit);
    if (tmp)
    {
        /* Found another delimiter, split string and save state. */
        *tmp = '\0';
        tmp++;
        while (!Utf8IsLead(*(tmp)))
        {
            *tmp = '\0';
            tmp++;
        }

        *Context = tmp;
    }
    else
    {
        /* Last segment, remember that. */
        *Context = nullptr;
    }

    return s1;
}

size_t Utf8Length(const char* String)
{
    char* It = const_cast<char *>(String);
    size_t Size = 0;
    while (*It)
    {
        Size++;
        It = Utf8Inc(It);
    }
    return Size;
}

bool SortCompare(wchar_t* a, wchar_t* b) { return wcscmp(a, b) < 0; }

bool EquivCharStrings(char* a, char* b) { return (strcmp(a, b) == 0); }

size_t HashCharString(char* a)
{
    size_t Hash = 7;
    for (unsigned int i = 0; i < strlen(a); i++)
    {
        Hash = Hash * 31 + a[i];
    }
    return Hash;
}

bool Equivwchar_tStrings(wchar_t* a, wchar_t* b) { return (wcscmp(a, b) == 0); }

size_t Hashwchar_tString(wchar_t* a)
{
    size_t Hash = 7;
    for (unsigned int i = 0; i < wcslen(a); i++)
    {
        Hash = Hash * 31 + a[i];
    }
    return Hash;
}

bool SortCompareChars(char* a, char* b) { return strcmp(a, b) < 0; }

static const std::unordered_map<std::wstring_view, std::wstring_view> aliasMap = {
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
std::pair<std::wstring_view, bool> applyAlias(std::wstring_view str)
{
    auto it = aliasMap.find(str);
    if (it != aliasMap.end())
        return {it->second, true};
    else
        return {str, false};
}

static bool TryToCreateDir(const wchar_t* Path, bool Silent, HWND NppWindow)
{
    if (!CreateDirectory(Path, nullptr))
    {
        if (!Silent)
        {
            if (!NppWindow)
                return false;

            wchar_t Message[DEFAULT_BUF_SIZE];
            swprintf(Message, L"Can't create directory %s", Path);
            MessageBox(NppWindow, Message, L"Error in directory creation",
                       MB_OK | MB_ICONERROR);
        }
        return false;
    }
    return true;
}

bool CheckForDirectoryExistence(std::wstring path, bool Silent, HWND NppWindow)
{
    for (auto &c : path)
    {
        if (c == L'\\')
        {
            c = L'\0';
            if (!PathFileExists(path.c_str ()))
            {
                if (!TryToCreateDir(path.c_str (), Silent, NppWindow))
                    return false;
            }
            c = L'\\';
        }
    }
    if (!PathFileExists(path.c_str ()))
    {
        if (!TryToCreateDir(path.c_str (), Silent, NppWindow))
            return false;
    }
    return true;
}

wchar_t* GetLastSlashPosition(wchar_t* Path) { return wcsrchr(Path, L'\\'); }

void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::wstring readIniValue(const wchar_t* appName, const wchar_t* keyName, const wchar_t* defaultValue,
                          const wchar_t* fileName)
{
    constexpr int initialBufferSize = 64;
    std::vector<wchar_t> buffer(initialBufferSize);
    while (true)
    {
        auto sizeWritten = GetPrivateProfileString(appName, keyName, defaultValue, buffer.data(),
                                                   static_cast<DWORD>(buffer.size()), fileName);
        if (sizeWritten < buffer.size() - 1)
            return buffer.data();
        buffer.resize(buffer.size() * 2);
    }
}
