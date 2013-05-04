#include <io.h>
#include <fcntl.h>
#include <LzExpand.h>

#include "Controls/CheckedList/CheckedList.h"
#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "HunspellInterface.h"
#include "Plugin.h"
#include "Progress.h"
#include "resource.h"
#include "SpellChecker.h"
#include "MainDef.h"
#include "unzip.h"

void DownloadDicsDlg::DoDialog ()
{
  if (!isCreated())
  {
    create (IDD_DOWNLOADDICS);
  }
  else
  {
    goToCenter ();
    display ();
  }
}

void DownloadDicsDlg::FillFileList ()
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  ComboBox_GetText (HAddress, Buf, DEFAULT_BUF_SIZE);
  GetDownloadDics ()->DoFtpOperation (FILL_FILE_LIST, Buf);
}

void DownloadDicsDlg::OnDisplayAction ()
{
  SendEvent (EID_FILL_FILE_LIST);
}

void DownloadDicsDlg::init (HINSTANCE hInst, HWND Parent, SpellChecker *SpellCheckerInstanceArg, HWND LibComboArg)
{
  SpellCheckerInstance = SpellCheckerInstanceArg;
  Timer = 0;
  LibCombo = LibComboArg;
  return Window::init (hInst, Parent);
}

DownloadDicsDlg::~DownloadDicsDlg ()
{
  DeleteTimerQueueTimer (0, Timer, 0);
}

