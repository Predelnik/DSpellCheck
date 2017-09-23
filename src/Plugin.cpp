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

#include "Plugin.h"
#include "menuCmdID.h"

//
// put the headers you need here
//

#include "aspell.h"
#include "CommonFunctions.h"
#include "Controls/CheckedList/CheckedList.h"
#include "MainDef.h"
#include "AboutDlg.h"
#include "DownloadDicsDlg.h"
#include "LangList.h"
#include "ProgressDlg.h"
#include "RemoveDictionariesDialog.h"
#include "SelectProxyDialog.h"
#include "SpellChecker.h"
#include "SuggestionsButton.h"

#include "StackWalker/StackWalker.h"
#include "utils/raii.h"

const wchar_t configFileName[] = L"DSpellCheck.ini";

#ifdef UNICODE
#define generic_itoa _itow
#else
#define generic_itoa itoa
#endif

FuncItem funcItem[nbFunc];
bool ResourcesInited = false;

FuncItem* get_funcItem() {
    // Well maybe we should lock mutex here
    return funcItem;
}

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;
wchar_t IniFilePath[MAX_PATH];
DWORD CustomGUIMessageIds[static_cast<int>(CustomGUIMessage::MAX)] = {0};
bool doCloseTag = false;
std::unique_ptr<SpellChecker> spellChecker;
std::unique_ptr<SettingsDlg> settingsDlg;
std::unique_ptr<SuggestionsButton> suggestionsButton;
std::unique_ptr<LangList> langListInstance;
std::unique_ptr<RemoveDictionariesDialog> removeDicsDlg;
std::unique_ptr<SelectProxyDialog> selectProxyDlg;
std::unique_ptr<ProgressDlg> progressDlg;
std::unique_ptr<DownloadDicsDlg> downloadDicsDlg;
std::unique_ptr<AboutDlg> aboutDlg;
HMENU LangsMenu;
int ContextMenuIdStart;
int LangsMenuIdStart = false;
bool UseAllocatedIds;

HANDLE hModule = nullptr;
HHOOK HMouseHook = nullptr;
// HHOOK HCmHook = NULL;

//
// Initialize your plug-in data here
// It will be called while plug-in loading
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    switch (wParam) {
    case WM_MOUSEMOVE:
        getSpellChecker()->InitSuggestionsBox();
        break;
    }
    return CallNextHookEx(HMouseHook, nCode, wParam, lParam);;
}

void SetContextMenuIdStart(int Id) { ContextMenuIdStart = Id; }

void SetLangsMenuIdStart(int Id) { LangsMenuIdStart = Id; }

void SetUseAllocatedIds(bool Value) { UseAllocatedIds = Value; }

int GetContextMenuIdStart() { return ContextMenuIdStart; }

int GetLangsMenuIdStart() { return LangsMenuIdStart; }

bool GetUseAllocatedIds() { return UseAllocatedIds; }

SpellChecker* getSpellChecker() { return spellChecker.get(); }


std::wstring GetDefaultHunspellPath() {
    std::wstring path = IniFilePath;
    return path.substr(0, path.rfind(L'\\')) + L"\\Hunspell";
}

void pluginInit(HANDLE hModuleArg) {
    hModule = hModuleArg;
    // Init it all dialog classes:
}

LangList* GetLangList() { return langListInstance.get(); }

RemoveDictionariesDialog* GetRemoveDics() { return removeDicsDlg.get(); }

SelectProxyDialog* GetSelectProxy() { return selectProxyDlg.get(); }

ProgressDlg* getProgress() { return progressDlg.get(); }

DownloadDicsDlg* GetDownloadDics() { return downloadDicsDlg.get(); }

HANDLE getHModule() { return hModule; }


class MyStackWalker : public StackWalker {
public:
    MyStackWalker() : StackWalker() {
    }

protected:
    virtual void OnOutput(LPCSTR szText) {
        FILE* fp = _wfopen(L"DSpellCheck_Debug.log", L"a");
        fprintf(fp, "%s", szText);
        fclose(fp);
        StackWalker::OnOutput(szText);
    }
};

int filter(unsigned int, struct _EXCEPTION_POINTERS* ep) {
    MyStackWalker sw;
    sw.ShowCallstack(GetCurrentThread(), ep->ContextRecord);
    return EXCEPTION_CONTINUE_SEARCH;
}

void CreateHooks() {
    HMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, nullptr, GetCurrentThreadId());
    // HCmHook = SetWindowsHookExW(WH_CALLWNDPROC, ContextMenuProc, 0,
    // GetCurrentThreadId());
}

void pluginCleanUp() {
    UnhookWindowsHookEx(HMouseHook);
}

