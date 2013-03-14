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

#include <shlwapi.h>

#include "CommonFunctions.h"
#include "Plugin.h"
#include "MainDef.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;
extern bool doCloseTag;
HANDLE HModule;
int RecheckDelay;
std::vector<std::pair <long, long> > CheckQueue;
HANDLE Timer;

int GetRecheckDelay ()
{
  return RecheckDelay;
}

void SetRecheckDelay (int Value)
{
  if (Value < 0)
    Value = 0;

  if (Value > 20000)
    Value = 20000;

  SendEvent (EID_WRITE_SETTING);
  std::pair <TCHAR *, int> *x = new std::pair <TCHAR *, int>;
  x->first = 0;
  SetString (x->first, _T ("Recheck_Delay"));
  x->second = Value;
  PostMessageToMainThread (TM_SET_SETTING, 0, (LPARAM) x);
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
    DeleteTimerQueueTimer (0, Timer, 0);
    pluginCleanUp();
    break;

  case DLL_THREAD_ATTACH:
    break;

  case DLL_THREAD_DETACH:
    break;
  }

  return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
  nppData = notpadPlusData;
  commandMenuInit();
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
  _In_  PVOID lpParameter,
  _In_  BOOLEAN TimerOrWaitFired
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
  SendEvent (EID_RECHECK_VISIBLE);
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
  // DEBUG_CODE:
  char Buf[DEFAULT_BUF_SIZE];
  long CurPos = SendMsgToEditor(&nppData, SCI_GETCURRENTPOS);
  int Style = SendMsgToEditor(&nppData, SCI_GETSTYLEAT, CurPos);
  // _ultot (Style /* notifyCode->nmhdr.code */, Buf, 10);
  SendMsgToEditor (&nppData, SCI_PROPERTYNAMES, 0, (LPARAM) Buf);
  OutputDebugStringA (Buf);
  OutputDebugString (_T ("\n"));
  switch (notifyCode->nmhdr.code)
  {
  case NPPN_SHUTDOWN:
    {
      commandMenuCleanUp();
    }
    break;

  case NPPN_READY:
    {
      SendMsgToEditor(&nppData, SCI_INDICSETSTYLE ,SCE_SQUIGGLE_UNDERLINE_RED, INDIC_SQUIGGLE);
      SendMsgToEditor(&nppData, SCI_INDICSETFORE, SCE_SQUIGGLE_UNDERLINE_RED, 0x0000ff);
      InitClasses ();
      CheckQueue.clear ();
      CreateThreadResources ();
      LoadSettings ();
      SendEvent (EID_SET_SUGGESTIONS_BOX_TRANSPARENCY);
      CreateHooks ();
      CreateTimerQueueTimer (&Timer, 0, ExecuteQueue, NULL, INFINITE, INFINITE , 0);
    }
    break;

  case NPPN_BUFFERACTIVATED:
    {
      SendEvent (EID_CHECK_FILE_NAME);
      SendEvent (EID_HIDE_SUGGESTIONS_BOX);
      RecheckVisible ();
      break;
    }

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
      ChangeTimerQueueTimer (0, Timer, RecheckDelay, INFINITE);
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
