#ifndef SETTINGS_DLG_H
#define SETTINGS_DLG_H
#include "StaticDialog/StaticDialog.h"
#include "PluginInterface.h"
#include "TabBar/ControlsTab.h"

class SpellChecker;

class SimpleDlg : public StaticDialog
{
public:
  __override void init (HINSTANCE hInst, HWND Parent, NppData nppData);
  void ApplySettings (SpellChecker *SpellCheckerInstance);
  BOOL AddAvalaibleLanguages (const char *CurrentLanguage);
  void FillSugestionsNum (int SuggestionsNum);

protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);

private:
  /* NppData struct instance */
  NppData NppDataInstance;

  /* handles of controls */
  HWND HSuggestionsNum;
  HWND HSpinSuggestionsNum;
  HWND HComboLanguage;
};

class AdvancedDlg : public StaticDialog
{
public:
  void ApplySettings (SpellChecker *SpellCheckerInstance);
  void FillDelimiters (const char *Delimiters);

protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);

private:
  HWND HEditDelimiters;
  NppData NppDataInstance;
};

class SettingsDlg : public StaticDialog
{
public:
  UINT DoDialog (void);
  __override void init (HINSTANCE hInst, HWND Parent, NppData nppData);
  SimpleDlg *GetSimpleDlg ();
  AdvancedDlg *GetAdvancedDlg ();
protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
  __override virtual void destroy ();
private:
  NppData NppDataInstance;
  SimpleDlg SimpleDlgInstance;
  AdvancedDlg AdvancedDlgInstance;
  WindowVector WindowVectorInstance;
  ControlsTab ControlsTabInstance;
};
#endif //SETTINGS_DLG_H
