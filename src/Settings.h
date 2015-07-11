#pragma once
// Class for storing settings which needed in worker threads but could be changed through GUI, should be filled gradually

enum class SpellerType
{
  aspell,
  hunspell,

  COUNT,
};

enum class FileMaskOption
{
  exclusion,
  inclusion,

  COUNT,
};

enum class SuggestionControlType
{
  box,
  menu,
};

struct SpellerSettings
{
  std::wstring activeLanguage;
  std::wstring activeMultiLanguage;
  std::wstring path;
  std::wstring additionalPath;
};

// Should be copiable
struct SettingsData
{
  SpellerType activeSpellerType = SpellerType::COUNT;
  enum_vector<SpellerType, SpellerSettings> spellerSettings;

  const SpellerSettings &activeSpellerSettings () const;
  SpellerSettings &activeSpellerSettings ();

  bool useLanguageNameAliases; // these opitions are for hunspell actually
  bool unifiedUserDictionary;  // ===

  SuggestionControlType suggestionControlType;
  int suggestionCount;

  int bufferSizeForSearch; // (in Kb) TODO: remove

  FileMaskOption fileMaskOption;
  std::wstring fileMask;
  bool skipCode;
  std::wstring delimiters;

  bool skipWordsStartingOrFinishingWithApostrophe;
  bool skipNumbers;
  bool skipCapitalizedWords;
  bool skipWordsHavingCapitals;
  bool skipWordsHavingOnlyCapitals;
  bool skipWordsWithUnderline;
  bool skipOneLetterWords;

  int underlineColor;
  int underlineStyle;

  bool skipStartingFinishingApostrophe;
  bool treatYoAsYe;
  bool convertUnicodeQuotes;

  int suggestionBoxSize;
  int suggestionBoxTransparency;
};

class SettingsDataChanger : Uncopiable
{
public:
  SettingsDataChanger (const SettingsData &dataSrc, std::function<void (const SettingsData&)> callback) : m_callback {callback}, m_valid {true}
  {
    m_data = dataSrc; // create-and-copy
  }

  SettingsData *operator-> ()
  {
    return &m_data;
  }

  ~SettingsDataChanger()
  {
    m_callback (m_data);
  }

  SettingsDataChanger (SettingsDataChanger &&other)
  {
    *this = std::move (other);
  }

  SettingsDataChanger &operator =(SettingsDataChanger &&other)
  {
    using std::swap;
    swap (m_data, other.m_data);
    swap (m_callback, other.m_callback);
    swap (m_valid, other.m_valid);
  }

private:
  SettingsData m_data;
  std::function<void (const SettingsData&)> m_callback;
  bool m_valid; // TODO-MSVC2015 Make auto generated move operators using move-only bool
};

class Settings
{
public:
  Settings (std::function<void (const SettingsData&)> callback);
  const SettingsData &get () const;
  std::shared_ptr<const SettingsData> cloneData () const;
  SettingsDataChanger change ();
  void exchange (SettingsData &newData);

private:
  std::shared_ptr<SettingsData> m_data;
  std::function<void (const SettingsData&)> m_callback;
};
