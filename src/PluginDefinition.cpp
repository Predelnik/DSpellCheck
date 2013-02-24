//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"

//
// put the headers you need here
//
#include <stdlib.h>
#include <time.h>
#include <tchar.h>
#include <shlwapi.h>
#include "MainDef.h"

#include "aspell.h"

const char configFileName[] = "DSpellCheck.ini";

#ifdef UNICODE
#define generic_itoa _itow
#else
#define generic_itoa itoa
#endif

FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

char iniFilePath[MAX_PATH];
bool doCloseTag = false;
BOOL AutoCheckText = false;

//
// Initialize your plugin data here
// It will be called while plugin loading
void pluginInit(HANDLE hModule)
{
  LoadAspell (0);
  AspellReinitSettings ();
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp ()
{
  AspellClear ();
  //WritePrivateProfileString(sectionName, keyName, doCloseTag?TEXT("1"):TEXT("0"), iniFilePath);
}

void LoadSettings ()
{
  char Buf[DEFAULT_BUF_SIZE];
  GetPrivateProfileStringA ("SpellCheck", "Autocheck", "0", Buf, DEFAULT_BUF_SIZE, iniFilePath);
  AutoCheckText = atoi (Buf);
  updateAutocheckStatus ();
  GetPrivateProfileStringA ("SpellCheck", "Language", "En_Us", Buf, DEFAULT_BUF_SIZE, iniFilePath);
  SetLanguage (Buf);
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
  //
  // Firstly we get the parameters from your plugin config file (if any)
  //

  // get path of plugin configuration
  ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniFilePath);

  // if config path doesn't exist, we create it
  if (PathFileExistsA(iniFilePath) == FALSE)
  {
    ::CreateDirectoryA(iniFilePath, NULL);
  }

  // make your plugin config file full file path name
  PathAppendA(iniFilePath, configFileName);

  //--------------------------------------------//
  //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
  //--------------------------------------------//
  // with function :
  // setCommand(int index,                      // zero based number to indicate the order of command
  //            TCHAR *commandName,             // the command name that you want to see in plugin menu
  //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
  //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
  //            bool check0nInit                // optional. Make this menu item be checked visually
  //            );
  setCommand(0, TEXT("Auto-check document"), SwitchAutoCheckText, NULL, false);
  setCommand(1, TEXT("---"), NULL, NULL, false);
  setCommand(2, TEXT("Settings..."), startSettings, NULL, false);

  // Here you insert a separator

  // Shortcut :
  // Following code makes the first command
  // bind to the shortcut Alt-F
  ShortcutKey *shKey = new ShortcutKey;
  shKey->_isAlt = true;
  shKey->_isCtrl = false;
  shKey->_isShift = false;
  shKey->_key = 0x46; //VK_F

  setCommand(4, TEXT("Current Full Path"), insertCurrentFullPath, shKey, false);
  setCommand(5, TEXT("Current File Name"), insertCurrentFileName, NULL, false);
  setCommand(6, TEXT("Current Directory"), insertCurrentDirectory, NULL, false);
  setCommand(7, TEXT("Date & Time - short format"), insertShortDateTime, NULL, false);
  setCommand(8, TEXT("Date & Time - long format"), insertLongDateTime, NULL, false);

  ShortcutKey *pShKey = new ShortcutKey;
  pShKey->_isAlt = true;
  pShKey->_isCtrl = false;
  pShKey->_isShift = false;
  pShKey->_key = 0x51; //VK_Q
  setCommand(9, TEXT("Close HTML/XML tag automatically"), insertHtmlCloseTag, pShKey, doCloseTag);

  setCommand(10, TEXT("---"), NULL, NULL, false);

  setCommand(11, TEXT("Get File Names Demo"), getFileNamesDemo, NULL, false);
  setCommand(12, TEXT("Get Session File Names Demo"), getSessionFileNamesDemo, NULL, false);
  setCommand(13, TEXT("Save Current Session Demo"), saveCurrentSessionDemo, NULL, false);

  setCommand(14, TEXT("---"), NULL, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
  // Don't forget to deallocate your shortcut here
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void hello()
{
  // Open a new document
  ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

  // Get the current scintilla
  int which = -1;
  ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
  if (which == -1)
    return;
  HWND curScintilla = (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;

  // Say hello now :
  // Scintilla control has no Unicode mode, so we use (char *) here
  ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)"Hello, Notepad++!");
}

static DWORD WINAPI threadZoomer(void *)
{
  // Get the current scintilla
  int which = -1;
  ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
  if (which == -1)
    return FALSE;

  HWND curScintilla = (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;

  int currentZoomLevel = (int)::SendMessage(curScintilla, SCI_GETZOOM, 0, 0);

  int i = currentZoomLevel;
  for (int j = 0 ; j < 4 ; j++)
  {
    for ( ; i >= -10 ; i--)
    {
      ::SendMessage(curScintilla, SCI_SETZOOM, i, 0);
      Sleep(30);
    }
    Sleep(100);
    for ( ; i <= 20 ; i++)
    {
      Sleep(30);
      ::SendMessage(curScintilla, SCI_SETZOOM, i, 0);
    }
    Sleep(100);
  }

  Sleep(100);
  for ( ; i >= currentZoomLevel ; i--)
  {
    Sleep(30);
    ::SendMessage(curScintilla, SCI_SETZOOM, i, 0);
  }
  return TRUE;
};

void helloFX()
{
  hello();
  HANDLE hThread = ::CreateThread(NULL, 0, threadZoomer, 0, 0, NULL);
  ::CloseHandle(hThread);
}

HWND getScintillaWindow()
{
  // Get the current scintilla
  int which = -1;
  SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
  if (which == -1)
    return NULL;
  return (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
}

BOOL ConvertingIsNeeded = FALSE;

void setEncodingById (int EncId)
{
  /*
  int CCH;
  char szCodePage[10];
  char *FinalString;
  */
  switch (EncId)
  {
  case SC_CP_UTF8:
    ConvertingIsNeeded = FALSE;
    // SetEncoding ("utf-8");
    break;
  default:
    {
      ConvertingIsNeeded = TRUE;
      /*
      CCH = GetLocaleInfoA (GetSystemDefaultLCID(), // or any LCID you may be interested in
      LOCALE_IDEFAULTANSICODEPAGE,
      szCodePage,
      countof (szCodePage));
      FinalString = new char [2 + strlen (szCodePage) + 1];
      strcpy (FinalString, "cp");
      strcat (FinalString, szCodePage);
      SetEncoding (FinalString);
      break;
      */
    }
  }
}

LRESULT SendMsgToEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
  HWND wEditor = getScintillaWindow();
  return SendMessage(wEditor, Msg, wParam, lParam);
}

void CreateWordUnderline (int start, int end)
{
  SendMsgToEditor (SCI_INDICATORFILLRANGE, start, (end - start + 1));
}

void RemoveWordUnderline (int start, int end)
{
  SendMsgToEditor (SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
  SendMsgToEditor (SCI_INDICATORCLEARRANGE, start, (end - start + 1));
}

// Warning - temporary buffer will be created
char *GetDocumentText ()
{
  int lengthDoc = (SendMsgToEditor (SCI_GETLENGTH) + 1);
  char * buf = (char *)malloc((sizeof(char) * lengthDoc));
  SendMsgToEditor (SCI_GETTEXT, lengthDoc, (LPARAM)buf);
  return buf;
}

void ClearAllUnderlines ()
{
  int length = SendMsgToEditor(SCI_GETLENGTH);
  if(length > 0){
    SendMsgToEditor (SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
    SendMsgToEditor (SCI_INDICATORCLEARRANGE, 1, length);
  }
}

AspellSpeller * spell_checker = 0;
char *Language = 0;

void set_string (char *&Target, const char *Str)
{
  CLEAN_AND_ZERO (Target);
  Target = new char[strlen (Str) + 1];
  strcpy (Target, Str);
}

void SetLanguage (const char *Str)
{
  set_string (Language, Str);
  WritePrivateProfileStringA ("SpellCheck", "Language", Language, iniFilePath);
  AspellReinitSettings ();
}

const char *GetLanguage ()
{
  return Language;
}

BOOL AspellReinitSettings ()
{
  AspellConfig *spell_config = new_aspell_config();
  aspell_config_replace (spell_config, "encoding", "utf-8");
  if (Language)
    aspell_config_replace(spell_config, "lang", Language);
  if (spell_checker)
  {
    delete_aspell_speller (spell_checker);
    spell_checker = 0;
  }

  AspellCanHaveError * possible_err = new_aspell_speller(spell_config);

  if (aspell_error_number(possible_err) != 0)
  {
    delete_aspell_config (spell_config);
    return FALSE;
  }
  else
    spell_checker = to_aspell_speller(possible_err);
  delete_aspell_config (spell_config);
  return TRUE;
}

BOOL AspellClear ()
{
  delete_aspell_speller (spell_checker);
  return TRUE;
}

BOOL CheckWord (char *word)
{
  if (ConvertingIsNeeded) // That's really part that sucks
  {
    // Well TODO: maybe convert only to unicode and tell aspell that we're using unicode
    // Cause double conversion kinda sucks
    int Length = strlen(word) + 1;
    WCHAR *WcharBuf = new WCHAR[strlen (word) + 1];
    int Utf8Length = Length * 4;
    char *Utf8Buf = new char[Utf8Length];
    MultiByteToWideChar(CP_ACP, 0, word, Length, WcharBuf, Length);
    WideCharToMultiByte (CP_UTF8, 0, WcharBuf, Length, Utf8Buf, Utf8Length, 0, 0);
    BOOL Ret = aspell_speller_check(spell_checker, Utf8Buf, strlen (Utf8Buf));
    CLEAN_AND_ZERO_ARR (WcharBuf);
    CLEAN_AND_ZERO_ARR (Utf8Buf);
    return Ret;
  }
  return aspell_speller_check(spell_checker, word, strlen (word));
}

void CheckDocument ()
{
  int oldid = SendMsgToEditor (SCI_GETINDICATORCURRENT);
  SendMsgToEditor (SCI_SETINDICATORCURRENT, SCE_SQUIGGLE_UNDERLINE_RED);
  char *text_to_check = GetDocumentText ();
  char *context = 0; // Temporary variable for strtok_s usage
  const char *delim = " \n\r\t,.!?-\"";
  char *token;
  ClearAllUnderlines ();
  token = strtok_s (text_to_check, delim, &context);
  while (token)
  {
    if (token && !CheckWord (token))
      CreateWordUnderline (token - text_to_check, token - text_to_check + strlen (token) - 1);
    token = strtok_s (NULL, delim, &context);
  }
  CLEAN_AND_ZERO_ARR (text_to_check);
  SendMsgToEditor (SCI_SETINDICATORCURRENT, oldid);
}

BOOL GetAutoCheckText ()
{
  return AutoCheckText;
}

void RecheckDocument ()
{
  int CodepageId = 0;
  CodepageId = (int) SendMsgToEditor (SCI_GETCODEPAGE, 0, 0);
  setEncodingById (CodepageId); // For now it just changes should we convert it to utf-8 or no
  if (AutoCheckText)
    CheckDocument ();
  else
    ClearAllUnderlines ();
}

void updateAutocheckStatus ()
{
  char Buf[DEFAULT_BUF_SIZE];
  itoa (AutoCheckText, Buf, DEFAULT_BUF_SIZE);
  WritePrivateProfileStringA ("SpellCheck", "Autocheck", Buf, iniFilePath);
  CheckMenuItem(GetMenu(nppData._nppHandle), funcItem[0]._cmdID, MF_BYCOMMAND | (AutoCheckText ? MF_CHECKED : MF_UNCHECKED));
  RecheckDocument ();
}

void SwitchAutoCheckText ()
{
  AutoCheckText = !AutoCheckText;
  updateAutocheckStatus ();
}

//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit)
{
  if (index >= nbFunc)
    return false;

  if (!pFunc)
    return false;

  lstrcpy(funcItem[index]._itemName, cmdName);
  funcItem[index]._pFunc = pFunc;
  funcItem[index]._init2Check = check0nInit;
  funcItem[index]._pShKey = sk;

  return true;
}

static DWORD WINAPI threadTextPlayer(void *text2display)
{
  // Open a new document
  ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

  // Get the current scintilla
  int which = -1;
  ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
  if (which == -1)
    return FALSE;

  HWND curScintilla = (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
  srand((unsigned int)time(NULL));
  int rangeMin = 0;
  int rangeMax = 250;
  for (size_t i = 0 ; i < strlen((char *)text2display) ; i++)
  {
    char charToShow[2] = {((char *)text2display)[i], '\0'};

    int ranNum = rangeMin + (int)((double)rand() / ((double)RAND_MAX + 1) * rangeMax);
    Sleep(ranNum + 30);

    ::SendMessage(curScintilla, SCI_APPENDTEXT, 1, (LPARAM)charToShow);
    ::SendMessage(curScintilla, SCI_GOTOPOS, ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0), 0);
  }

  return TRUE;
};

void WhatIsNpp()
{
  char *text2display = "Notepad++ is a free (as in \"free speech\" and also as in \"free beer\") source code editor and Notepad replacement that supports several languages.\n\
                       Running in the MS Windows environment, its use is governed by GPL License.\n\
                       \n\
                       Based on a powerful editing component Scintilla, Notepad++ is written in C++ and uses pure Win32 API and STL which ensures a higher execution speed and smaller program size.\n\
                       By optimizing as many routines as possible without losing user friendliness, Notepad++ is trying to reduce the world carbon dioxide emissions. When using less CPU power, the PC can throttle down and reduce power consumption, resulting in a greener environment.";
  HANDLE hThread = ::CreateThread(NULL, 0, threadTextPlayer, text2display, 0, NULL);
  ::CloseHandle(hThread);
}

void insertCurrentPath(int which)
{
  int msg = NPPM_GETFULLCURRENTPATH;
  if (which == FILE_NAME)
    msg = NPPM_GETFILENAME;
  else if (which == CURRENT_DIRECTORY)
    msg = NPPM_GETCURRENTDIRECTORY;

  int currentEdit;
  TCHAR path[MAX_PATH];

  // A message to Notepad++ to get a multibyte string (if ANSI mode) or a wide char string (if Unicode mode)
  ::SendMessage(nppData._nppHandle, msg, 0, (LPARAM)path);

  //
  ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
  HWND curScint = (currentEdit == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
#ifdef UNICODE
  int encoding = (int)::SendMessage(curScint, SCI_GETCODEPAGE, 0, 0);
  char pathA[MAX_PATH];
  WideCharToMultiByte(encoding, 0, path, -1, pathA, MAX_PATH, NULL, NULL);
  ::SendMessage(curScint, SCI_REPLACESEL, 0, (LPARAM)pathA);
#else
  ::SendMessage(curScint, SCI_REPLACESEL, 0, (LPARAM)path);
#endif
}

void insertCurrentFullPath()
{
  insertCurrentPath(FULL_CURRENT_PATH);
}
void insertCurrentFileName()
{
  insertCurrentPath(FILE_NAME);
}
void insertCurrentDirectory()
{
  insertCurrentPath(CURRENT_DIRECTORY);
}

const bool shortDate = true;
const bool longDate = false;

void insertShortDateTime()
{
  insertDateTime(shortDate);
}

void insertLongDateTime()
{
  insertDateTime(longDate);
}

void insertDateTime(bool format)
{
  TCHAR date[128];
  TCHAR time[128];
  TCHAR dateTime[256];

  SYSTEMTIME st;
  ::GetLocalTime(&st);
  ::GetDateFormat(LOCALE_USER_DEFAULT, (format == shortDate)?DATE_SHORTDATE:DATE_LONGDATE, &st, NULL, date, 128);
  ::GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, time, 128);

  wsprintf(dateTime, TEXT("%s %s"), time, date);

  int currentEdit;
  ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
  HWND curScint = (currentEdit == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
#ifdef UNICODE
  int encoding = (int)::SendMessage(curScint, SCI_GETCODEPAGE, 0, 0);
  char dateTimeA[MAX_PATH];
  WideCharToMultiByte(encoding, 0, dateTime, -1, dateTimeA, MAX_PATH, NULL, NULL);
  ::SendMessage(curScint, SCI_REPLACESEL, 0, (LPARAM)dateTimeA);
#else
  ::SendMessage(curScint, SCI_REPLACESEL, 0, (LPARAM)dateTime);
#endif
}

void insertHtmlCloseTag()
{
  doCloseTag = !doCloseTag;
  ::CheckMenuItem(::GetMenu(nppData._nppHandle), funcItem[5]._cmdID, MF_BYCOMMAND | (doCloseTag?MF_CHECKED:MF_UNCHECKED));
}

void getFileNamesDemo()
{
  int nbFile = (int)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, 0);
  TCHAR toto[10];
  ::MessageBox(nppData._nppHandle, generic_itoa(nbFile, toto, 10), TEXT("nb opened files"), MB_OK);

  TCHAR **fileNames = (TCHAR **)new TCHAR*[nbFile];
  for (int i = 0 ; i < nbFile ; i++)
  {
    fileNames[i] = new TCHAR[MAX_PATH];
  }

  if (::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMES, (WPARAM)fileNames, (LPARAM)nbFile))
  {
    for (int i = 0 ; i < nbFile ; i++)
      ::MessageBox(nppData._nppHandle, fileNames[i], TEXT(""), MB_OK);
  }

  for (int i = 0 ; i < nbFile ; i++)
  {
    delete [] fileNames[i];
  }
  delete [] fileNames;
}

void getSessionFileNamesDemo()
{
  const TCHAR *sessionFullPath = TEXT("c:\\test.session");
  int nbFile = (int)::SendMessage(nppData._nppHandle, NPPM_GETNBSESSIONFILES, 0, (LPARAM)sessionFullPath);

  if (!nbFile)
  {
    ::MessageBox(nppData._nppHandle, TEXT("Please modify \"sessionFullPath\" in \"NppInsertPlugin.cpp\" in order to point to a valide session file"), TEXT("Error :"), MB_OK);
    return;
  }
  TCHAR toto[10];
  ::MessageBox(nppData._nppHandle, generic_itoa(nbFile, toto, 10), TEXT("nb session files"), MB_OK);

  TCHAR **fileNames = (TCHAR **)new TCHAR*[nbFile];
  for (int i = 0 ; i < nbFile ; i++)
  {
    fileNames[i] = new TCHAR[MAX_PATH];
  }

  if (::SendMessage(nppData._nppHandle, NPPM_GETSESSIONFILES, (WPARAM)fileNames, (LPARAM)sessionFullPath))
  {
    for (int i = 0 ; i < nbFile ; i++)
      ::MessageBox(nppData._nppHandle, fileNames[i], TEXT("session file name :"), MB_OK);
  }

  for (int i = 0 ; i < nbFile ; i++)
  {
    delete [] fileNames[i];
  }
  delete [] fileNames;
}

void saveCurrentSessionDemo()
{
  TCHAR *sessionPath = (TCHAR *)::SendMessage(nppData._nppHandle, NPPM_SAVECURRENTSESSION, 0, 0);
  if (sessionPath)
    ::MessageBox(nppData._nppHandle, sessionPath, TEXT("Saved Session File :"), MB_OK);
}