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

//
// put the headers you need here
//

#include "CommonFunctions.h"
#include "CheckedList/CheckedList.h"
#include "MainDef.h"
#include "AboutDlg.h"
#include "DownloadDicsDlg.h"
#include "LangList.h"
#include "ProgressDlg.h"
#include "RemoveDictionariesDialog.h"
#include "SelectProxyDialog.h"
#include "SpellChecker.h"
#include "SuggestionsButton.h"

#include "StackWalker.h"
#include "utils/raii.h"
#include "resource.h"

const wchar_t config_file_name[] = L"DSpellCheck.ini";


FuncItem func_item[nb_func];
bool resources_inited = false;

FuncItem* get_func_item() {
    // Well maybe we should lock mutex here
    return func_item;
}

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData npp_data;
wchar_t ini_file_path[MAX_PATH];
DWORD custom_gui_message_ids[static_cast<int>(CustomGuiMessage::max)] = {0};
bool do_close_tag = false;
std::unique_ptr<SpellChecker> spell_checker;
std::unique_ptr<SettingsDlg> settings_dlg;
std::unique_ptr<SuggestionsButton> suggestions_button;
std::unique_ptr<LangList> lang_list_instance;
std::unique_ptr<RemoveDictionariesDialog> remove_dics_dlg;
std::unique_ptr<SelectProxyDialog> select_proxy_dlg;
std::unique_ptr<ProgressDlg> progress_dlg;
std::unique_ptr<DownloadDicsDlg> download_dics_dlg;
std::unique_ptr<AboutDlg> about_dlg;
HMENU langs_menu;
int context_menu_id_start;
int langs_menu_id_start = false;
bool use_allocated_ids;

HANDLE h_module = nullptr;
HHOOK h_mouse_hook = nullptr;

//
// Initialize your plug-in data here
// It will be called while plug-in loading
LRESULT CALLBACK mouse_proc(int n_code, WPARAM w_param, LPARAM l_param) {
    switch (w_param) {
    case WM_MOUSEMOVE:
        get_spell_checker()->init_suggestions_box();
        break;
    }
    return CallNextHookEx(h_mouse_hook, n_code, w_param, l_param);;
}

void set_context_menu_id_start(int id) { context_menu_id_start = id; }

void set_langs_menu_id_start(int id) { langs_menu_id_start = id; }

void set_use_allocated_ids(bool value) { use_allocated_ids = value; }

int get_context_menu_id_start() { return context_menu_id_start; }

int get_langs_menu_id_start() { return langs_menu_id_start; }

bool get_use_allocated_ids() { return use_allocated_ids; }

SpellChecker* get_spell_checker() { return spell_checker.get(); }


std::wstring get_default_hunspell_path() {
    std::wstring path = ini_file_path;
    return path.substr(0, path.rfind(L'\\')) + L"\\Hunspell";
}

void plugin_init(HANDLE h_module_arg) {
    h_module = h_module_arg;
    // Init it all dialog classes:
}

LangList* get_lang_list() { return lang_list_instance.get(); }

RemoveDictionariesDialog* get_remove_dics() { return remove_dics_dlg.get(); }

SelectProxyDialog* get_select_proxy() { return select_proxy_dlg.get(); }

ProgressDlg* get_progress() { return progress_dlg.get(); }

DownloadDicsDlg* get_download_dics() { return download_dics_dlg.get(); }

HANDLE get_h_module() { return h_module; }


class MyStackWalker : public StackWalker {
public:
    MyStackWalker() : StackWalker() {
    }

protected:
    void OnOutput(LPCSTR sz_text) override {
        FILE* fp = _wfopen(L"DSpellCheck_Debug.log", L"a");
        fprintf(fp, "%s", sz_text);
        fclose(fp);
        StackWalker::OnOutput(sz_text);
    }
};

int filter(unsigned int, struct _EXCEPTION_POINTERS* ep) {
    MyStackWalker sw;
    sw.ShowCallstack(GetCurrentThread(), ep->ContextRecord);
    return EXCEPTION_CONTINUE_SEARCH;
}

void create_hooks() {
    h_mouse_hook = SetWindowsHookEx(WH_MOUSE, mouse_proc, nullptr, GetCurrentThreadId());
    // HCmHook = SetWindowsHookExW(WH_CALLWNDPROC, ContextMenuProc, 0,
    // GetCurrentThreadId());
}

void plugin_clean_up() {
    UnhookWindowsHookEx(h_mouse_hook);
}

