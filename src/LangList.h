#include "StaticDialog/StaticDialog.h"

class SpellChecker;

class LangList : public StaticDialog
{
public:
  void init (HINSTANCE hInst, HWND Parent);
  void DoDialog ();
  HWND GetListBox ();
  void ApplyChoice (SpellChecker *SpellCheckerInstance);
protected:
  __override virtual BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
protected:
  HWND HLangList;
};