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
  InitClasses ();
  CreateThreadResources ();
  LoadSettings ();
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

extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
  // DEBUG_CODE:
  TCHAR Buf[DEFAULT_BUF_SIZE];
  _ultot (notifyCode->nmhdr.code, Buf, 10);
  OutputDebugString (Buf);
  OutputDebugString (_T("\n"));

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
      SendEvent (EID_SET_SUGGESTIONS_BOX_TRANSPARENCY);
      CreateHooks ();
    }
    break;

  case NPPN_BUFFERACTIVATED:
    {
      RecheckVisible ();
      break;
    }

  case SCN_UPDATEUI:
    SendEvent (EID_HIDE_SUGGESTIONS_BOX);
    if(notifyCode->updated & (SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL))
      RecheckVisible ();
    break;

  case SCN_MODIFIED:
    if(notifyCode->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
      RecheckVisible ();
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
  }
  return FALSE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
  return TRUE;
}
#endif //UNICODE
