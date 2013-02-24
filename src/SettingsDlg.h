#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H
#include "StaticDialog.h"
#include "PluginInterface.h"

class SettingsDlg : public StaticDialog
{
public:
  SettingsDlg (void);
  void init(HINSTANCE hInst, NppData nppData);
  UINT DoDialog (void);
protected:
  BOOL AddAvalaibleLanguages ();
  virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);

private:
  /* NppData struct instance */
  NppData NppDataInstance;

  /* handles of controls */
  HWND HComboLanguage;
  HWND HOk;
  HWND HCancel;
};
#endif //SETTINGS_DLG_H
