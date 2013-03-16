#include "Controls/StaticDialog/StaticDialog.h"

class AboutDlg : public StaticDialog
{
public:
  void init (HINSTANCE hInst, HWND Parent);
  void DoDialog ();
protected:
  BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
};