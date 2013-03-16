#ifdef _DEBUG
#include <stdlib.h>
#include <crtdbg.h>
#ifndef DEBUG_NEW
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#endif

#include <windows.h>

#include <string>
#include <string.h>
#include <cstring>
#include <time.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <wchar.h>
#include <stringapiset.h>
#include <vector>
#include <tchar.h>
