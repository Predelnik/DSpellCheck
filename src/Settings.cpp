#include "Settings.h"

const SpellerSettings & SettingsData::activeSpellerSettings() const
{
  return spellerSettings[activeSpellerType];
}

SpellerSettings & SettingsData::activeSpellerSettings ()
{
  return spellerSettings[activeSpellerType];
}

Settings::Settings (std::function<void (const SettingsData &)> callback) : m_data (std::make_shared<SettingsData> ()), m_callback (callback)
{

}

const SettingsData & Settings::get() const
{
  return *m_data;
}

std::shared_ptr<const SettingsData> Settings::cloneData() const
{
  return std::static_pointer_cast<const SettingsData> (m_data);
}

void Settings::exchange (SettingsData &newData)
{
  m_data = std::make_shared<SettingsData> ();
  using std::swap;
  swap (*m_data, newData); // create-and-copy
}

SettingsDataChanger Settings::change ()
{
  return {*m_data, m_callback};
}
