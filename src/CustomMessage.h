#pragma once

namespace CustomGUIMessage
{
  enum e
  {
    DO_MESSAGE_BOX = 0,   // Use MessageBoxInfo as wParam.
    SHOW_CALCULATED_MENU = 1,
    AUTOCHECK_STATE_CHANGED = 2,
    CONFIGURATION_CHANGED = 3,  // Either settings, either language list have changed. GUI needs refreshment
    MAX,
  };
}

const wchar_t *const CustomGUIMesssagesNames[] = {
  L"DSpellCheck_MessageBox",
  L"DSpellCheck_ShowCalculatedMenu",
  L"DSpellCheck_AutoCheckStateChanged",
  L"DSpellCheck_LanguageListChanged",
};

static_assert (sizeof (CustomGUIMesssagesNames) / sizeof (wchar_t *)== CustomGUIMessage::MAX, "CustomGUIMesssagesNames size should be equal to CustomGUIMessage::MAX");
