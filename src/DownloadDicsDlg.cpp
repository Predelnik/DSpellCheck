/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <io.h>
#include <fcntl.h>
#include <LzExpand.h>

#include "Controls/CheckedList/CheckedList.h"
#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "HunspellInterface.h"
#include "LanguageName.h"
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

DownloadDicsDlg::DownloadDicsDlg ()
{
  Timer = 0;
  RefreshIcon = 0;
  CheckIfSavingIsNeeded = 0;
  CurrentLangs = 0;
  CurrentLangsFiltered = 0;
  HFileList = 0;
}

void DownloadDicsDlg::init (HINSTANCE hInst, HWND Parent, SpellChecker *SpellCheckerInstanceArg)
{
  SpellCheckerInstance = SpellCheckerInstanceArg;
  return Window::init (hInst, Parent);
}

DownloadDicsDlg::~DownloadDicsDlg ()
{
  if (Timer)
    DeleteTimerQueueTimer (0, Timer, 0);
  CLEAN_AND_ZERO (CurrentLangs);
  CLEAN_AND_ZERO (CurrentLangsFiltered);
  if (RefreshIcon)
    DestroyIcon (RefreshIcon);
}

void DownloadDicsDlg::IndicateThatSavingMightBeNeeded ()
{
  CheckIfSavingIsNeeded = 1;
}

static BOOL TryToCreateDir (TCHAR *Path)
{
  if (!CreateDirectory (Path, NULL))
  {
    TCHAR Message[DEFAULT_BUF_SIZE];
    _stprintf (Message, _T ("Can't create directory %s"), Path);
    MessageBox (0, Message, _T ("Error in directory creation"), MB_OK | MB_ICONERROR);
    return FALSE;
  }
  return TRUE;
}

static BOOL CheckForDirectoryExistence (TCHAR *Path)
{
  for (unsigned int i = 0; i < _tcslen (Path); i++)
  {
    if (Path[i] == _T ('\\'))
    {
      Path[i] = _T ('\0');
      if (!PathFileExists (Path))
      {
        if (!TryToCreateDir (Path))
          return FALSE;
      }
      Path[i] = _T ('\\');
    }
  }
  if (!PathFileExists (Path))
  {
    if (!TryToCreateDir (Path))
      return FALSE;
  }
  return TRUE;
}

