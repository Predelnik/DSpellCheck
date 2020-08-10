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
#include "aspell.h"
#include "SpellerContainer.h"

struct AspellCanHaveError;
struct AspellSpeller;
class Settings;

#include "SpellerInterface.h"

class LanguageInfo;

class AspellInterface : public SpellerInterface {
  using SpellerPtr = std::unique_ptr<AspellSpeller, void (*)(AspellSpeller *)>;
public:
  AspellInterface(HWND npp_window_arg, const Settings &settings); // Window for error reporting
  ~AspellInterface() override;
  std::vector<LanguageInfo> get_language_list() const override;
  void set_language(const wchar_t* lang) override;
  void set_multiple_languages(
      const std::vector<std::wstring>& list) override; // Languages are from LangList
  bool
  check_word(const WordForSpeller& word) const override; // Word in Utf-8 or ANSI (For now only Utf-8)
  bool is_working() const override;
    std::vector<std::wstring> get_suggestions(const wchar_t* word) const override;
  void add_to_dictionary(const wchar_t* word) override;
  void ignore_all(const wchar_t* word) override;

  bool init(const wchar_t* path_arg);
  AspellStatus get_status() const;
  std::wstring get_default_personal_dictionary_path() const;

private:
  void send_aspell_error(AspellCanHaveError *error);
    void setup_aspell_config(AspellConfig* spell_config);

private:
  mutable AspellSpeller *m_last_selected_speller = nullptr;
  SpellerPtr m_single_speller;
  std::vector<SpellerPtr> m_spellers;
  bool m_aspell_loaded = false;
  bool m_correct_bitness = true;
  HWND m_npp_window = nullptr; // For message boxes
  const Settings &m_settings;
};
