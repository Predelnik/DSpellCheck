#pragma once
#include "CommonFunctions.h"
#include "SpellerId.h"
#include "lsignal.h"
#include "utils/TemporaryAcessor.h"
#include "utils/enum_array.h"

struct NppData;
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
  SpellerInterface &active_speller();
  AspellStatus get_aspell_status() const;
  void cleanup();

public:
  mutable lsignal::signal<void()> speller_status_changed;

private:
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