void RegisterCustomMessages() {
    for (int i = 0; i < static_cast<int>(CustomGUIMessage::MAX); i++) {
        CustomGUIMessageIds[i] = RegisterWindowMessage(CustomGUIMesssagesNames[i]);
    }
}

DWORD GetCustomGUIMessageId(CustomGUIMessage MessageId) {
    return CustomGUIMessageIds[static_cast<int>(MessageId)];
}

void SwitchAutoCheckText() { getSpellChecker()->SwitchAutoCheck(); }

void GetSuggestions() {
    // SendEvent (EID_INITSUGGESTIONS);
}

void StartSettings() { settingsDlg->DoDialog(); }

void StartManual() {
    ShellExecute(nullptr, L"open",
                 L"https://github.com/Predelnik/DSpellCheck/wiki/Manual", nullptr,
                 nullptr, SW_SHOW);
}

void StartAboutDlg() { aboutDlg->DoDialog(); }

void StartLanguageList() { langListInstance->DoDialog(); }

void LoadSettings() { getSpellChecker()->LoadSettings(); }

void RecheckVisible() { getSpellChecker()->RecheckVisible(); }

void FindNextMistake() { getSpellChecker()->FindNextMistake(); }

void FindPrevMistake() { getSpellChecker()->FindPrevMistake(); }

void QuickLangChangeContext() {
    POINT Pos;
    GetCursorPos(&Pos);
    TrackPopupMenu(GetLangsSubMenu(), 0, Pos.x, Pos.y, 0, nppData._nppHandle, nullptr);
}

//
// Initialization of your plug-in commands
// You should fill your plug-ins commands here
void commandMenuInit() {
    //
    // Firstly we get the parameters from your plugin config file (if any)
    //

    // get path of plugin configuration
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH,
                  (LPARAM)IniFilePath);

    // if config path doesn't exist, we create it
    if (PathFileExists(IniFilePath) == false) {
        ::CreateDirectory(IniFilePath, nullptr);
    }

    // make your plugin config file full file path name
    PathAppend(IniFilePath, configFileName);

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate
    // the order of command
    //            wchar_t *commandName,             // the command name that you
    //            want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function
    //            (function pointer) associated with this command. The body should
    //            be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut
    //            to trigger this command
    //            bool check0nInit                // optional. Make this menu item
    //            be checked visually
    //            );
    {
        auto shKey = std::make_unique<ShortcutKey>();
        shKey->_isAlt = true;
        shKey->_isCtrl = false;
        shKey->_isShift = false;
        shKey->_key = 0x41 + 'a' - 'a';
        setNextCommand(TEXT("Spell Check Document Automatically"),
                       SwitchAutoCheckText, std::move(shKey), false);
    }
    {
        auto shKey = std::make_unique<ShortcutKey>();
        shKey->_isAlt = true;
        shKey->_isCtrl = false;
        shKey->_isShift = false;
        shKey->_key = 0x41 + 'n' - 'a';
        setNextCommand(TEXT("Find Next Misspelling"), FindNextMistake, std::move(shKey), false);
    }
    {
        auto shKey = std::make_unique<ShortcutKey>();
        shKey->_isAlt = true;
        shKey->_isCtrl = false;
        shKey->_isShift = false;
        shKey->_key = 0x41 + 'b' - 'a';
        setNextCommand(TEXT("Find Previous Misspelling"), FindPrevMistake, std::move(shKey),
                       false);
    }

    {
        auto shKey = std::make_unique<ShortcutKey>();
        shKey->_isAlt = true;
        shKey->_isCtrl = false;
        shKey->_isShift = false;
        shKey->_key = 0x41 + 'd' - 'a';
        setNextCommand(TEXT("Change Current Language"), QuickLangChangeContext, std::move(shKey),
                       false);
    }
    setNextCommand(TEXT("---"), nullptr, nullptr, false);

    setNextCommand(TEXT("Settings..."), StartSettings, nullptr, false);
    setNextCommand(TEXT("Online Manual"), StartManual, nullptr, false);
    setNextCommand(TEXT("About"), StartAboutDlg, nullptr, false);
}

void AddIcons() {
    static ToolbarIconsWrapper AutoCheckIcon{
        static_cast<HINSTANCE>(hModule), MAKEINTRESOURCE(IDB_AUTOCHECK2),
        IMAGE_BITMAP, 16, 16, LR_LOADMAP3DCOLORS
    };
    ::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON,
                  static_cast<WPARAM>(funcItem[0]._cmdID), reinterpret_cast<LPARAM>(AutoCheckIcon.get()));
}

void UpdateLangsMenu() {
    getSpellChecker()->DoPluginMenuInclusion();
}

