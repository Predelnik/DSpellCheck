#include "utf8.h"

bool utf8_is_lead(char c)
{
    return (((c & 0x80) == 0) // 0xxxxxxx
        || ((c & 0xC0) == 0xC0 && (c & 0x20) == 0) // 110xxxxx
        || ((c & 0xE0) == 0xE0 && (c & 0x10) == 0) // 1110xxxx
        || ((c & 0xF0) == 0xF0 && (c & 0x08) == 0) // 11110xxx
        || ((c & 0xF8) == 0xF8 && (c & 0x04) == 0) // 111110xx
        || ((c & 0xFC) == 0xFC && (c & 0x02) == 0));
}

bool utf8_is_cont(char c)
{
    return ((c & 0x80) == 0x80 && (c & 0x40) == 0); // 10xxxxx
}

char* utf8_dec(const char* string, const char* current)
{
    const char* temp;
    if (string >= current)
        return nullptr;

    temp = current - 1;
    while (string <= temp && (!utf8_is_lead(*temp)))
        temp--;

    return (char *)temp;
}

char* utf8_inc(const char* string)
{
    const char* temp;
    temp = string + 1;
    while (*temp && !utf8_is_lead(*temp))
        temp++;

    return (char *)temp;
}

int utf8_symbol_len(char c)
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
    int Firswchar_tSize1 = utf8_symbol_len(*Str1);
    int Firswchar_tSize2 = utf8_symbol_len(*Str2);
    if (Firswchar_tSize1 != Firswchar_tSize2)
        return false;
    return (strncmp(Str1, Str2, Firswchar_tSize1) == 0);
}

char* utf8_pbrk(const char* s, const char* set)
{
    const char* x;
    for (; *s; s = utf8_inc(s))
        for (x = set; *x; x = utf8_inc(x))
            if (Utf8Firswchar_tsAreEqual(s, x))
                return (char *)s;
    return nullptr;
}

std::ptrdiff_t Utf8spn(const char* s, const char* set)
{
    const char* x;
    const char* it = nullptr;
    it = s;

    for (; *it; it = utf8_inc(it))
    {
        for (x = set; *x; x = utf8_inc(x))
        {
            if (Utf8Firswchar_tsAreEqual(it, x))
                goto continue_outer;
        }
        break;
    continue_outer:;
    }
    return it - s;
}

char* utf8_chr(const char* s, const char* sfc) // Char is first from the string
// sfc (string with first char)
{
    while (*s)
    {
        if (s && Utf8Firswchar_tsAreEqual(s, sfc))
            return (char *)s;
        s = utf8_inc(s);
    }
    return nullptr;
}

char* utf8_strtok(char* s1, const char* Delimit, char** Context)
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
    tmp = utf8_pbrk(s1, Delimit);
    if (tmp)
    {
        /* Found another delimiter, split string and save state. */
        *tmp = '\0';
        tmp++;
        while (!utf8_is_lead(*(tmp)))
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

size_t utf8_length(const char* String)
{
    char* It = const_cast<char *>(String);
    size_t Size = 0;
    while (*It)
    {
        Size++;
        It = utf8_inc(It);
    }
    return Size;
}
