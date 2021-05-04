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

#include "SpellerContainer.h"
#include "AspellInterface.h"
#include "HunspellInterface.h"
#include "LanguageInfo.h"
#include "NativeSpellerInterface.h"
#include "plugin/Settings.h"
#include "common/enum_range.h"
#include "common/string_utils.h"
#include "core/SpellCheckerHelpers.h"

void SpellerContainer::create_spellers(const NppData &npp_data) {
  m_aspell_speller = std::make_unique<AspellInterface>(npp_data.npp_handle, m_settings);
  m_hunspell_speller = std::make_unique<HunspellInterface>(npp_data.npp_handle, m_settings);
  m_native_speller = std::make_unique<NativeSpellerInterface>(m_settings);
  m_hunspell_speller->speller_loaded.connect([this] { speller_status_changed(); });
}

void SpellerContainer::fill_speller_ptr_array() {
  for (auto id : enum_range<SpellerId>()) {
    m_spellers[id] = [&]() -> SpellerInterface * {
      switch (id) {
      case SpellerId::aspell:
        return m_aspell_speller.get();
      case SpellerId::hunspell:
        return m_hunspell_speller.get();
      case SpellerId::native:
        return m_native_speller.get();
      case SpellerId::COUNT:
        break;
      }
      return nullptr;
    }();
  }
}

AspellStatus SpellerContainer::get_aspell_status() const { return m_aspell_speller->get_status(); }

std::wstring SpellerContainer::get_aspell_default_personal_dictionary_path() const { return m_aspell_speller->get_default_personal_dictionary_path(); }

std::vector<LanguageInfo> SpellerContainer::get_available_languages() const {
  if (!active_speller().is_working())
    return {};

  auto langs = active_speller().get_language_list();
  std::sort(langs.begin(), langs.end(), m_settings.data.language_name_style != LanguageNameStyle::original ? less_aliases : less_original);
  return langs;
}

const NativeSpellerInterface &SpellerContainer::native_speller() const { return *m_native_speller; }

const SpellerInterface &SpellerContainer::active_speller() const { return const_cast<Self *>(this)->active_speller(); }

void SpellerContainer::cleanup() { m_native_speller->cleanup(); }

SpellerInterface &SpellerContainer::active_speller() {
  if (!m_single_speller)
    return *m_spellers[m_settings.data.active_speller_lib_id];

  return *m_single_speller;
}

void SpellerContainer::init_spellers(const NppData &npp_data) {
  create_spellers(npp_data);
  fill_speller_ptr_array();
}

void SpellerContainer::on_settings_changed() {
  init_speller();
  speller_status_changed();
}

void SpellerContainer::ignore_word(std::wstring wstr) {
  SpellCheckerHelpers::apply_word_conversions(m_settings, wstr);
  // calling get_suggestions before ignoring the word is a requirement by some spellers currently
  // we're just discarding the result
  static_cast<void> (active_speller().get_suggestions(wstr.c_str()));
  active_speller().ignore_all(wstr.c_str());
}

void SpellerContainer::add_to_dictionary(std::wstring wstr) {
  SpellCheckerHelpers::apply_word_conversions(m_settings, wstr);
  active_speller().add_to_dictionary(wstr.c_str());
}

SpellerContainer::SpellerContainer(const Settings *settings, const NppData *npp_data) : m_settings(*settings) {
  init_spellers(*npp_data);
  m_native_speller->init(); // just to allow checking if it's available or not
  m_settings.settings_changed.connect([this] { on_settings_changed(); });
}

SpellerContainer::SpellerContainer(const Settings *settings, std::unique_ptr<SpellerInterface> speller) : m_settings(*settings), m_spellers({}) {
  m_single_speller = std::move(speller);
  m_settings.settings_changed.connect([this] { on_settings_changed(); });
  on_settings_changed();
}

static void set_multiple_languages(std::wstring_view multi_string, SpellerInterface &speller) {
  std::vector<std::wstring> multi_lang_list;
  for (auto token : make_delimiter_tokenizer(multi_string, LR"(\|)").get_all_tokens())
    multi_lang_list.emplace_back(token);

  speller.set_multiple_languages(multi_lang_list);
}

SpellerContainer::~SpellerContainer() = default;

void SpellerContainer::apply_settings_to_active_speller() {
  if (m_single_speller)
    return;

  switch (m_settings.data.active_speller_lib_id) {
  case SpellerId::native:
    m_native_speller->init();
    break;
  case SpellerId::aspell:
    m_aspell_speller->init(m_settings.data.aspell_dll_path.c_str());
    break;
  case SpellerId::hunspell:
    m_hunspell_speller->set_use_one_dic(m_settings.data.use_unified_dictionary);
    m_hunspell_speller->set_directory(m_settings.data.hunspell_user_path.c_str());
    m_hunspell_speller->set_additional_directory(m_settings.data.hunspell_system_path.c_str());
    break;
  case SpellerId::COUNT:
    break;
  }
}

void SpellerContainer::init_speller() {
  apply_settings_to_active_speller();

  if (!m_settings.data.auto_check_text)
    return;

  if (auto language = m_settings.get_active_language(); language != multiple_language_alias) {
    active_speller().set_language(language.c_str());
    active_speller().set_mode(SpellerInterface::SpellerMode::SingleLanguage);
  } else {
    set_multiple_languages(m_settings.get_active_multi_languages(), active_speller());
    active_speller().set_mode(SpellerInterface::SpellerMode::MultipleLanguages);
  }
}

TemporaryAcessor<SpellerContainer::Self> SpellerContainer::modify() const {
  auto non_const_this = const_cast<Self *>(this);
  return {*non_const_this, [non_const_this]() { non_const_this->speller_status_changed(); }};
}
