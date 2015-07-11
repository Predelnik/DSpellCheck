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
#include "Definements.h"
#include "DownloadDicsDlg.h"
#include "Settings.h"

#include "FTPClient.h"
#include "FTPDataTypes.h"
#include "HunspellController.h"
#include "LanguageName.h"
#include "Plugin.h"
#include "Progress.h"
#include "resource.h"
#include "SelectProxy.h"
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
  SetFocus (HFileList);
}

void DownloadDicsDlg::FillFileList ()
{
  TCHAR Buf[DEFAULT_BUF_SIZE];
  ComboBox_GetText (HAddress, Buf, DEFAULT_BUF_SIZE);
  GetDownloadDics ()->DoFtpOperation (FILL_FILE_LIST, Buf);
}

void DownloadDicsDlg::OnDisplayAction ()
{
  SendNetworkEvent (EID_FILL_FILE_LIST);
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

DWORD DownloadDicsDlg::AskReplacementMessage (TCHAR *DicName)
{
  TCHAR ReplaceMessage[DEFAULT_BUF_SIZE];
  TCHAR *TBuf = 0;
  SetStringWithAliasApplied (TBuf, DicName);
  _stprintf_s (ReplaceMessage, _T ("Looks like %s dictionary is already present. Do you want to replace it?"), TBuf);
  CLEAN_AND_ZERO_ARR (TBuf);
  MessageBoxInfo MsgBox (_hParent, ReplaceMessage, _T ("Dictionary already exists"), MB_YESNO);
  return SendMessage (_hParent, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX), (WPARAM) &MsgBox, 0);
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
  if (!CheckForDirectoryExistence (TempPath, FALSE, _hParent)) // If path isn't exist we're gonna try to create it else it's finish
  {
    MessageBoxInfo MsgBox (_hParent, _T ("Path defined as temporary dir doesn't exist and couldn't be created, probably one of subdirectories have limited access, please make temporary path valid."), _T ("Temporary Path is Broken"), MB_OK | MB_ICONEXCLAMATION);
    SendMessage (_hParent, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
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
  auto settings = SpellCheckerInstance->getSettings ();
  auto dicPath = SpellCheckerInstance->GetInstallSystem () ? settings->spellerSettings[SpellerType::hunspell].path.data () : settings->spellerSettings[SpellerType::hunspell].additionalPath.data ();
  if (!CheckForDirectoryExistence (dicPath, 0, _hParent)) // If path doesn't exist we're gonna try to create it else it's finish
  {
    MessageBoxInfo MsgBox (_hParent, _T ("Directory for dictionaries doesn't exist and couldn't be created, probably one of subdirectories have limited access, please choose accessible directory for dictionaries"), _T ("Dictionaries Haven't Been Downloaded"), MB_OK | MB_ICONEXCLAMATION);
    SendMessage (_hParent, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
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
      setString (LocalPathANSI, LocalPath);
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
          setString (DicFileName, DicFileNameANSI);
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
          setString (DicFileName, (*it).first);
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
          setString (DicFileName, (*it).first);
          _tcscat (DicFileLocalPath, DicFileName);
          _tcscat (DicFileLocalPath, _T (".aff"));
          _tcscpy (HunspellDicPath, dicPath);
          _tcscat (HunspellDicPath, _T ("\\"));
          _tcscat (HunspellDicPath, DicFileName);
          _tcscat (HunspellDicPath, _T (".aff"));
          BOOL Confirmation = TRUE;
          BOOL ReplaceQuestionWasAsked = FALSE;
          if (PathFileExists (HunspellDicPath))
          {
            DWORD Answer = AskReplacementMessage (DicFileName);
            ReplaceQuestionWasAsked = TRUE;
            if (Answer == IDNO)
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
              Res = (AskReplacementMessage (DicFileName) == IDNO);
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
  {
    MessageBoxInfo MsgBox (_hParent, _T ("Access denied to dictionaries directory, either try to run Notepad++ as administrator or select some different, accessible dictionary path"), _T ("Dictionaries Haven't Been Downloaded"), MB_OK | MB_ICONEXCLAMATION);
    SendMessage (_hParent, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
  }
  else if (DownloadedCount)
  {
    MessageBoxInfo MsgBox (_hParent, Message, _T ("Dictionaries Downloaded"), MB_OK | MB_ICONINFORMATION);
    SendMessage (_hParent, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
  }
  else if (SupposedDownloadedCount) // Otherwise - silent
  {
    MessageBoxInfo MsgBox (_hParent, _T ("Sadly, no dictionaries were copied, please try again"), _T ("Dictionaries Haven't Been Downloaded"), MB_OK | MB_ICONEXCLAMATION);
    SendMessage (_hParent, getCustomGUIMessageId (CustomGUIMessage::DO_MESSAGE_BOX),  (WPARAM) &MsgBox, 0);
  }
  for (int i = 0; i < ListBox_GetCount (HFileList); i++)
    CheckedListBox_SetCheckState (HFileList, i, BST_UNCHECKED);
  SendEvent (EID_HUNSPELL_DICTIONARIES_CHANGE);
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
  if (!HFileList || !CurrentLangs)
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

  auto settings = getSpellChecker()->getSettings ();
  for (unsigned int i = 0; i < CurrentLangsFiltered->size (); i++)
  {
    ListBox_AddString (HFileList, settings->useLanguageNameAliases ? CurrentLangsFiltered->at (i).AliasName : CurrentLangsFiltered->at (i).OrigName);
  }
}

// Copy of CFile with ability to kick progress bar
class Observer : public nsFTP::CFTPClient::CNotification
{
  FILE* m_pFile;
  tstring m_strFileName;
  Progress *ProgressInstance;
  DownloadDicsDlg *DownloadDicsInstance;
  long BytesReceived;
  long TargetSize;
  nsFTP::CFTPClient *FtpSession;
public:
  enum TOriginEnum { orBegin=SEEK_SET, orEnd=SEEK_END, orCurrent=SEEK_CUR };

  Observer (Progress *ProgressArg, long TargetSizeArg, nsFTP::CFTPClient *FtpSessionArg, DownloadDicsDlg *DownloadDicsArg) : m_pFile(NULL)
  {
    ProgressInstance = ProgressArg;
    TargetSize = TargetSizeArg;
    FtpSession = FtpSessionArg;
    DownloadDicsInstance = DownloadDicsArg;
    BytesReceived = 0;
  }

  virtual void OnBytesReceived(const nsFTP::TByteVector& /*vBuffer*/, long lReceivedBytes) override
  {
    BytesReceived += lReceivedBytes;
    ProgressInstance->SetProgress (BytesReceived * 100 / TargetSize);
    TCHAR Message[DEFAULT_BUF_SIZE];

    _stprintf (Message, _T ("%d / %d bytes downloaded (%d %%)"), BytesReceived, TargetSize, BytesReceived * 100 / TargetSize);
    ProgressInstance->SetBottomMessage (Message);

    if (WaitForNetworkEvent (EID_CANCEL_DOWNLOAD, 0) == WAIT_OBJECT_0)
    {
      ProgressInstance->SetBottomMessage (_T ("Aborting Download..."));
      DownloadDicsInstance->SetCancelPressed (TRUE);
      FtpSession->Abort ();
      return;
    }
  }
};

void DownloadDicsDlg::SetCancelPressed (BOOL Value)
{
  CancelPressed = Value;
}

#define INITIAL_BUFFER_SIZE 50 * 1024
#define INITIAL_SMALL_BUFFER_SIZE 10 * 1024
void DownloadDicsDlg::DoFtpOperationThroughHttpProxy (FTP_OPERATION_TYPE Type, TCHAR *Address, TCHAR *FileName, TCHAR *Location)
{
  TCHAR *ProxyFinalString = new TCHAR [_tcslen (SpellCheckerInstance->GetProxyHostName ()) + 10];
  char *FileBuffer = 0;
  _stprintf (ProxyFinalString, _T ("%s:%d"), SpellCheckerInstance->GetProxyHostName (), SpellCheckerInstance->GetProxyPort ());
  HINTERNET WinInetHandle = InternetOpen (_T ("DSpellCheck"), INTERNET_OPEN_TYPE_PROXY, ProxyFinalString, _T (""), 0);
  CLEAN_AND_ZERO_ARR (ProxyFinalString);
  FtpTrim (Address);
  TCHAR *Url = new TCHAR[_tcslen (Address) + (Type == DOWNLOAD_FILE ? _tcslen (FileName) : 0) + 6 + 1];

  if (!WinInetHandle)
  {
    if (Type == FILL_FILE_LIST)
    {
      StatusColor = COLOR_FAIL;
      Static_SetText (HStatus, _T ("Status: Connection cannot be established"));
    }
    goto cleanup;
  }

  _tcscpy (Url, _T ("ftp://"));
  _tcscat (Url, Address);
  if (Type == DOWNLOAD_FILE)
    _tcscat (Url, FileName);

  DWORD TimeOut = 15000;
  InternetSetOption (WinInetHandle, INTERNET_OPTION_CONNECT_TIMEOUT, &TimeOut, sizeof (DWORD));
  InternetSetOption (WinInetHandle, INTERNET_OPTION_SEND_TIMEOUT, &TimeOut, sizeof (DWORD));
  InternetSetOption (WinInetHandle, INTERNET_OPTION_RECEIVE_TIMEOUT, &TimeOut, sizeof (DWORD));

  HINTERNET OpenedURL = 0;
  OpenedURL = InternetOpenUrl (WinInetHandle, Url, 0, 0, INTERNET_FLAG_PASSIVE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
  if (!OpenedURL)
  {
    if (Type == FILL_FILE_LIST)
    {
      StatusColor = COLOR_FAIL;
      TCHAR Buf[256];

      _stprintf (Buf, _T ("Status: URL cannot be opened (Error code: %d)"), GetLastError ());
      Static_SetText (HStatus, Buf);
      goto cleanup;
    }
  }

  if (!SpellCheckerInstance->GetProxyAnonymous ())
  {
    InternetSetOption (OpenedURL, INTERNET_OPTION_PROXY_USERNAME,
      (LPVOID) SpellCheckerInstance->GetProxyUserName (), _tcslen (SpellCheckerInstance->GetProxyUserName ()) + 1);
    InternetSetOption (OpenedURL, INTERNET_OPTION_PROXY_PASSWORD,
      (LPVOID) SpellCheckerInstance->GetProxyPassword (), _tcslen (SpellCheckerInstance->GetProxyPassword ()) + 1);
    InternetSetOption (OpenedURL, INTERNET_OPTION_PROXY_SETTINGS_CHANGED,
      0, 0);
    HttpSendRequest (OpenedURL, 0, 0, 0, 0);
  }

  DWORD Code = 0;
  DWORD Dummy = 0;
  DWORD Size = sizeof (DWORD);

  /*
  char Buffer[10000];
  DWORD Length = 10000;
  HttpQueryInfo (OpenedURL, HTTP_QUERY_RAW_HEADERS_CRLF, Buffer, &Length, &Dummy);
  */

  if (!HttpQueryInfo (OpenedURL, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &Code, &Size, &Dummy))
  {
    if (Type == FILL_FILE_LIST)
    {
      StatusColor = COLOR_FAIL;
      TCHAR Buf[256];
      _stprintf (Buf, _T ("Status: Query status code failed (Error: %d)"), GetLastError ());
      Static_SetText (HStatus, Buf);
    }
    goto cleanup;
  }
  if (Code != 200)
  {
    if (Type == FILL_FILE_LIST)
    {
      StatusColor = COLOR_FAIL;
      if (Code == HTTP_STATUS_PROXY_AUTH_REQ )
        Static_SetText (HStatus, _T ("Status: Proxy Authorization Required"));
      else
      {
        TCHAR Buf[256];

        _stprintf (Buf, _T ("Status: Bad status code (%d)"), Code);
        Static_SetText (HStatus, Buf);
      }
    }
    goto cleanup;
  }

  if (Type == FILL_FILE_LIST)
  {
    FileBuffer = new char [INITIAL_BUFFER_SIZE];
    char *CurPos = FileBuffer;
    char *TempBuf = 0;
    DWORD BytesRead = 0;
    DWORD BytesReadTotal = 0;
    DWORD BytesToRead = 0;
    unsigned int CurBufSize = INITIAL_BUFFER_SIZE;
    while (1)
    {
      InternetQueryDataAvailable (OpenedURL, &BytesToRead, 0, 0);
      if (BytesToRead == 0)
        break;
      if (BytesReadTotal + BytesToRead > CurBufSize)
      {
        TempBuf = new char[CurBufSize * 2];
        memcpy (TempBuf, FileBuffer, CurBufSize);
        CurPos = CurPos - FileBuffer + TempBuf;
        CLEAN_AND_ZERO_ARR (FileBuffer);
        FileBuffer = TempBuf;
        CurBufSize *= 2;
      }

      InternetReadFile (OpenedURL, CurPos, BytesToRead, &BytesRead);
      if (BytesRead == 0)
        break;
      BytesReadTotal += BytesRead;
      CurPos += BytesRead;
    }
    CurPos = FileBuffer;
    TCHAR *FileName = 0;
    int count = 0;
    // Bad Parsing. Really, really bad. I'm sorry :(
    if (CurPos == 0)
    {
      StatusColor = COLOR_FAIL;
      Static_SetText (HStatus, _T ("Status: Proxy HTTP Output cannot be parsed"));
      goto cleanup;
    }
    while ((size_t) (CurPos - FileBuffer) < BytesReadTotal)
    {
      char *TempCurPos = CurPos;
      CurPos = strstr (CurPos, "</A>");
      if (CurPos == 0)
        CurPos = strstr (TempCurPos, "</a>");

      if (CurPos == 0)
        break;
      TempCurPos = CurPos;
      while (*TempCurPos != '>' && TempCurPos > FileBuffer)
        TempCurPos--;

      if (TempCurPos == 0)
      {
        StatusColor = COLOR_FAIL;
        Static_SetText (HStatus, _T ("Status: Proxy HTTP Output cannot be parsed"));
        goto cleanup;
      }
      TempCurPos++;
      CurPos--;
      if (CurPos <= TempCurPos)
      {
        CurPos += 2;
        continue;
      }
      TempBuf = new char [CurPos - TempCurPos + 1 + 1];
      strncpy (TempBuf, TempCurPos, CurPos - TempCurPos + 1);
      TempBuf[CurPos - TempCurPos + 1] = '\0';
      CurPos += 2;
      if (!PathMatchSpecA (TempBuf, "*.zip"))
      {
        CLEAN_AND_ZERO_ARR (TempBuf);
        continue;
      }

      TempBuf[strlen (TempBuf) - 4] = '\0';
      count++;

      FileName = 0;
      setString (FileName, TempBuf);

      LanguageName Lang (FileName); // Probably should add options for using/not using aliases
      CurrentLangs->push_back (Lang);
      CLEAN_AND_ZERO_ARR (TempBuf);
      CLEAN_AND_ZERO_ARR (FileName);
    }

    if (count == 0)
    {
      StatusColor = COLOR_WARN;
      Static_SetText (HStatus, _T ("Status: Directory doesn't contain any zipped files"));
      goto cleanup;
    }

    {
      auto settings = getSpellChecker ()->getSettings ();
      std::sort (CurrentLangs->begin (), CurrentLangs->end (), settings->useLanguageNameAliases ? CompareAliases : CompareOriginal);
    }

    UpdateListBox (); // Used only here and on filter change
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
    FileBuffer = new char [INITIAL_SMALL_BUFFER_SIZE];
    DWORD CurBufSize = INITIAL_SMALL_BUFFER_SIZE;
    DWORD BytesToRead = 0;
    DWORD BytesRead;
    DWORD BytesReadTotal = 0;
    FileName[_tcslen (FileName) - 4] = _T ('\0');
    TCHAR Message[DEFAULT_BUF_SIZE];

    if (PathFileExists (Location))
    {
      SetFileAttributes (Location, FILE_ATTRIBUTE_NORMAL);
      DeleteFile (Location);
    }
    if (PathFileExists (Location))
    {
      SetFileAttributes (Location, FILE_ATTRIBUTE_NORMAL);
      DeleteFile (Location);
    }
    int FileHandle = _topen (Location, _O_CREAT | _O_BINARY | _O_WRONLY);
    if (!FileHandle)
      goto cleanup;

    GetProgress ()->SetMarquee (true);
    while (1)
    {
      InternetQueryDataAvailable (OpenedURL, &BytesToRead, 0, 0);
      if (BytesToRead == 0)
        break;
      if (BytesToRead > CurBufSize)
      {
        CLEAN_AND_ZERO_ARR (FileBuffer);
        FileBuffer = new char[BytesToRead];
        CurBufSize = BytesToRead;
      }

      InternetReadFile (OpenedURL, FileBuffer, BytesToRead, &BytesRead);
      if (BytesRead == 0)
        break;

      write (FileHandle, FileBuffer, BytesRead);
      BytesReadTotal += BytesRead;

      _stprintf (Message, _T ("%d / ???   bytes downloaded"), BytesReadTotal);

      GetProgress ()->SetBottomMessage (Message);

      if (WaitForNetworkEvent (EID_CANCEL_DOWNLOAD, 0) == WAIT_OBJECT_0)
      {
        GetProgress ()->SetBottomMessage (_T ("Aborting Download..."));
        SetCancelPressed (TRUE);
        break;
      }
    }
    _close (FileHandle);
    GetProgress ()->SetMarquee (false);
  }
cleanup:
  if (WinInetHandle)
    InternetCloseHandle (WinInetHandle);
  CLEAN_AND_ZERO_ARR (FileBuffer);
  CLEAN_AND_ZERO_ARR (Url);
}

void DownloadDicsDlg::DoFtpOperation (FTP_OPERATION_TYPE Type, TCHAR *Address, TCHAR *FileName, TCHAR *Location)
{
  TCHAR *Folders = 0;
  Observer *ProgressUpdater = 0;
  if (Type == FILL_FILE_LIST)
  {
    EnableWindow (HInstallSelected, FALSE);
    StatusColor = COLOR_NEUTRAL;
    Static_SetText (HStatus, _T ("Status: Loading..."));
    ListBox_ResetContent (HFileList);
    CLEAN_AND_ZERO (CurrentLangs);
    CurrentLangs = new std::vector<LanguageName> ();
  }

  if (SpellCheckerInstance->GetUseProxy () && SpellCheckerInstance->GetProxyType () == 0)
  {
    DoFtpOperationThroughHttpProxy (Type, Address, FileName, Location);
    return;
  }

  FtpTrim (Address);
  Folders = _tcschr (Address, _T ('/'));
  if (Folders != 0)
  {
    *Folders = _T ('\0');
    Folders++;
  }

  nsFTP::CLogonInfo *logonInfo = 0;
  nsFTP::CFTPClient ftpClient (nsSocket::CreateDefaultBlockingSocketInstance (), 3);
  if (!SpellCheckerInstance->GetUseProxy ())
    logonInfo = new nsFTP::CLogonInfo (Address, 21, _T ("anonymous"), _T (""), _T (""));
  else
    logonInfo = new nsFTP::CLogonInfo (
    Address, 21, _T ("anonymous"), _T (""), _T (""),
    SpellCheckerInstance->GetProxyHostName (),
    _T (""), _T (""),
    static_cast<USHORT> (SpellCheckerInstance->GetProxyPort ()),
    nsFTP::CFirewallType::UserWithNoLogon ()
    );

  if (!ftpClient.Login(*logonInfo))
  {
    if (Type == FILL_FILE_LIST)
    {
      StatusColor = COLOR_FAIL;
      Static_SetText (HStatus, _T ("Status: Connection Error"));
    }
    goto cleanup;
  }

  if (Type == FILL_FILE_LIST)
  {
    nsFTP::TFTPFileStatusShPtrVec List;

    if (!ftpClient.List (Folders, List, true))
    {
      StatusColor = COLOR_FAIL;
      Static_SetText (HStatus, _T ("Status: Can't list directory files"));
      goto cleanup;
    }

    TCHAR *Buf = 0;
    int count = 0;

    for (unsigned int i = 0; i < List.size (); i++)
    {
      if (!PathMatchSpec (List.at (i)->Name ().c_str (), _T ("*.zip")))
        continue;

      count++;
      setString (Buf, List.at (i)->Name ().c_str ());
      Buf[_tcslen (Buf) - 4] = 0;
      LanguageName Lang (Buf); // Probably should add options for using/not using aliases
      CurrentLangs->push_back (Lang);
    }

    if (count == 0)
    {
      StatusColor = COLOR_WARN;
      Static_SetText (HStatus, _T ("Status: Directory doesn't contain any zipped files"));
      goto cleanup;
    }

    {
      auto settings = getSpellChecker()->getSettings();
      std::sort (CurrentLangs->begin (), CurrentLangs->end (), settings->useLanguageNameAliases ? CompareAliases : CompareOriginal);
    }


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
    if (PathFileExists (Location))
    {
      SetFileAttributes (Location, FILE_ATTRIBUTE_NORMAL);
      DeleteFile (Location);
    }
    /*
    int FileHandle = _topen (Location, _O_CREAT | _O_BINARY | _O_WRONLY);
    if (FileHandle == -1)
    goto cleanup; // Then file couldn't be downloaded
    close (FileHandle);
    */

    if (ftpClient.ChangeWorkingDirectory (Folders) != nsFTP::FTP_OK)
      goto cleanup;

    long FileSize = 0;
    if (ftpClient.FileSize (FileName, FileSize) != nsFTP::FTP_OK)
      goto cleanup;

    ProgressUpdater = new Observer (GetProgress (), FileSize, &ftpClient, this);
    ftpClient.AttachObserver (ProgressUpdater);

    if (!ftpClient.DownloadFile (FileName, Location, nsFTP::CRepresentation(nsFTP::CType::Image()), true))
    {
      if (PathFileExists (Location))
      {
        SetFileAttributes (Location, FILE_ATTRIBUTE_NORMAL);
        DeleteFile (Location);
      }
      goto cleanup;
    }

    ftpClient.DetachObserver (ProgressUpdater);
  }
cleanup:
  if (ProgressUpdater)
    ftpClient.DetachObserver (ProgressUpdater);
  CLEAN_AND_ZERO (ProgressUpdater);
  CLEAN_AND_ZERO (logonInfo);
  if (ftpClient.IsConnected ())
    ftpClient.Logout ();
  return;
}

VOID CALLBACK ReinitServer (
  PVOID lpParameter,
  BOOLEAN /*TimerOrWaitFired*/
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

void DownloadDicsDlg::Refresh ()
{
  ReinitServer (this, FALSE);
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
      RefreshIcon = (HICON) LoadImage (_hInst, MAKEINTRESOURCE (IDI_REFRESH), IMAGE_ICON, 16, 16, 0);
      SendMessage (HRefresh, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) RefreshIcon);
      //ComboBox_SetText(HAddress, _T ("ftp://127.0.0.1"));
      SendEvent (EID_INIT_DOWNLOAD_COMBOBOX);
      //ComboBox_SetText(HAddress, _T ("ftp://gd.tuwien.ac.at/office/openoffice/contrib/dictionaries"));
      SendEvent (EID_FILL_DOWNLOAD_DICS_DIALOG);
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
          SendNetworkEvent (EID_DOWNLOAD_SELECTED);
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
          SendEvent (EID_UPDATE_FROM_DOWNLOAD_DICS_OPTIONS_NO_UPDATE);
          ReinitServer (this, FALSE);
          CheckIfSavingIsNeeded = 0;
        }
        break;
      case IDC_REFRESH:
        {
          Refresh ();
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
      case IDC_SELECTPROXY:
        if (HIWORD (wParam) == BN_CLICKED)
        {
          GetSelectProxy ()->DoDialog ();
          SendEvent (EID_SHOW_SELECT_PROXY);
        }
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