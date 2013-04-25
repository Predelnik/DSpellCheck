#include <io.h>
#include <fcntl.h>
#include <LzExpand.h>
#include <Wininet.h>

#include "Controls/CheckedList/CheckedList.h"
#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "Plugin.h"
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
    OnDisplayAction ();
    goToCenter ();
    display ();
  }
}

void DownloadDicsDlg::OnDisplayAction ()
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  ComboBox_GetText (HAddress, Buf, DEFAULT_BUF_SIZE);
  DoFtpOperation (FILL_FILE_LIST, Buf);
}

void DownloadDicsDlg::init (HINSTANCE hInst, HWND Parent, SpellChecker *SpellCheckerInstanceArg)
{
  SpellCheckerInstance = SpellCheckerInstanceArg;
  return Window::init (hInst, Parent);
}

#define BUF_SIZE_FOR_COPY 16384
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
  TCHAR *FileName = 0;
  BOOL IsAffFile = FALSE;
  BOOL IsDicFile = FALSE;
  unz_file_info Dummy;
  char FileCopyBuf[BUF_SIZE_FOR_COPY];
  TCHAR Message[DEFAULT_BUF_SIZE];
  GetTempPath (MAX_PATH, TempPath);
  _tcscpy (Message, _T ("Dictionaries copied:\n"));
  std::map<char *, int, bool (*)(char *, char *)> FilesFound (SortCompareChars); //0x01 - .aff found, 0x02 - .dic found
  for (int i = 0; i < Count; i++)
  {
    if (CheckedListBox_GetCheckState (HFileList, i) == BST_CHECKED)
    {
      FileName = new TCHAR [ListBox_GetTextLen (HFileList, i) + 4 + 1];
      ListBox_GetText (HFileList, i, FileName);
      _tcscat (FileName, _T (".zip"));
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
        unzGetCurrentFileInfo (fp, &Dummy, DicFileNameANSI, DEFAULT_BUF_SIZE, 0, 0, 0 ,0);
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

          while ((BytesCopied = unzReadCurrentFile (fp, FileCopyBuf, BUF_SIZE_FOR_COPY)) != 0)
          {
            _write (LocalDicFileHandle, FileCopyBuf, BytesCopied);
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
  MessageBox (_hSelf, Message, _T ("Dictionaries Downloaded"), MB_OK | MB_ICONINFORMATION);
  CLEAN_AND_ZERO_ARR (LocalPathANSI);
  CLEAN_AND_ZERO_ARR (DicFileName);
}

void DownloadDicsDlg::DoFtpOperation (FTP_OPERATION_TYPE Type, TCHAR *Address, TCHAR *FileName, TCHAR *Location)
{
  StrTrim (Address, _T (" "));
  TCHAR *Folders;
  for (unsigned int i = 0; i < _tcslen (Address); i++) // Exchanging slashes
  {
    if (Address[i] == _T ('\\'))
      Address[i] = _T ('/');
  }
  const TCHAR FtpPrefix[] = _T ("ftp://");
  int FtpPrefixLen = _tcslen (FtpPrefix);
  if (_tcsncmp (FtpPrefix, Address, _tcslen (FtpPrefix)) == 0) // Cutting out stuff like ftp://
  {
    for (unsigned int i = 0; i <= _tcslen (Address) - FtpPrefixLen; i++)
    {
      Address[i] = Address [i + FtpPrefixLen];
    }
  }

  Folders = _tcschr (Address, _T ('/'));
  *Folders = _T ('\0');
  Folders++;

  HINTERNET Session = InternetOpen (_T ("DSpellCheck"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  if (!Session)
  {
    Static_SetText (HStatus, _T ("Connection Error"));
    return;
  }
  HINTERNET Internet = InternetConnect (Session, Address, INTERNET_INVALID_PORT_NUMBER, NULL, NULL, INTERNET_SERVICE_FTP, 0, NULL);
  ListBox_ResetContent (HFileList);
  if (!Internet)
  {
    Static_SetText (HStatus, _T ("Connection Error"));
    return;
  }

  if (!FtpSetCurrentDirectory (Internet, Folders))
  {
    Static_SetText (HStatus, _T ("Directory wasn't found"));
    goto cleanup;
  }

  if (Type == FILL_FILE_LIST)
  {
    WIN32_FIND_DATA FindData;
    HINTERNET FindHandle = FtpFindFirstFile (Internet, _T("*.zip"), &FindData, 0, 0);
    if (!FindHandle)
    {
      Static_SetText (HStatus, _T ("Directory doesn't contain any zipped files"));
      goto cleanup;
    }
    FindData.cFileName[_tcslen (FindData.cFileName) - 4] = 0;
    ListBox_AddString (HFileList, FindData.cFileName);
    while (InternetFindNextFile (FindHandle, &FindData))
    {
      FindData.cFileName[_tcslen (FindData.cFileName) - 4] = 0;
      ListBox_AddString (HFileList, FindData.cFileName);
    }
  }
  else if (Type == DOWNLOAD_FILE)
  {
    FtpGetFile (Internet, FileName, Location, FALSE, FILE_ATTRIBUTE_NORMAL, FTP_TRANSFER_TYPE_BINARY, 0);
  }

cleanup:
  InternetCloseHandle (Internet);
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
          SendEvent (EID_DOWNLOAD_SELECTED);
        display (false);
        break;
      case IDCANCEL:
        if (HIWORD (wParam) == BN_CLICKED)
          display (false);

        break;
      }
    }
    break;
  };
  return FALSE;
}