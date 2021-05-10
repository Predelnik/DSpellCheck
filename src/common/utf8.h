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

#pragma once

char *utf8_dec(const char *string, const char *current);
char *utf8_chr(const char *s, const char *sfc);
int utf8_symbol_len(char c);
const char *utf8_inc(const char *string);
char *utf8_inc(char *string);
const char *utf8_pbrk(const char *s, const char *set);
size_t utf8_length(const char *string);
bool utf8_is_lead(char c);
bool utf8_is_cont(char c);