#define BUF_SIZE_FOR_COPY 10240
void DownloadDicsDlg::DownloadSelected ()
{
  int Count = ListBox_GetCount (HFileList);
  TCHAR TempPath [MAX_PATH];
  TCHAR LocalPath [MAX_PATH];
  char *LocalPathANSI = 0;
  TCHAR Buf[DEFAULT_BUF_SIZE];
  char *DicFileNameANSI = 0;
  std::map<char *, int, bool (*)(char *, char *)>::iterator it;
  TCHAR *DicFileName = 0;
  TCHAR DicFileLocalPath[MAX_PATH];
  TCHAR HunspellDicPath[MAX_PATH];
  TCHAR Message[DEFAULT_BUF_SIZE];
  TCHAR *FileName = 0;
  BOOL IsAffFile = FALSE;
  BOOL IsDicFile = FALSE;
  unz_file_info FInfo;
  char FileCopyBuf[(BUF_SIZE_FOR_COPY)];
  TCHAR ProgMessage[DEFAULT_BUF_SIZE];
  GetTempPath (MAX_PATH, TempPath);
  _tcscpy (Message, _T ("Dictionaries copied:\n"));
  std::map<char *, int, bool (*)(char *, char *)> FilesFound (SortCompareChars); //0x01 - .aff found, 0x02 - .dic found
  Progress *p = GetProgress ();
  p->SetProgress (0);
  p->SetBottomMessage (_T (""));
  p->SetTopMessage (_T (""));
  for (int i = 0; i < Count; i++)
  {
    if (CheckedListBox_GetCheckState (HFileList, i) == BST_CHECKED)
    {
      FileName = new TCHAR [ListBox_GetTextLen (HFileList, i) + 4 + 1];
      ListBox_GetText (HFileList, i, FileName);
      _tcscat (FileName, _T (".zip"));
      _stprintf (ProgMessage, _T ("Downloading %s..."), FileName);
      p->SetTopMessage (ProgMessage);
      _tcscpy (LocalPath, TempPath);
      _tcscat (LocalPath, FileName);
      ComboBox_GetText (HAddress, Buf, DEFAULT_BUF_SIZE);
      DoFtpOperation (DOWNLOAD_FILE, Buf, FileName, LocalPath);
      SetString (LocalPathANSI, LocalPath);
      unzFile fp = unzOpen (LocalPathANSI);
      if (unzGoToFirstFile (fp) != UNZ_OK)
        goto clean_and_continue;
      do
      {
        DicFileNameANSI = new char [DEFAULT_BUF_SIZE];
        unzGetCurrentFileInfo (fp, &FInfo, DicFileNameANSI, DEFAULT_BUF_SIZE, 0, 0, 0 ,0);
        IsAffFile = (strcmp (DicFileNameANSI + strlen (DicFileNameANSI) - 4, ".aff") == 0);
        IsDicFile = (strcmp (DicFileNameANSI + strlen (DicFileNameANSI) - 4, ".dic") == 0);

        if (IsAffFile || IsDicFile)
        {
          DicFileNameANSI [strlen (DicFileNameANSI) - 4] = '\0';
          BOOL CleanArr = TRUE;
          if (FilesFound.find (DicFileNameANSI) == FilesFound.end ())
          {
            FilesFound [DicFileNameANSI] = 0;
            CleanArr = FALSE;
          }
          std::map<char *, int, bool (*)(char *, char *)>::iterator it = FilesFound.find (DicFileNameANSI);
          (*it).second |= (IsAffFile ? 0x01 : 0x02);
          unzOpenCurrentFile (fp);
          int BytesCopied = 0;
          _tcscpy (DicFileLocalPath, TempPath);
          SetString (DicFileName, DicFileNameANSI);
          _tcscat (DicFileLocalPath, DicFileName);
          if (IsAffFile)
            _tcscat (DicFileLocalPath, _T (".aff"));
          else
            _tcscat (DicFileLocalPath, _T (".dic"));

          SetFileAttributes (DicFileLocalPath, FILE_ATTRIBUTE_NORMAL);
          int LocalDicFileHandle = _topen (DicFileLocalPath, _O_CREAT | _O_BINARY | _O_WRONLY);
          if (LocalDicFileHandle == -1)
            continue;

          _stprintf (ProgMessage, _T ("Extracting %s..."), DicFileName);
          p->SetTopMessage (ProgMessage);
          DWORD BytesTotal = 0;
          while ((BytesCopied = unzReadCurrentFile (fp, FileCopyBuf, (BUF_SIZE_FOR_COPY))) != 0)
          {
            _write (LocalDicFileHandle, FileCopyBuf, BytesCopied);
            BytesTotal += BytesCopied;
            FInfo.uncompressed_size, BytesTotal * 100 / FInfo.uncompressed_size;
            p->SetProgress (BytesTotal * 100 / FInfo.uncompressed_size);
            _stprintf (ProgMessage, _T ("%d / %d bytes extracted (%d %%)"), BytesTotal, FInfo.uncompressed_size, BytesTotal * 100 / FInfo.uncompressed_size);
            p->SetBottomMessage (ProgMessage);
          }
          unzCloseCurrentFile (fp);
          _close (LocalDicFileHandle);
        }
      } while (unzGoToNextFile (fp) == UNZ_OK);
      // Now we're gonna check what's exactly we extracted with using FilesFound map
      it = FilesFound.begin ();
      for (; it != FilesFound.end (); ++it)
      {
        if ((*it).second != 3) // Some of .aff/.dic is missing
        {
          _tcscpy (DicFileLocalPath, TempPath);
          SetString (DicFileName, (*it).first);
          _tcscat (DicFileLocalPath, DicFileName);
          switch ((*it).second)
          {
          case 1:
            _tcscat (DicFileLocalPath, _T (".aff"));
            break;
          case 2:
            _tcscat (DicFileLocalPath, _T (".dic"));
            break;
          }
          SetFileAttributes (DicFileLocalPath, FILE_ATTRIBUTE_NORMAL);
          DeleteFile (DicFileLocalPath);
        }
        else
        {
          _tcscpy (DicFileLocalPath, TempPath);
          SetString (DicFileName, (*it).first);
          _tcscat (DicFileLocalPath, DicFileName);
          _tcscat (DicFileLocalPath, _T (".aff"));
          _tcscpy (HunspellDicPath, SpellCheckerInstance->GetHunspellPath ());
          _tcscat (HunspellDicPath, _T ("\\"));
          _tcscat (HunspellDicPath, DicFileName);
          _tcscat (HunspellDicPath, _T (".aff"));
          if (PathFileExists (HunspellDicPath))
            DeleteFile (DicFileLocalPath); // Probably there should be message box about replacing.
          else
            MoveFile (DicFileLocalPath, HunspellDicPath);
          _tcscpy (DicFileLocalPath + _tcslen (DicFileLocalPath) - 4, _T (".dic"));
          _tcscpy (HunspellDicPath + _tcslen (HunspellDicPath) - 4, _T (".dic"));
          if (PathFileExists (HunspellDicPath))
            DeleteFile (DicFileLocalPath);
          else
            MoveFile (DicFileLocalPath, HunspellDicPath);
          _tcscat (Message, DicFileName);
          _tcscat (Message, _T ("\n"));
        }
      }
clean_and_continue:
      unzClose (fp);
      DeleteFile (LocalPath); // Removing downloaded .zip file
      CLEAN_AND_ZERO_ARR (FileName);
    }
  }
  GetProgress ()->display (false);
  MessageBox (_hSelf, Message, _T ("Dictionaries Downloaded"), MB_OK | MB_ICONINFORMATION);
  SpellCheckerInstance->GetHunspellSpeller ()->SetDirectory (SpellCheckerInstance->GetHunspellPath ()); // Calling the update for Hunspell dictionary list
  if (ComboBox_GetCurSel (LibCombo) == 1)
    SpellCheckerInstance->ReinitLanguageLists (1);
  CLEAN_AND_ZERO_ARR (LocalPathANSI);
  CLEAN_AND_ZERO_ARR (DicFileName);
}

