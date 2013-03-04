#ifndef SUGGESTIONS_H
#define SUGGESTIONS_H

#include "StaticDialog/StaticDialog.h"

class Suggestions : public StaticDialog
{
public:
  void DoDialog ();
  HMENU GetPopupMenu ();
  int GetResult ();
protected:
  __override BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
private:
  int MenuResult;
  HMENU PopupMenu;
};
#endif // SUGGESTIONS_H
