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

#include <shlwapi.h>

#include "CommonFunctions.h"
#include "Plugin.h"
#include "MainDef.h"
#include "SpellChecker.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;
extern bool doCloseTag;
HANDLE HModule;
int RecheckDelay;
std::vector<std::pair <long, long> > CheckQueue;
HANDLE Timer;
WNDPROC wndProcNotepad = NULL;

int GetRecheckDelay ()
{
  return RecheckDelay;
}

void SetRecheckDelay (int Value, int WriteToIni)
{
  if (Value < 0)
    Value = 0;

  if (Value > 20000)
    Value = 20000;

  if (WriteToIni)
  {
    std::pair <TCHAR *, DWORD> *x = new std::pair <TCHAR *, DWORD>;
    x->first = 0;
    SetString (x->first, _T ("Recheck_Delay"));
    x->second = MAKELPARAM (Value, 500);
    PostMessageToMainThread (TM_WRITE_SETTING, 0, (LPARAM) x);
  }
  RecheckDelay = Value;
}

BOOL APIENTRY DllMain( HANDLE hModule,
                      DWORD  reasonForCall,
                      LPVOID lpReserved )
{
  HModule = hModule;

  switch (reasonForCall)
  {
  case DLL_PROCESS_ATTACH:
    pluginInit(hModule);
    break;

  case DLL_PROCESS_DETACH:
    pluginCleanUp();
    _CrtDumpMemoryLeaks();
    if (wndProcNotepad != NULL)
      ::SetWindowLongPtr(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad); // Removing subclassing
    break;

  case DLL_THREAD_ATTACH:
    break;

  case DLL_THREAD_DETACH:
    break;
  }

  return TRUE;
}
WPARAM LastHwnd  = NULL;
LPARAM LastCoords = 0;
std::vector <SuggestionsMenuItem*> *MenuList = NULL;
// Ok, trying to use window subclassing to handle messages
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  int ret;
  switch (Message)
  {
  case WM_INITMENUPOPUP:
    {
      OutputDebugString (_T ("TM_CONTEXT_MENU sent\n"));
      if (MenuList)
      {
        HMENU Menu = (HMENU) wParam;
        for (unsigned int i = 0; i < MenuList->size (); i++)
        {
          InsertSuggMenuItem (Menu, (*MenuList)[i]->Text, (*MenuList)[i]->Id, i, (*MenuList)[i]->Separator);
        }
      }
    }
    break;
  case WM_COMMAND:
    if (HIWORD (wParam) == 0)
    {
      OutputDebugString (_T ("TM_MENU_RESULT sent\n"));
      PostMessageToMainThread (TM_MENU_RESULT, wParam, 0);
    }
    break;
  case WM_CONTEXTMENU:
    if (wParam)
    {
      LastHwnd = wParam;
      LastCoords = lParam;
      PostMessageToMainThread (TM_PRECALCULATE_MENU, wParam, lParam);
      return TRUE;
    }
    else
    {
      MenuList = (std::vector <SuggestionsMenuItem*> *) lParam;
      wParam = LastHwnd;
      lParam = LastCoords;
    }
    break;
  case WM_EXITMENULOOP:
    {
      if (MenuList)
      {
        for (unsigned int i = 0; i < MenuList->size (); i++)
          CLEAN_AND_ZERO ((*MenuList)[i]);

        CLEAN_AND_ZERO (MenuList);
      }
    }
    break;
  }
  /*
  TCHAR Buf[DEFAULT_BUF_SIZE];
  _itot (Message, Buf, 16);
  _tcscat (Buf, _T ("\n"));
  OutputDebugString (Buf);
  */
  ret = ::CallWindowProc(wndProcNotepad, hWnd, Message, wParam, lParam);
  return ret;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
  nppData = notpadPlusData;
  commandMenuInit();
  wndProcNotepad = (WNDPROC)::SetWindowLongPtr(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
  return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
  *nbF = nbFunc;
  return funcItem;
}

static void AddToQueue (long Start, long End)
{
  std::pair <long, long> *Pair = new std::pair <long, long>;
  Pair->first = Start;
  Pair->second = End;
  CheckQueue.push_back (*Pair);
}

// For now doesn't look like there is such a need in check modified, but code stays until thorough testing
VOID CALLBACK ExecuteQueue (
  PVOID lpParameter,
  BOOLEAN TimerOrWaitFired
  )
{
  /*
  std::vector<std::pair <long, long>>::iterator Iterator;
  for (Iterator = CheckQueue.begin (); Iterator != CheckQueue.end (); ++Iterator)
  {
  std::pair <long, long> *Pair = new std::pair <long, long> (*Iterator);
  SendEvent (EID_RECHECK_MODIFIED_ZONE);
  PostMessageToMainThread (TM_MODIFIED_ZONE_INFO, 0, (LPARAM) Pair);
  }
  CheckQueue.clear ();
  */
  DeleteTimerQueueTimer (0, Timer, 0);
  Timer = 0;
  SendEvent (EID_RECHECK_VISIBLE);
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
  // DEBUG_CODE:
  long CurPos = SendMsgToEditor(&nppData, SCI_GETCURRENTPOS);
  int Style = SendMsgToEditor(&nppData, SCI_GETSTYLEAT, CurPos);
  switch (notifyCode->nmhdr.code)
  {
  case NPPN_SHUTDOWN:
    {
      SendEvent (EID_KILLTHREAD);
      commandMenuCleanUp();
      DeleteTimerQueueTimer (0, Timer, 0);
    }
    break;

  case NPPN_READY:
    {
      InitClasses ();
      CheckQueue.clear ();
      CreateThreadResources ();
      LoadSettings ();
      SendEvent (EID_CHECK_FILE_NAME);
      CreateHooks ();
      CreateTimerQueueTimer (&Timer, 0, ExecuteQueue, NULL, INFINITE, INFINITE , 0);
      RecheckVisible ();
      SendEvent (EID_SET_SUGGESTIONS_BOX_TRANSPARENCY);
    }
    break;

  case NPPN_BUFFERACTIVATED:
    {
      SendEvent (EID_CHECK_FILE_NAME);
      //SendEvent (EID_HIDE_SUGGESTIONS_BOX);
      RecheckVisible ();
    }
    break;

  case SCN_UPDATEUI:
    if(notifyCode->updated & (SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL))
    {
      RecheckVisible ();
    }
    SendEvent (EID_HIDE_SUGGESTIONS_BOX);
    break;

  case SCN_MODIFIED:
    if(notifyCode->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
    {
      // SendEvent (EID_HIDE_SUGGESTIONS_BOX);
      long Start = 0, End = 0;
      if (notifyCode->modificationType & SC_MOD_DELETETEXT)
      {
        Start = notifyCode->position;
        End = notifyCode->position;
      }
      else
      {
        Start = notifyCode->position;
        End = notifyCode->position + notifyCode->length - 1;
      }

      // AddToQueue (Start, End);
      if (Timer)
        ChangeTimerQueueTimer (0, Timer, RecheckDelay, 0);
      else
        CreateTimerQueueTimer (&Timer, 0, ExecuteQueue, NULL, RecheckDelay, 0 , 0);
    }
    break;

  case NPPN_LANGCHANGED:
    {
      SendEvent (EID_RECHECK_VISIBLE);
    }
    break;

  default:
    return;
  }
}

// Here you can process the Npp Messages
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
  switch (Message)
  {
  case WM_MOVE:
    SendEvent (EID_HIDE_SUGGESTIONS_BOX);
    return FALSE;
  }
  return FALSE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
  return TRUE;
}
#endif //UNICODE