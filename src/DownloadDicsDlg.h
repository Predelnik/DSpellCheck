#ifndef DOWNLOAD_DICS_DLG_H
#define DOWNLOAD_DICS_DLG_H

#include "staticdialog\staticdialog.h"
#include <Wininet.h>

struct LanguageName;

void FtpTrim (TCHAR *FtpAddress);

enum FTP_OPERATION_TYPE
{
  FILL_FILE_LIST = 0,
  DOWNLOAD_FILE,
};
class SpellChecker;

class DownloadDicsDlg : public StaticDialog
{
public:
  ~DownloadDicsDlg ();
  void DoDialog ();
  // Maybe hunspell interface should be passed here
  void init (HINSTANCE hInst, HWND Parent, SpellChecker *SpellCheckerInstanceArg, HWND LibComboArg);
  BOOL CALLBACK run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam);
  void DoFtpOperation (FTP_OPERATION_TYPE Type, TCHAR *Address, TCHAR *FileName = 0, TCHAR *Location = 0);
  void DownloadSelected ();
  void FillFileList ();
  void RemoveTimer ();
  void OnDisplayAction ();
  void IndicateThatSavingMightBeNeeded ();
private:
private:
  std::vector<LanguageName> *CurrentLangs;
  HBRUSH DefaultBrush;
  COLORREF StatusColor;
  SpellChecker *SpellCheckerInstance;
  HWND LibCombo;
  HWND HFileList;
  HWND HAddress;
  HWND HStatus;
  HANDLE Timer;
  int CheckIfSavingIsNeeded;
};
#endif // DOWNLOAD_DICS_DLG_H