void register_custom_messages() {
    for (int i = 0; i < static_cast<int>(CustomGuiMessage::max); i++) {
        custom_gui_message_ids[i] = RegisterWindowMessage(custom_gui_messsages_names[i]);
    }
}

DWORD get_custom_gui_message_id(CustomGuiMessage message_id) {
    return custom_gui_message_ids[static_cast<int>(message_id)];
}

void switch_auto_check_text() { get_spell_checker()->switch_auto_check(); }

void get_suggestions() {
    // SendEvent (EID_INITSUGGESTIONS);
}

void start_settings() { settings_dlg->do_dialog(); }

void start_manual() {
    ShellExecute(nullptr, L"open",
                 L"https://github.com/Predelnik/DSpellCheck/wiki/Manual", nullptr,
                 nullptr, SW_SHOW);
}

void start_about_dlg() { about_dlg->do_dialog(); }

void start_language_list() { lang_list_instance->do_dialog(); }

void load_settings() { get_spell_checker()->on_load_settings(); }

void recheck_visible() { get_spell_checker()->recheck_visible(); }

void find_next_mistake() { get_spell_checker()->find_next_mistake(); }

void find_prev_mistake() { get_spell_checker()->find_prev_mistake(); }

void quick_lang_change_context() {
    POINT pos;
    GetCursorPos(&pos);
    TrackPopupMenu(get_langs_sub_menu(), 0, pos.x, pos.y, 0, npp_data.npp_handle, nullptr);
}

//
// Initialization of your plug-in commands
// You should fill your plug-ins commands here
void command_menu_init() {
    //
    // Firstly we get the parameters from your plugin config file (if any)
    //

    // get path of plugin configuration
    ::SendMessage(npp_data.npp_handle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH,
                  (LPARAM)ini_file_path);

    // if config path doesn't exist, we create it
    if (PathFileExists(ini_file_path) == false) {
        ::CreateDirectory(ini_file_path, nullptr);
    }

    // make your plugin config file full file path name
    PathAppend(ini_file_path, config_file_name);

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
        auto sh_key = std::make_unique<ShortcutKey>();
        sh_key->is_alt = true;
        sh_key->is_ctrl = false;
        sh_key->is_shift = false;
        sh_key->key = 0x41 + 'a' - 'a';
        set_next_command(TEXT("Spell Check Document Automatically"),
                       switch_auto_check_text, std::move(sh_key), false);
    }
    {
        auto sh_key = std::make_unique<ShortcutKey>();
        sh_key->is_alt = true;
        sh_key->is_ctrl = false;
        sh_key->is_shift = false;
        sh_key->key = 0x41 + 'n' - 'a';
        set_next_command(TEXT("Find Next Misspelling"), find_next_mistake, std::move(sh_key), false);
    }
    {
        auto sh_key = std::make_unique<ShortcutKey>();
        sh_key->is_alt = true;
        sh_key->is_ctrl = false;
        sh_key->is_shift = false;
        sh_key->key = 0x41 + 'b' - 'a';
        set_next_command(TEXT("Find Previous Misspelling"), find_prev_mistake, std::move(sh_key),
                       false);
    }

    {
        auto sh_key = std::make_unique<ShortcutKey>();
        sh_key->is_alt = true;
        sh_key->is_ctrl = false;
        sh_key->is_shift = false;
        sh_key->key = 0x41 + 'd' - 'a';
        set_next_command(TEXT("Change Current Language"), quick_lang_change_context, std::move(sh_key),
                       false);
    }
    set_next_command(TEXT("---"), nullptr, nullptr, false);

    set_next_command(TEXT("Settings..."), start_settings, nullptr, false);
    set_next_command(TEXT("Online Manual"), start_manual, nullptr, false);
    set_next_command(TEXT("About"), start_about_dlg, nullptr, false);
}

void add_icons() {
    static ToolbarIconsWrapper auto_check_icon{
        static_cast<HINSTANCE>(h_module), MAKEINTRESOURCE(IDB_AUTOCHECK2),
        IMAGE_BITMAP, 16, 16, LR_LOADMAP3DCOLORS
    };
    ::SendMessage(npp_data.npp_handle, NPPM_ADDTOOLBARICON,
                  static_cast<WPARAM>(func_item[0].cmd_id), reinterpret_cast<LPARAM>(auto_check_icon.get()));
}

void update_langs_menu() {
    get_spell_checker()->do_plugin_menu_inclusion();
}

