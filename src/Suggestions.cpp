#include "resource.h"
#include "Suggestions.h"

void Suggestions::DoDialog ()
{
  create (IDD_SUGGESTIONS);
}

BOOL CALLBACK Suggestions::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
  switch (Message)
  {
  case WM_INITDIALOG:
    return TRUE;
  }
  return FALSE;
}
