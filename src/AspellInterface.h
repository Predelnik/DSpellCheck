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
#pragma once

struct AspellCanHaveError;
struct AspellSpeller;

#include "AbstractSpellerInterface.h"

class AspellInterface : public AbstractSpellerInterface {
  using speller_ptr = std::unique_ptr<AspellSpeller, void (*)(AspellSpeller *)>;
public:
  AspellInterface(HWND npp_window_arg); // Window for error reporting
  ~AspellInterface();
    std::vector<std::wstring> get_language_list() override;
  void set_language(const wchar_t* lang) override;
  void set_multiple_languages(
      const std::vector<std::wstring>& list) override; // Languages are from LangList
  bool
  check_word(const char* word) override; // Word in Utf-8 or ANSI (For now only Utf-8)
  bool is_working() override;
    std::vector<std::string> get_suggestions(const char* word) override;
  void add_to_dictionary(const char* word) override;
  void ignore_all(const char* word) override;

  bool init(const wchar_t* path_arg);

private:
  void send_aspell_error(AspellCanHaveError *error);

public:
private:
  AspellSpeller *m_last_selected_speller;
  speller_ptr m_single_speller;
  std::vector<speller_ptr> m_spellers;
  bool m_aspell_loaded;
  HWND m_npp_window; // For message boxes
};
