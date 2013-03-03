#include "StaticDialog/StaticDialog.h"

class Suggestions : public StaticDialog
{
public:
  void DoDialog ();
protected:
  BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam) __override;
}