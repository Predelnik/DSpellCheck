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
#include "aspell.h"

struct AspellCanHaveError;
struct AspellSpeller;

#include "SpellerInterface.h"

class LanguageInfo;

class AspellInterface : public SpellerInterface {
  using SpellerPtr = std::unique_ptr<AspellSpeller, void (*)(AspellSpeller *)>;
public:
  AspellInterface(HWND npp_window_arg); // Window for error reporting
  ~AspellInterface();
  std::vector<LanguageInfo> get_language_list() const override;
  void set_language(const wchar_t* lang) override;
  void set_multiple_languages(
      const std::vector<std::wstring>& list) override; // Languages are from LangList
  bool
  check_word(const wchar_t* word) override; // Word in Utf-8 or ANSI (For now only Utf-8)
  bool is_working() const override;
    std::vector<std::wstring> get_suggestions(const wchar_t* word) override;
  void add_to_dictionary(const wchar_t* word) override;
  void ignore_all(const wchar_t* word) override;
  void set_allow_run_together (bool allow);

  bool init(const wchar_t* path_arg);

private:
  void send_aspell_error(AspellCanHaveError *error);
    void setup_aspell_config(AspellConfig* spell_config);

private:
  AspellSpeller *m_last_selected_speller;
  SpellerPtr m_single_speller;
  std::vector<SpellerPtr> m_spellers;
  bool m_allow_run_together;
  bool m_aspell_loaded;
  HWND m_npp_window; // For message boxes
};
