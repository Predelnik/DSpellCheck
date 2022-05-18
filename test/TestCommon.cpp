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

#include "MockSpeller.h"

void setup_speller(MockSpeller &speller) {
  speller.set_inner_dict({{L"English",
                           {L"This", L"is", L"test", L"document", L"Document", L"Please", L"please",
                            L"bear", L"with", L"me", L"И", L"ещё", L"немного", L"слов", L"Paris", L"D'Artagnan"}}});
  MockSpeller::SuggestionsDict dict;
  dict[L"English"][L"abcdef"] = {L"document", L"please"};
  dict[L"English"][L"немонго"] = {L"немного", L"много"};
  speller.set_suggestions_dict(std::move (dict));
}
