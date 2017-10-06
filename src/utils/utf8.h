#pragma once

char *utf8_dec(const char *string, const char *current);
char *utf8_chr(const char *s, const char *sfc);
int utf8_symbol_len(char c);
char *utf8_strtok(char *s1, const char *Delimit, char **Context);
const char* utf8_inc(const char* string);
char* utf8_inc(char* string);
char *utf8_pbrk(const char *s, const char *set);
size_t utf8_length(const char *String);
bool utf8_is_lead(char c);
bool utf8_is_cont(char c);