static std::wstring get_menu_item_text(HMENU menu, UINT index) {
    auto str_len = GetMenuString(menu, index, nullptr, 0, MF_BYPOSITION);
    std::vector<wchar_t> buf(str_len + 1);
    GetMenuString(menu, index, buf.data(), static_cast<int> (buf.size()), MF_BYPOSITION);
    return buf.data();
}

HMENU get_dspellcheck_menu() {
    HMENU plugins_menu =
        reinterpret_cast<HMENU>(send_msg_to_npp(&npp_data, NPPM_GETMENUHANDLE, NPPPLUGINMENU));
    HMENU dspellcheck_menu = nullptr;
    int count = GetMenuItemCount(plugins_menu);
    for (int i = 0; i < count; i++) {
        auto str = get_menu_item_text(plugins_menu, i);
        if (str == npp_plugin_name) {
            MENUITEMINFO mif;
            mif.fMask = MIIM_SUBMENU;
            mif.cbSize = sizeof(MENUITEMINFO);
            bool res = GetMenuItemInfo(plugins_menu, i, true, &mif);

            if (res)
                dspellcheck_menu = static_cast<HMENU>(mif.hSubMenu);
            break;
        }
    }
    return dspellcheck_menu;
}

HMENU get_langs_sub_menu(HMENU dspellcheck_menu_arg) {
    HMENU dspellcheck_menu;
    if (!dspellcheck_menu_arg)
        dspellcheck_menu = get_dspellcheck_menu();
    else
        dspellcheck_menu = dspellcheck_menu_arg;
    if (!dspellcheck_menu)
        return nullptr;

    MENUITEMINFO mif;

    mif.fMask = MIIM_SUBMENU;
    mif.cbSize = sizeof(MENUITEMINFO);

    bool res =
        GetMenuItemInfo(dspellcheck_menu, QUICK_LANG_CHANGE_ITEM, true, &mif);
    if (!res)
        return nullptr;

    return mif.hSubMenu; // TODO: CHECK IS THIS CORRECT FIX
}

void init_classes() {
    INITCOMMONCONTROLSEX icc;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    InitCheckedListBox(static_cast<HINSTANCE>(h_module));

    suggestions_button = std::make_unique<SuggestionsButton>();
    suggestions_button->init_dlg(static_cast<HINSTANCE>(h_module), npp_data.npp_handle, npp_data);
    suggestions_button->do_dialog();

    settings_dlg = std::make_unique<SettingsDlg>();
    settings_dlg->init_settings(static_cast<HINSTANCE>(h_module), npp_data.npp_handle, npp_data);

    about_dlg = std::make_unique<AboutDlg>();
    about_dlg->init(static_cast<HINSTANCE>(h_module), npp_data.npp_handle);

    progress_dlg = std::make_unique<ProgressDlg>();
    progress_dlg->init(static_cast<HINSTANCE>(h_module), npp_data.npp_handle);

    lang_list_instance = std::make_unique<LangList>();
    lang_list_instance->init(static_cast<HINSTANCE>(h_module), npp_data.npp_handle);

    select_proxy_dlg = std::make_unique<SelectProxyDialog>();
    select_proxy_dlg->init(static_cast<HINSTANCE>(h_module), npp_data.npp_handle);

    remove_dics_dlg = std::make_unique<RemoveDictionariesDialog>();
    remove_dics_dlg->init(static_cast<HINSTANCE>(h_module), npp_data.npp_handle);

    spell_checker =
        std::make_unique<SpellChecker>(ini_file_path, settings_dlg.get(), &npp_data,
                                       suggestions_button.get(), lang_list_instance.get());

    download_dics_dlg = std::make_unique<DownloadDicsDlg>();
    download_dics_dlg->init_dlg(static_cast<HINSTANCE>(h_module), npp_data.npp_handle,
                             spell_checker.get());

    resources_inited = true;
}

void command_menu_clean_up() {
}

//
// Function that initializes plug-in commands
//
static int counter = 0;

std::vector<std::unique_ptr<ShortcutKey>> shortcut_storage;

bool set_next_command(const wchar_t* cmd_name, Pfuncplugincmd p_func, std::unique_ptr<ShortcutKey> sk,
                    bool check0_n_init) {
    if (counter >= nb_func)
        return false;

    if (!p_func) {
        counter++;
        return false;
    }

    lstrcpy(func_item[counter].item_name, cmd_name);
    func_item[counter].p_func = p_func;
    func_item[counter].init2_check = check0_n_init;
    shortcut_storage.push_back(std::move(sk));
    func_item[counter].p_sh_key = shortcut_storage.back().get();
    counter++;

    return true;
}