static std::wstring getMenuItemText(HMENU menu, UINT index) {
    auto strLen = GetMenuString(menu, index, nullptr, 0, MF_BYPOSITION);
    std::vector<wchar_t> buf(strLen + 1);
    GetMenuString(menu, index, buf.data(), static_cast<int> (buf.size()), MF_BYPOSITION);
    return buf.data();
}

HMENU GetDSpellCheckMenu() {
    HMENU PluginsMenu =
        reinterpret_cast<HMENU>(SendMsgToNpp(&nppData, NPPM_GETMENUHANDLE, NPPPLUGINMENU));
    HMENU DSpellCheckMenu = nullptr;
    int Count = GetMenuItemCount(PluginsMenu);
    for (int i = 0; i < Count; i++) {
        auto str = getMenuItemText(PluginsMenu, i);
        if (str == NPP_PLUGIN_NAME) {
            MENUITEMINFO Mif;
            Mif.fMask = MIIM_SUBMENU;
            Mif.cbSize = sizeof(MENUITEMINFO);
            bool Res = GetMenuItemInfo(PluginsMenu, i, true, &Mif);

            if (Res)
                DSpellCheckMenu = static_cast<HMENU>(Mif.hSubMenu);
            break;
        }
    }
    return DSpellCheckMenu;
}

HMENU GetLangsSubMenu(HMENU DSpellCheckMenuArg) {
    HMENU DSpellCheckMenu;
    if (!DSpellCheckMenuArg)
        DSpellCheckMenu = GetDSpellCheckMenu();
    else
        DSpellCheckMenu = DSpellCheckMenuArg;
    if (!DSpellCheckMenu)
        return nullptr;

    MENUITEMINFO Mif;

    Mif.fMask = MIIM_SUBMENU;
    Mif.cbSize = sizeof(MENUITEMINFO);

    bool Res =
        GetMenuItemInfo(DSpellCheckMenu, QUICK_LANG_CHANGE_ITEM, true, &Mif);
    if (!Res)
        return nullptr;

    return Mif.hSubMenu; // TODO: CHECK IS THIS CORRECT FIX
}

void InitClasses() {
    INITCOMMONCONTROLSEX icc;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    InitCheckedListBox(static_cast<HINSTANCE>(hModule));

    suggestionsButton = std::make_unique<SuggestionsButton>();
    suggestionsButton->initDlg(static_cast<HINSTANCE>(hModule), nppData._nppHandle, nppData);
    suggestionsButton->DoDialog();

    settingsDlg = std::make_unique<SettingsDlg>();
    settingsDlg->initSettings(static_cast<HINSTANCE>(hModule), nppData._nppHandle, nppData);

    aboutDlg = std::make_unique<AboutDlg>();
    aboutDlg->init(static_cast<HINSTANCE>(hModule), nppData._nppHandle);

    progressDlg = std::make_unique<ProgressDlg>();
    progressDlg->init(static_cast<HINSTANCE>(hModule), nppData._nppHandle);

    langListInstance = std::make_unique<LangList>();
    langListInstance->init(static_cast<HINSTANCE>(hModule), nppData._nppHandle);

    selectProxyDlg = std::make_unique<SelectProxyDialog>();
    selectProxyDlg->init(static_cast<HINSTANCE>(hModule), nppData._nppHandle);

    removeDicsDlg = std::make_unique<RemoveDictionariesDialog>();
    removeDicsDlg->init(static_cast<HINSTANCE>(hModule), nppData._nppHandle);

    spellChecker =
        std::make_unique<SpellChecker>(IniFilePath, settingsDlg.get(), &nppData,
                                       suggestionsButton.get(), langListInstance.get());

    downloadDicsDlg = std::make_unique<DownloadDicsDlg>();
    downloadDicsDlg->initDlg(static_cast<HINSTANCE>(hModule), nppData._nppHandle,
                             spellChecker.get());

    ResourcesInited = true;
}

void commandMenuCleanUp() {
}

//
// Function that initializes plug-in commands
//
static int counter = 0;

std::vector<std::unique_ptr<ShortcutKey>> shortcutStorage;

bool setNextCommand(const wchar_t* cmdName, PFUNCPLUGINCMD pFunc, std::unique_ptr<ShortcutKey> sk,
                    bool check0nInit) {
    if (counter >= nbFunc)
        return false;

    if (!pFunc) {
        counter++;
        return false;
    }

    lstrcpy(funcItem[counter]._itemName, cmdName);
    funcItem[counter]._pFunc = pFunc;
    funcItem[counter]._init2Check = check0nInit;
    shortcutStorage.push_back(std::move(sk));
    funcItem[counter]._pShKey = shortcutStorage.back().get();
    counter++;

    return true;
}
