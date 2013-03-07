#ifndef SUGGESTIONS_H
#define SUGGESTIONS_H

#include "StaticDialog/StaticDialog.h"

class Suggestions : public StaticDialog
{
public:
  Suggestions ();
  void DoDialog ();
  HMENU GetPopupMenu ();
  int GetResult ();
protected:
  __override BOOL CALLBACK run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
private:
  int MenuResult;
  BOOL StatePressed;
  BOOL StateHovered;
  BOOL StateMenu;
  HMENU PopupMenu;
};
#endif // SUGGESTIONS_H
