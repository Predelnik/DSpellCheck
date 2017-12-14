#pragma once
#include "CommonFunctions.h"
#include "SpellerId.h"
#include "utils/enum_array.h"

struct NppData;
class Settings;
class AspellInterface;
class HunspellInterface;
class NativeSpellerInterface;
class SpellerInterface;

class SpellerContainer {
  using self = SpellerContainer;

public:
  explicit SpellerContainer(const Settings *settings, const NppData *npp_data);
  void native_reinit_settings();
  ~SpellerContainer();
  void update_aspell_language_options();
  void update_hunspell_language_options();
  bool hunspell_reinit_settings();
  bool aspell_reinit_settings();
  void init_speller();
  std::vector<LanguageInfo> get_available_languages() const;
  HunspellInterface &get_hunspell_speller() const {
    return *m_hunspell_speller;
  };
  const SpellerInterface &active_speller() const;
  SpellerInterface &active_speller();
  int get_aspell_status() const;
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
};