DWORD LatestHandle = 0;
DWORD BytesRead = 0;
HANDLE MayContinue;
DWORD LatestResult = 0;
int ActiveContext = 0;
int WaitingFor = 0;
int Test = 0;

#define CONTEXT_CONNECT 1
#define CONTEXT_FIND_FIRST_FILE 2
#define CONTEXT_OPEN_FILE 3

VOID CALLBACK
  CallBack(
  __in HINTERNET hInternet,
  __in DWORD_PTR dwContext,
  __in DWORD dwInternetStatus,
  __in_bcount(dwStatusInformationLength) LPVOID lpvStatusInformation,
  __in DWORD dwStatusInformationLength
  )
{
  if (dwInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE)
  {
    LatestResult = ((LPINTERNET_ASYNC_RESULT)lpvStatusInformation)->dwResult;
    SetEvent (MayContinue);
  }
}

void DownloadDicsDlg::DoFtpOperation (FTP_OPERATION_TYPE Type, TCHAR *Address, TCHAR *FileName, TCHAR *Location)
{
  StrTrim (Address, _T (" "));
  TCHAR *Folders = 0;
  HINTERNET Internet = 0;
  const TCHAR FtpPrefix[] = _T ("ftp://");
  int FtpPrefixLen = _tcslen (FtpPrefix);
  for (unsigned int i = 0; i < _tcslen (Address); i++) // Exchanging slashes
  {
    if (Address[i] == _T ('\\'))
      Address[i] = _T ('/');
  }

  if (Type == FILL_FILE_LIST)
  {
    Static_SetText (HStatus, _T ("Status: Loading..."));

    if (_tcsncmp (FtpPrefix, Address, FtpPrefixLen) == 0) // Cutting out stuff like ftp://
    {
      for (unsigned int i = 0; i <= _tcslen (Address) - FtpPrefixLen; i++)
      {
        Address[i] = Address [i + FtpPrefixLen];
      }
    }

    Folders = _tcschr (Address, _T ('/'));
    if (Folders != 0)
    {
      *Folders = _T ('\0');
      Folders++;
    }
  }

  HINTERNET Session = InternetOpen (_T ("DSpellCheck"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_ASYNC);
  if (!Session)
  {
    Static_SetText (HStatus, _T ("Status: Connection Error"));
    return;
  }

  INTERNET_STATUS_CALLBACK CallbackPointer = InternetSetStatusCallback (Session, (INTERNET_STATUS_CALLBACK) CallBack);
  if (CallbackPointer == INTERNET_INVALID_STATUS_CALLBACK)
    goto cleanup;

  MayContinue = ::CreateEvent (NULL, FALSE, FALSE, NULL);
  WaitingFor = INTERNET_STATUS_REQUEST_COMPLETE;
  ActiveContext = CONTEXT_CONNECT;
  InternetConnect (Session, Address, INTERNET_INVALID_PORT_NUMBER, NULL, NULL, INTERNET_SERVICE_FTP, 0, CONTEXT_CONNECT);

  WaitForSingleObject (MayContinue, INFINITE);
  HINTERNET Internet = (HINTERNET) LatestResult;

  // HANDLE hFtpThread;
  // DWORD ftpThreadId;

  // hFtpThread = CreateThread (NULL, 0, FtpThread, Internet, 0, &ftpThreadId);

  if (Type == FILL_FILE_LIST)
    ListBox_ResetContent (HFileList);

  if (!Internet)
  {
    Static_SetText (HStatus, _T ("Status: Connection Error"));
    return;
  }

  // PostThreadMessage (ftpThreadId, TM_CHANGE_DIR, (WPARAM) Folders, 0);

  // if (WaitForMultipleEvents (EID_FTP_OK, EID_FTP_FAIL) != WAIT_OBJECT_0)
  if (Folders != 0) // Otherwise it's a root dir
  {
    DWORD Res = 0;
    FtpSetCurrentDirectory (Internet, Folders);
    WaitForSingleObject (MayContinue, INFINITE);
    if (!LatestResult)
    {
      Static_SetText (HStatus, _T ("Status: Directory wasn't found"));
      goto cleanup;
    }
  }

  if (Type == FILL_FILE_LIST)
  {
    WIN32_FIND_DATAA FindData;
    TCHAR *Buf = 0;
    WaitingFor = INTERNET_STATUS_REQUEST_COMPLETE; // INTERNET_STATUS_RESPONSE_RECEIVED;// INTERNET_STATUS_RESPONSE_RECEIVED;
    ActiveContext = CONTEXT_FIND_FIRST_FILE;
    FtpFindFirstFileA (Internet, "*.zip", &FindData, 0, CONTEXT_FIND_FIRST_FILE);
    WaitForSingleObject (MayContinue, INFINITE);
    HINTERNET FindHandle = (HINTERNET) LatestResult;
    if (!FindHandle)
    {
      Static_SetText (HStatus, _T ("Status: Directory doesn't contain any zipped files"));
      goto cleanup;
    }
    SetString (Buf, FindData.cFileName);
    Buf[_tcslen (Buf) - 4] = 0;
    ListBox_AddString (HFileList, Buf);
    while (1)
    {
      InternetFindNextFileA (FindHandle, &FindData);
      WaitForSingleObject (MayContinue, INFINITE);
      if (!LatestResult)
        break;

      SetString (Buf, FindData.cFileName);
      Buf[_tcslen (Buf) - 4] = 0;
      ListBox_AddString (HFileList, Buf);
    }
    CLEAN_AND_ZERO_ARR (Buf);
    Static_SetText (HStatus, _T ("Status: List of available files was successfully loaded"));
  }
  else if (Type == DOWNLOAD_FILE)
  {
    DWORD LSize, HSize, Size;
    DWORD BytesSum = 0;
    char DataBuf[BUF_SIZE_FOR_COPY];

    if (PathFileExists (Location))
    {
      SetFileAttributes (Location, FILE_ATTRIBUTE_NORMAL);
      DeleteFile (Location);
    }
    int FileHandle = _topen (Location, _O_CREAT | _O_BINARY | _O_WRONLY);
    if (FileHandle == -1)
      return; // Then file couldn't be download
    DWORD *BytesCopied = new DWORD;
    TCHAR Message[DEFAULT_BUF_SIZE];
    INTERNET_BUFFERS InetBuf;
    memset (&InetBuf, 0, sizeof (INTERNET_BUFFERS));
    InetBuf.dwStructSize = sizeof (INTERNET_BUFFERS);
    FtpOpenFile (Internet, FileName, GENERIC_READ , FTP_TRANSFER_TYPE_BINARY, CONTEXT_OPEN_FILE);
    WaitForSingleObject (MayContinue, INFINITE);
    HINTERNET File = (HINTERNET) LatestResult;
    LSize = FtpGetFileSize (File, &HSize);
    Size = LSize;
    while (1)
    {
      BOOL Res = InternetReadFile (File, DataBuf, BUF_SIZE_FOR_COPY, BytesCopied);
      if (!Res)
      {
        DWORD Err = GetLastError ();
        if (Err == ERROR_IO_PENDING)
          WaitForSingleObject (MayContinue, INFINITE);
        else
          return;
      }

      // InternetReadFileEx (File, &InetBuf, IRF_ASYNC, CONTEXT_DL);
      /*
      DWORD LastError = GetLastError ();
      while (GetLastError () == ERROR_IO_PENDING)
      {
      }
      */

      /*
      if (GetLastError () != 0)
      goto cleanup;
      */

      // BytesCopied = InetBuf.dwBufferLength;

      if (!*BytesCopied || !LatestResult)
        break;

      BytesSum += BytesRead;
      GetProgress ()->SetProgress (BytesSum * 100 / Size);

      _stprintf (Message, _T ("%d / %d bytes downloaded (%d %%)"), BytesSum, Size, BytesSum * 100 / Size);
      GetProgress ()->SetBottomMessage (Message);
      _write (FileHandle, DataBuf, *BytesCopied);
    }
    CLEAN_AND_ZERO (BytesCopied);
    _close (FileHandle);
    InternetCloseHandle (File);
    // FtpGetFile (Internet, FileName, Location, FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0);
  }

cleanup:
  CloseHandle (MayContinue);
  // WaitForSingleObject (FtpThread, INFINITE);
  InternetCloseHandle (Internet);
}

VOID CALLBACK ReinitServer (
  PVOID lpParameter,
  BOOLEAN TimerOrWaitFired
  )
{
  ((DownloadDicsDlg *) lpParameter)->OnDisplayAction ();
  ((DownloadDicsDlg *) lpParameter)->RemoveTimer ();
}

void DownloadDicsDlg::RemoveTimer ()
{
  DeleteTimerQueueTimer (0, Timer, 0);
  Timer = 0;
}

BOOL CALLBACK DownloadDicsDlg::run_dlgProc (UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_INITDIALOG:
    {
      HFileList = ::GetDlgItem (_hSelf, IDC_FILE_LIST);
      HAddress = ::GetDlgItem (_hSelf, IDC_ADDRESS);
      HStatus = ::GetDlgItem (_hSelf, IDC_SERVER_STATUS);
      //ComboBox_SetText(HAddress, _T ("ftp://127.0.0.1"));
      ComboBox_SetText(HAddress, _T ("ftp://gd.tuwien.ac.at/office/openoffice/contrib/dictionaries"));
      OnDisplayAction ();
      return TRUE;
    }
    break;
  case WM_COMMAND:
    {
      switch (LOWORD (wParam))
      {
      case IDOK:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          SendEvent (EID_DOWNLOAD_SELECTED);
          GetProgress ()->DoDialog ();
          display (false);
        }
        break;
      case IDCANCEL:
        if (HIWORD (wParam) == BN_CLICKED)
          display (false);

        break;
      case IDC_ADDRESS:
        if (HIWORD (wParam) == CBN_EDITCHANGE)
        {
          if (Timer)
            ChangeTimerQueueTimer (0, Timer, 1000, 0);
          else
            CreateTimerQueueTimer (&Timer, 0, ReinitServer, this, 1000, 0 , 0);
        }
        break;
      }
    }
    break;
  };
  return FALSE;
}