#ifndef SUGGESTIONS_H
#define SUGGESTIONS_H

#include "StaticDialog/StaticDialog.h"

class Suggestions : public StaticDialog
{
public:
  void DoDialog ();
  HMENU GetPopupMenu ();
protected:
  __override BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
private:
  HMENU PopupMenu;
};
#endif // SUGGESTIONS_H
