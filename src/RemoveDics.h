#include "StaticDialog/StaticDialog.h"

class SpellChecker;

class RemoveDics : public StaticDialog
{
public:
  void init (HINSTANCE hInst, HWND Parent, HWND LibComboArg);
  void DoDialog ();
  void RemoveSelected (SpellChecker *SpellCheckerInstance);
  HWND GetListBox ();
protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
protected:
  HWND HLangList;
  HWND LibCombo;
};