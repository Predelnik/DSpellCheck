#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H
#include "StaticDialog/StaticDialog.h"
#include "PluginInterface.h"
#include "TabBar/ControlsTab.h"

class SimpleDlg : public StaticDialog
{
public:
  __override void init (HINSTANCE hInst, HWND Parent, NppData nppData);
  void ApplySettings ();
protected:
  BOOL AddAvalaibleLanguages ();
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);

private:
  /* NppData struct instance */
  NppData NppDataInstance;

  /* handles of controls */
  HWND HComboLanguage;
  HWND HOk;
  HWND HCancel;
};

class AdvanceDlg : public StaticDialog
{
public:
  void ApplySettings ();

protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);

private:
  HWND HEditDelimeters;
  NppData NppDataInstance;
};

class SettingsDlg : public StaticDialog
{
public:
  UINT DoDialog (void);
  __override void init (HINSTANCE hInst, HWND Parent, NppData nppData);
protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
  __override virtual void destroy ();
private:
  NppData NppDataInstance;
  SimpleDlg SimpleDlgInstance;
  AdvanceDlg AdvancedDlgInstance;
  WindowVector WindowVectorInstance;
  ControlsTab ControlsTabInstance;
};
#endif //SETTINGS_DLG_H
