// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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
#include "PrecompiledHeader.h"
#include "Settings.h"
#include "utils/enum_range.h"
#include "utils/string_utils.h"

void SpellerContainer::create_spellers(const NppData &npp_data) {
  m_aspell_speller = std::make_unique<AspellInterface>(npp_data.npp_handle);
  m_hunspell_speller = std::make_unique<HunspellInterface>(npp_data.npp_handle);
  m_native_speller = std::make_unique<NativeSpellerInterface>();
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

AspellStatus SpellerContainer::get_aspell_status() const {
  if (m_aspell_speller->is_working()) {
    if (get_available_languages().empty())
      return AspellStatus::no_dictionaries;

    return AspellStatus::working;
  }

  return AspellStatus::not_working;
}

std::vector<LanguageInfo> SpellerContainer::get_available_languages() const {
  if (!active_speller().is_working())
    return {};

  auto langs = active_speller().get_language_list();
  std::sort(langs.begin(), langs.end(),
            m_settings.use_language_name_aliases ? less_aliases
                                                 : less_original);
  return langs;
}

const NativeSpellerInterface &SpellerContainer::native_speller() const {
  return *m_native_speller;
}

const SpellerInterface &SpellerContainer::active_speller() const {
  return const_cast<Self *>(this)->active_speller();
}

void SpellerContainer::cleanup() { m_native_speller->cleanup(); }

SpellerInterface &SpellerContainer::active_speller() {
  if (!m_single_speller)
    return *m_spellers[m_settings.active_speller_lib_id];

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

SpellerContainer::SpellerContainer(const Settings *settings,
                                   const NppData *npp_data)
    : m_settings(*settings) {
  init_spellers(*npp_data);
  m_native_speller->init(); // just to allow checking if it's available or not
  m_settings.settings_changed.connect([this] { on_settings_changed(); });
}

SpellerContainer::SpellerContainer(const Settings *settings,
                                   std::unique_ptr<SpellerInterface> speller)
    : m_settings(*settings), m_spellers({}) {
  m_single_speller = std::move(speller);
  m_settings.settings_changed.connect([this] { on_settings_changed(); });
  on_settings_changed ();
}

static void set_multiple_languages(std::wstring_view multi_string,
                                   SpellerInterface &speller) {
  std::vector<std::wstring> multi_lang_list;
  for (auto token :
       make_delimiter_tokenizer(multi_string, LR"(\|)").get_all_tokens())
    multi_lang_list.emplace_back(token);

  speller.set_multiple_languages(multi_lang_list);
}

SpellerContainer::~SpellerContainer() = default;

void SpellerContainer::apply_settings_to_active_speller() {
  if (m_single_speller)
    return;

  switch (m_settings.active_speller_lib_id) {
  case SpellerId::native:
    m_native_speller->init();
    break;
  case SpellerId::aspell:
    m_aspell_speller->init(m_settings.aspell_path.c_str());
    m_aspell_speller->set_allow_run_together(
        m_settings.aspell_allow_run_together_words);
    break;
  case SpellerId::hunspell:
    m_hunspell_speller->set_use_one_dic(m_settings.use_unified_dictionary);
    m_hunspell_speller->set_directory(m_settings.hunspell_user_path.c_str());
    m_hunspell_speller->set_additional_directory(
        m_settings.hunspell_system_path.c_str());
    break;
  case SpellerId::COUNT:
    break;
  }
}

void SpellerContainer::init_speller() {
  apply_settings_to_active_speller();

  if (auto language = m_settings.get_active_language();
      language != L"<MULTIPLE>") {
    active_speller().set_language(language.c_str());
    active_speller().set_mode(SpellerInterface::SpellerMode::SingleLanguage);
  } else {
    set_multiple_languages(m_settings.get_active_multi_languages(),
                           active_speller());
    active_speller().set_mode(SpellerInterface::SpellerMode::MultipleLanguages);
  }
}

TemporaryAcessor<SpellerContainer::Self> SpellerContainer::modify() const {
  auto non_const_this = const_cast<Self *>(this);
  return {*non_const_this,
          [non_const_this]() { non_const_this->speller_status_changed(); }};
}
