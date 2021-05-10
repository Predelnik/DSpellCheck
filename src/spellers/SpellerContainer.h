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
#include "lsignal.h"
#include "SpellerId.h"
#include "common/CommonFunctions.h"
#include "common/enum_array.h"
#include "common/TemporaryAcessor.h"

class NppData;
class Settings;
class AspellInterface;
class HunspellInterface;
class NativeSpellerInterface;
class SpellerInterface;
class LanguageInfo;
class MockSpeller;

enum class AspellStatus {
  working,
  no_dictionaries,
  not_working,
  incorrect_bitness,
};

class SpellerContainer {
  using Self = SpellerContainer;

public:
  explicit SpellerContainer(const Settings *settings, const NppData *npp_data);
  SpellerContainer(const Settings *settings,
                   std::unique_ptr<SpellerInterface> speller);
  ~SpellerContainer();
  void apply_settings_to_active_speller();
  void init_speller();
  TemporaryAcessor<Self> modify() const;
  std::vector<LanguageInfo> get_available_languages() const;

  HunspellInterface &get_hunspell_speller() const {
    return *m_hunspell_speller;
  }

  const NativeSpellerInterface &native_speller() const;
  const SpellerInterface &active_speller() const;
  AspellStatus get_aspell_status() const;
  std::wstring get_aspell_default_personal_dictionary_path() const;
  void cleanup();
  void ignore_word(std::wstring wstr);
  void add_to_dictionary(std::wstring wstr);;

public:
  mutable lsignal::signal<void()> speller_status_changed;

private:
  SpellerInterface &active_speller();
  void create_spellers(const NppData &npp_data);
  void fill_speller_ptr_array();
  void init_spellers(const NppData &npp_data);
  void on_settings_changed();

private:
  const Settings &m_settings;
  std::unique_ptr<AspellInterface> m_aspell_speller;
  std::unique_ptr<HunspellInterface> m_hunspell_speller;
  std::unique_ptr<NativeSpellerInterface> m_native_speller;
  enum_array<SpellerId, SpellerInterface *> m_spellers;
  std::unique_ptr<SpellerInterface> m_single_speller;
};