#define BUF_SIZE_FOR_COPY 10240
void DownloadDicsDlg::DownloadSelected ()
{
  int Count = ListBox_GetCount (HFileList);
  TCHAR TempPath [MAX_PATH];
  TCHAR LocalPath [MAX_PATH];
  TCHAR *ConvertedDicName = 0;
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
  if (!CheckForDirectoryExistence (TempPath)) // If path isn't exist we're gonna try to create it else it's finish
  {
    MessageBox (0, _T ("Path defined as temporary dir doesn't exist and couldn't be created, probably one of subdirectories have limited access, please make temporary path valid."), _T ("Temporary Path is Broken"), MB_OK | MB_ICONEXCLAMATION);
    return;
  }
  CancelPressed = FALSE;
  int Failure = 0;
  _tcscpy (Message, _T ("Dictionaries copied:\n"));
  std::map<char *, int, bool (*)(char *, char *)> FilesFound (SortCompareChars); //0x01 - .aff found, 0x02 - .dic found
  Progress *p = GetProgress ();
  p->SetProgress (0);
  p->SetBottomMessage (_T (""));
  p->SetTopMessage (_T (""));
  int DownloadedCount = 0;
  int SupposedDownloadedCount = 0;
  if (!CheckForDirectoryExistence (SpellCheckerInstance->GetHunspellPath ())) // If path isn't exist we're gonna try to create it else it's finish
  {
    MessageBox (0, _T ("Directory for dictionaries doesn't exist and couldn't be created, probably one of subdirectories have limited access, please choose accessible directory for dictionaries"), _T ("Dictionaries Haven't Been Downloaded"), MB_OK | MB_ICONEXCLAMATION);
    return;
  }
  for (int i = 0; i < Count; i++)
  {
    if (CheckedListBox_GetCheckState (HFileList, i) == BST_CHECKED)
    {
      SupposedDownloadedCount++;
      FileName = new TCHAR [_tcslen (CurrentLangsFiltered->at (i).OrigName) + 4 + 1];
      FileName[0] = _T ('\0');
      _tcscat (FileName, CurrentLangsFiltered->at (i).OrigName);
      _tcscat (FileName, _T (".zip"));
      _stprintf (ProgMessage, _T ("Downloading %s..."), FileName);
      p->SetTopMessage (ProgMessage);
      _tcscpy (LocalPath, TempPath);
      _tcscat (LocalPath, FileName);
      ComboBox_GetText (HAddress, Buf, DEFAULT_BUF_SIZE);
      DoFtpOperation (DOWNLOAD_FILE, Buf, FileName, LocalPath);
      if (CancelPressed)
        break;
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
        BOOL CleanArr = TRUE;
        if (IsAffFile || IsDicFile)
        {
          DicFileNameANSI [strlen (DicFileNameANSI) - 4] = '\0';

          if (FilesFound.find (DicFileNameANSI) == FilesFound.end ())
          {
            FilesFound [DicFileNameANSI] = 0;
            CleanArr = FALSE;
          }

          it = FilesFound.find (DicFileNameANSI);

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
        if (CleanArr)
          CLEAN_AND_ZERO_ARR (DicFileNameANSI);
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
          _tcscpy (HunspellDicPath, SpellCheckerInstance->GetInstallSystem () ? SpellCheckerInstance->GetHunspellAdditionalPath () : SpellCheckerInstance->GetHunspellPath ());
          _tcscat (HunspellDicPath, _T ("\\"));
          _tcscat (HunspellDicPath, DicFileName);
          _tcscat (HunspellDicPath, _T (".aff"));
          BOOL Confirmation = TRUE;
          BOOL ReplaceQuestionWasAsked = FALSE;
          if (PathFileExists (HunspellDicPath))
          {
            TCHAR ReplaceMessage[DEFAULT_BUF_SIZE];
            _stprintf_s (ReplaceMessage, _T ("Looks like %s dictionary is already present. Do you want to replace it?"), DicFileName);
            ReplaceQuestionWasAsked = TRUE;
            if (MessageBox (0, ReplaceMessage, _T ("Dictionary already exists"), MB_YESNO) == IDNO)
            {
              Confirmation = FALSE;
              SetFileAttributes (DicFileLocalPath, FILE_ATTRIBUTE_NORMAL);
              DeleteFile (DicFileLocalPath);
            }
            else
            {
              SetFileAttributes (HunspellDicPath, FILE_ATTRIBUTE_NORMAL);
              DeleteFile (HunspellDicPath);
            }
          }

          if (Confirmation && !MoveFile (DicFileLocalPath, HunspellDicPath))
          {
            SetFileAttributes (DicFileLocalPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (DicFileLocalPath);
            Failure = 1;
          }
          _tcscpy (DicFileLocalPath + _tcslen (DicFileLocalPath) - 4, _T (".dic"));
          _tcscpy (HunspellDicPath + _tcslen (HunspellDicPath) - 4, _T (".dic"));
          if (!Confirmation)
          {
            SetFileAttributes (DicFileLocalPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (DicFileLocalPath);
          }
          else if (PathFileExists (HunspellDicPath))
          {
            int Res = 0;
            if (ReplaceQuestionWasAsked)
              Res = !Confirmation;
            else
            {
              TCHAR ReplaceMessage[DEFAULT_BUF_SIZE];
              _stprintf_s (ReplaceMessage, _T ("Looks like %s dictionary is already present. Do you want to replace it?"), DicFileName);
              Res = (MessageBox (0, ReplaceMessage, _T ("Dictionary already exists"), MB_YESNO) == IDNO);
            }
            if (Res)
            {
              SetFileAttributes (DicFileLocalPath, FILE_ATTRIBUTE_NORMAL);
              DeleteFile (DicFileLocalPath);
              Confirmation = FALSE;
            }
            else
            {
              SetFileAttributes (HunspellDicPath, FILE_ATTRIBUTE_NORMAL);
              DeleteFile (HunspellDicPath);
            }
          }

          if (Confirmation && !MoveFile (DicFileLocalPath, HunspellDicPath))
          {
            SetFileAttributes (DicFileLocalPath, FILE_ATTRIBUTE_NORMAL);
            DeleteFile (DicFileLocalPath);
            Failure = 1;
          }
          SetStringWithAliasApplied (ConvertedDicName, DicFileName);
          if (Failure)
            goto clean_and_continue;

          if (Confirmation)
          {
            _tcscat (Message, ConvertedDicName);
            _tcscat (Message, _T ("\n"));
            DownloadedCount++;
          }
          if (!Confirmation)
            SupposedDownloadedCount--;
        }
      }
clean_and_continue:
      it = FilesFound.begin ();
      for (;it != FilesFound.end (); it++)
        delete[] ((*it).first);

      FilesFound.clear ();
      unzClose (fp);
      SetFileAttributes (LocalPath, FILE_ATTRIBUTE_NORMAL);
      DeleteFile (LocalPath); // Removing downloaded .zip file
      CLEAN_AND_ZERO_ARR (FileName);
      if (Failure)
        break;
    }
  }
  GetProgress ()->display (false);
  if (Failure == 1)
    MessageBox (0, _T ("Access denied to dictionaries directory, either try to run Notepad++ as administrator or select some different, accessible dictionary path"), _T ("Dictionaries Haven't Been Downloaded"), MB_OK | MB_ICONEXCLAMATION);
  else if (DownloadedCount)
    MessageBox (0, Message, _T ("Dictionaries Downloaded Successfully"), MB_OK | MB_ICONINFORMATION);
  else if (SupposedDownloadedCount) // Otherwise - silent
    MessageBox (0, _T ("Sadly, no dictionaries were copied, please try again"), _T ("Dictionaries Haven't Been Downloaded"), MB_OK | MB_ICONEXCLAMATION);
  for (int i = 0; i < ListBox_GetCount (HFileList); i++)
    CheckedListBox_SetCheckState (HFileList, i, BST_UNCHECKED);
  SpellCheckerInstance->GetHunspellSpeller ()->SetDirectory (SpellCheckerInstance->GetHunspellPath ()); // Calling the update for Hunspell dictionary list
  SpellCheckerInstance->GetHunspellSpeller ()->SetAdditionalDirectory (SpellCheckerInstance->GetHunspellAdditionalPath ());
  SpellCheckerInstance->ReinitLanguageLists ();
  SpellCheckerInstance->DoPluginMenuInclusion ();
  CLEAN_AND_ZERO_ARR (LocalPathANSI);
  CLEAN_AND_ZERO_ARR (DicFileName);
  CLEAN_AND_ZERO_ARR (ConvertedDicName);
}

void FtpTrim (TCHAR *FtpAddress)
{
  StrTrim (FtpAddress, _T (" "));
  const TCHAR FtpPrefix[] = _T ("ftp://");
  int FtpPrefixLen = _tcslen (FtpPrefix);
  for (unsigned int i = 0; i < _tcslen (FtpAddress); i++) // Exchanging slashes
  {
    if (FtpAddress[i] == _T ('\\'))
      FtpAddress[i] = _T ('/');
  }

  if (_tcsncmp (FtpPrefix, FtpAddress, FtpPrefixLen) == 0) // Cutting out stuff like ftp://
  {
    for (unsigned int i = 0; i <= _tcslen (FtpAddress) - FtpPrefixLen; i++)
    {
      FtpAddress[i] = FtpAddress [i + FtpPrefixLen];
    }
  }

  for (unsigned int i = 0; i < _tcslen (FtpAddress); i++)
  {
    FtpAddress[i] = _totlower (FtpAddress[i]);
    if (FtpAddress[i] == '/')
      break; // In dir names upper/lower case could matter
  }
}

void DownloadDicsDlg::UpdateListBox ()
{
  if (!HFileList)
    return;

  CLEAN_AND_ZERO (CurrentLangsFiltered);
  CurrentLangsFiltered = new std::vector<LanguageName> ();
  for (unsigned int i = 0; i < CurrentLangs->size (); i++)
  {
    LanguageName Lang (CurrentLangs->at (i));
    if (SpellCheckerInstance->GetShowOnlyKnown () && !Lang.AliasApplied) // TODO: Add option to ignore/don't ignore non resolved package names
      continue;
    CurrentLangsFiltered->push_back (Lang);
  }
  ListBox_ResetContent (HFileList);
  for (unsigned int i = 0; i < CurrentLangsFiltered->size (); i++)
  {
    ListBox_AddString (HFileList, SpellCheckerInstance->GetDecodeNames () ? CurrentLangsFiltered->at (i).AliasName : CurrentLangsFiltered->at (i).OrigName);
  }
}

void DownloadDicsDlg::DoFtpOperation (FTP_OPERATION_TYPE Type, TCHAR *Address, TCHAR *FileName, TCHAR *Location)
{
  TCHAR *Folders = 0;
  if (Type == FILL_FILE_LIST)
  {
    EnableWindow (HInstallSelected, FALSE);
    StatusColor = COLOR_NEUTRAL;
    Static_SetText (HStatus, _T ("Status: Loading..."));
  }

  FtpTrim (Address);

  Folders = _tcschr (Address, _T ('/'));
  if (Folders != 0)
  {
    *Folders = _T ('\0');
    Folders++;
  }

  HINTERNET Session = InternetOpen (_T ("DSpellCheck"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
  if (!Session)
  {
    StatusColor = COLOR_FAIL;
    Static_SetText (HStatus, _T ("Status: Connection Error"));
    return;
  }

  DWORD TimeOut = 1000;
  InternetSetOption (Session, INTERNET_OPTION_CONNECT_TIMEOUT, &TimeOut, sizeof (DWORD));

  HINTERNET Internet = InternetConnect (Session, Address, INTERNET_INVALID_PORT_NUMBER, NULL, NULL, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE, 0);

  if (!Internet)
  {
    StatusColor = COLOR_FAIL;
    Static_SetText (HStatus, _T ("Status: Connection Error"));
    return;
  }

  if (Folders != 0) // Otherwise it's a root dir
  {
    DWORD Res = 0;
    if (!FtpSetCurrentDirectory (Internet, Folders))
    {
      StatusColor = COLOR_WARN;
      Static_SetText (HStatus, _T ("Status: Directory wasn't found"));
      goto cleanup;
    }
  }

  if (Type == FILL_FILE_LIST)
  {
    WIN32_FIND_DATA FindData;
    TCHAR *Buf = 0;
    HINTERNET FindHandle = FtpFindFirstFile (Internet, _T ("*.zip"), &FindData, 0, 0);
    if (!FindHandle)
    {
      StatusColor = COLOR_WARN;
      Static_SetText (HStatus, _T ("Status: Directory doesn't contain any zipped files"));
      goto cleanup;
    }
    CLEAN_AND_ZERO (CurrentLangs);
    CurrentLangs = new std::vector<LanguageName> ();
    do
    {
      SetString (Buf, FindData.cFileName);
      Buf[_tcslen (Buf) - 4] = 0;
      LanguageName Lang (Buf); // Probably should add options for using/not using aliases
      CurrentLangs->push_back (Lang);
    }
    while (InternetFindNextFile (FindHandle, &FindData));

    std::sort (CurrentLangs->begin (), CurrentLangs->end (), SpellCheckerInstance->GetDecodeNames () ? CompareAliases : CompareOriginal);

    UpdateListBox (); // Used only here and on filter change
    CLEAN_AND_ZERO_ARR (Buf);
    // If it is success when we perhaps should add this address to our list.
    int Len = ComboBox_GetTextLength (HAddress) + 1;
    if (CheckIfSavingIsNeeded)
    {
      TCHAR *NewServer = new TCHAR [Len];
      ComboBox_GetText (HAddress, NewServer, Len);
      PostMessageToMainThread (TM_ADD_USER_SERVER, (WPARAM) NewServer, 0);
    }
    StatusColor = COLOR_OK;
    Static_SetText (HStatus, _T ("Status: List of available files was successfully loaded"));
    EnableWindow (HInstallSelected, TRUE);
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
      return; // Then file couldn't be downloaded
    DWORD *BytesCopied = new DWORD;
    TCHAR Message[DEFAULT_BUF_SIZE];
    INTERNET_BUFFERS InetBuf;
    memset (&InetBuf, 0, sizeof (INTERNET_BUFFERS));
    InetBuf.dwStructSize = sizeof (INTERNET_BUFFERS);
    HINTERNET File = FtpOpenFile (Internet, FileName, GENERIC_READ , FTP_TRANSFER_TYPE_BINARY, 0);
    LSize = FtpGetFileSize (File, &HSize);
    Size = LSize;
    while (InternetReadFile (File, DataBuf, BUF_SIZE_FOR_COPY, BytesCopied))
    {
      if (!*BytesCopied)
        break;

      BytesSum += *BytesCopied;
      GetProgress ()->SetProgress (BytesSum * 100 / Size);

      _stprintf (Message, _T ("%d / %d bytes downloaded (%d %%)"), BytesSum, Size, BytesSum * 100 / Size);
      GetProgress ()->SetBottomMessage (Message);
      _write (FileHandle, DataBuf, *BytesCopied);
      if (WaitForEvent (EID_CANCEL_DOWNLOAD, 0) == WAIT_OBJECT_0)
      {
        CancelPressed = TRUE;
        _close (FileHandle);
        DeleteFile (Location);
        return;
      }
    }
    CLEAN_AND_ZERO (BytesCopied);
    _close (FileHandle);
    InternetCloseHandle (File);
  }

cleanup:
  InternetCloseHandle (Internet);
}

VOID CALLBACK ReinitServer (
  PVOID lpParameter,
  BOOLEAN TimerOrWaitFired
  )
{
  DownloadDicsDlg *DlgInstance = ((DownloadDicsDlg *) lpParameter);
  DlgInstance->IndicateThatSavingMightBeNeeded ();
  DlgInstance->OnDisplayAction ();
  DlgInstance->RemoveTimer ();
}

void DownloadDicsDlg::RemoveTimer ()
{
  DeleteTimerQueueTimer (0, Timer, 0);
  Timer = 0;
}

void DownloadDicsDlg::SetOptions (BOOL ShowOnlyKnown, BOOL InstallSystem)
{
  Button_SetCheck (HShowOnlyKnown, ShowOnlyKnown ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck (HInstallSystem, InstallSystem ? BST_CHECKED : BST_UNCHECKED);
}

void DownloadDicsDlg::UpdateOptions (SpellChecker *SpellCheckerInstance)
{
  SpellCheckerInstance->SetShowOnlyKnow (Button_GetCheck (HShowOnlyKnown) == BST_CHECKED);
  SpellCheckerInstance->SetInstallSystem (Button_GetCheck (HInstallSystem) == BST_CHECKED);
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
      HInstallSelected = ::GetDlgItem (_hSelf, IDOK);
      HShowOnlyKnown = ::GetDlgItem (_hSelf, IDC_SHOWONLYKNOWN);
      HRefresh = ::GetDlgItem (_hSelf, IDC_REFRESH);
      HInstallSystem = ::GetDlgItem (_hSelf, IDC_INSTALL_SYSTEM);
      // RefreshIcon = LoadIcon (_hInst, MAKEINTRESOURCE (IDI_ICON1));
      RefreshIcon = (HICON) LoadImage (_hInst, MAKEINTRESOURCE (IDI_ICON1), IMAGE_ICON, 16, 16, 0);
      SendMessage (HRefresh, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) RefreshIcon);
      //ComboBox_SetText(HAddress, _T ("ftp://127.0.0.1"));
      SendEvent (EID_INIT_DOWNLOAD_COMBOBOX);
      //ComboBox_SetText(HAddress, _T ("ftp://gd.tuwien.ac.at/office/openoffice/contrib/dictionaries"));
      SendEvent (EID_FILL_DOWNLOAD_DICS_DIALOG);
      OnDisplayAction ();
      DefaultBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
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
        {
          for (int i = 0; i < ListBox_GetCount (HFileList); i++)
            CheckedListBox_SetCheckState (HFileList, i, BST_UNCHECKED);
          SendEvent (EID_HIDE_DOWNLOAD_DICS);
        }

        break;
      case IDC_ADDRESS:
        if (HIWORD (wParam) == CBN_EDITCHANGE)
        {
          if (Timer)
            ChangeTimerQueueTimer (0, Timer, 1000, 0);
          else
            CreateTimerQueueTimer (&Timer, 0, ReinitServer, this, 1000, 0 , 0);
        }
        else if (HIWORD (wParam) == CBN_SELCHANGE)
        {
          ReinitServer (this, FALSE);
          CheckIfSavingIsNeeded = 0;
        }
        break;
      case IDC_REFRESH:
        {
          ReinitServer (this, FALSE);
        }
        break;
      case IDC_INSTALL_SYSTEM:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          SendEvent (EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS_NO_UPDATE);
        }
        break;
      case IDC_SHOWONLYKNOWN:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          SendEvent (EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS);
        }
        break;
      }
    }
    break;
  case WM_CTLCOLORSTATIC:
    if (HStatus == (HWND)lParam)
    {
      HDC hDC = (HDC)wParam;
      SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
      // SetBkMode(hDC, TRANSPARENT);
      SetTextColor(hDC, StatusColor);
      return (INT_PTR) DefaultBrush;
    }
    break;
  case WM_CLOSE:
    DeleteObject (DefaultBrush);
    break;
  };
  return FALSE;
}