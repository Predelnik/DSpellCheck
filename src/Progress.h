#ifndef PROGRESS_H
#define PROGRESS_H

#include "StaticDialog\StaticDialog.h"

class Progress : public StaticDialog
{
public:
  Progress(void);
  ~Progress(void);

  BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
  void init (HINSTANCE hInst, HWND Parent);
  void DoDialog ();
  void SetProgress ();
  void SetProgress (int value);
  void SetBottomMessage (TCHAR *Message);
  void SetTopMessage (TCHAR *Message);
private:
  HWND HDescBottom;
  HWND HDescTop;
  HWND HProgressBar;
};
#endif //PROGRESS_H
