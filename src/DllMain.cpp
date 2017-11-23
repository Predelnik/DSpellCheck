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

#include "CommonFunctions.h"
#include "DownloadDicsDlg.h"
#include "LangList.h"
#include "Plugin.h"
#include "RemoveDictionariesDialog.h"
#include "MainDef.h"
#include "SpellChecker.h"
#include "menuCmdID.h"
#include "Settings.h"
#include "npp/NppInterface.h"

#ifdef VLD_BUILD
#include <vld.h>
#endif // VLD_BUILD

extern FuncItem func_item[nb_func]; // NOLINT
extern NppData npp_data; // NOLINT
extern bool do_close_tag; // NOLINT
std::vector<std::pair<long, long>> check_queue;
UINT_PTR ui_timer = 0u;
UINT_PTR recheck_timer = 0u;
bool recheck_done = true;
WNDPROC wnd_proc_notepad = nullptr;
bool restyling_caused_recheck_was_done =
    false; // Hack to avoid eternal cycle in case of scintilla bug
bool first_restyle = true; // hack to successfully avoid checking hyperlinks when they appear on program start

// ReSharper disable once CppInconsistentNaming
bool APIENTRY DllMain(HANDLE h_module, DWORD reason_for_call, LPVOID)
{
    switch (reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        plugin_init(h_module);
        break;

    case DLL_PROCESS_DETACH:
        //_CrtDumpMemoryLeaks();
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return true;
}

WPARAM last_hwnd = NULL;
LPARAM last_coords = 0;
std::vector<SuggestionsMenuItem> cur_menu_list;
// Ok, trying to use window subclassing to handle messages
LRESULT CALLBACK sub_wnd_proc_notepad(HWND h_wnd, UINT message, WPARAM w_param,
                                      LPARAM l_param)
{
    LRESULT ret; // int->LRESULT, fix x64 issue, still compatible with x86
    switch (message)
    {
    case WM_INITMENUPOPUP:
        {
            // Checking that it isn't system menu and nor any main menu except 0th
            if (!cur_menu_list.empty() && LOWORD(l_param) == 0 && HIWORD(l_param) == 0)
            {
                // Special check for 0th main menu item
                MENUBARINFO info;
                info.cbSize = sizeof(MENUBARINFO);
                GetMenuBarInfo(npp_data.npp_handle, OBJID_MENU, 0, &info);
                HMENU main_menu = info.hMenu;
                MENUITEMINFO file_menu_item;
                file_menu_item.cbSize = sizeof(MENUITEMINFO);
                file_menu_item.fMask = MIIM_SUBMENU;
                GetMenuItemInfo(main_menu, 0, true, &file_menu_item);

                const auto menu = reinterpret_cast<HMENU>(w_param);
                if (file_menu_item.hSubMenu != menu && get_langs_sub_menu() != menu)
                {
                    int i = 0;
                    for (auto& item : cur_menu_list)
                    {
                        insert_sugg_menu_item(menu, item.text.c_str(), item.id, i, item.separator);
                        ++i;
                    }
                }
            }
            cur_menu_list.clear();
        }
        break;

    case WM_COMMAND:
        if (HIWORD(w_param) == 0)
        {
            if (!get_use_allocated_ids())
                get_spell_checker()->process_menu_result(w_param);

            if (LOWORD(w_param) == IDM_FILE_PRINTNOW ||
                LOWORD(w_param) == IDM_FILE_PRINT)
            {
                // Disable autocheck while printing
                bool prev_value = get_settings().auto_check_text;

                auto mut_settings = get_settings().modify();
                mut_settings->auto_check_text = false;
                ret = ::CallWindowProc(wnd_proc_notepad, h_wnd, message, w_param, l_param);
                mut_settings->auto_check_text = prev_value;

                return ret;
            }
        }
        break;
    case WM_NOTIFY:
        // Removing possibility of adding items to tab bar menu.
        if (((LPNMHDR)l_param)->code == NM_RCLICK)
        {
            cur_menu_list.clear();
        }
        break;
    case WM_CONTEXTMENU:
        last_hwnd = w_param;
        last_coords = l_param;
        get_spell_checker()->precalculate_menu();
        return true;
    case WM_DISPLAYCHANGE:
        {
            get_spell_checker()->hide_suggestion_box();
        }
        break;
    }

    if (message != 0)
    {
        if (message == get_custom_gui_message_id(CustomGuiMessage::generic_callback))
        {
            const auto callback_ptr = std::unique_ptr<CallbackData>(reinterpret_cast<CallbackData *>(w_param));
            if (callback_ptr->alive_status.expired())
                return false;

            callback_ptr->callback();
            return true;
        }
    }
    ret = ::CallWindowProc(wnd_proc_notepad, h_wnd, message, w_param, l_param);
    return ret;
}

LRESULT show_calculated_menu(const std::vector<SuggestionsMenuItem>&& menu_list)
{
    cur_menu_list = std::move(menu_list);
    return ::CallWindowProc(wnd_proc_notepad, npp_data.npp_handle, WM_CONTEXTMENU, last_hwnd, last_coords);
}

// ReSharper disable CppInconsistentNaming
extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
    npp_data = notpadPlusData;
    init_npp_interface ();
    command_menu_init();
    wnd_proc_notepad = reinterpret_cast<WNDPROC>(::SetWindowLongPtr(npp_data.npp_handle, GWLP_WNDPROC,
                                                                    reinterpret_cast<LPARAM>(sub_wnd_proc_notepad)));
}

extern "C" __declspec(dllexport) const wchar_t* getName()
{
    return npp_plugin_name;
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF)
{
    *nbF = nb_func;
    return func_item;
}

// ReSharper restore CppInconsistentNaming

// For now doesn't look like there is such a need in check modified, but code
// stays until thorough testing
void WINAPI do_recheck(HWND, UINT, UINT_PTR, DWORD)
{
    if (recheck_timer)
        SetTimer(npp_data.npp_handle, recheck_timer, USER_TIMER_MAXIMUM, do_recheck);
    recheck_done = true;

    get_spell_checker()->recheck_visible(npp_interface().active_view());
    if (!first_restyle)
        restyling_caused_recheck_was_done = true;
    first_restyle = false;
}

// (PVOID /*lpParameter*/, BOOLEAN /*TimerOrWaitFired*/)
void WINAPI ui_update(HWND, UINT, UINT_PTR, DWORD)
{
    get_download_dics()->ui_update();
}

// ReSharper disable once CppInconsistentNaming
extern "C" __declspec(dllexport) void beNotified(SCNotification* notify_code)
{
    notify (notify_code);
    switch (notify_code->nmhdr.code)
    {
    case NPPN_SHUTDOWN:
        {
            MSG msg;
            while (PeekMessage(&msg, nullptr, WM_USER, 0xFFFF,
                               PM_REMOVE)) // Clearing message queue to make sure that
                // we're not stuck in SendMesage somewhere
            {
            };

            if (recheck_timer) KillTimer(nullptr, recheck_timer);
            if (ui_timer) KillTimer(nullptr, ui_timer);
            command_menu_clean_up();

            plugin_clean_up();
            if (wnd_proc_notepad != nullptr)
                // LONG_PTR is more x64 friendly, yet not affecting x86
                ::SetWindowLongPtr(npp_data.npp_handle, GWLP_WNDPROC,
                                   (LONG_PTR)wnd_proc_notepad); // Removing subclassing
        }
        break;

    case NPPN_READY:
        {
            register_custom_messages();
            init_classes();
            check_queue.clear();
            load_settings();
            get_spell_checker()->check_file_name();
            create_hooks();
            recheck_timer = SetTimer(npp_data.npp_handle, 0, USER_TIMER_MAXIMUM, do_recheck);
            ui_timer = SetTimer(npp_data.npp_handle, 0, 100, ui_update);
            get_spell_checker()->recheck_visible_both_views();
            restyling_caused_recheck_was_done = false;
            get_spell_checker()->set_suggestions_box_transparency();
        }
        break;

    case NPPN_BUFFERACTIVATED:
        {
            if (get_spell_checker())
            {
                get_spell_checker()->check_file_name();
                recheck_visible();
                restyling_caused_recheck_was_done = false;
            }
        }
        break;

    case SCN_UPDATEUI:
        if (notify_code->updated & (SC_UPDATE_CONTENT) && (recheck_done || first_restyle) &&
            !restyling_caused_recheck_was_done) // If restyling wasn't caused by user input...
        {
            get_spell_checker()->recheck_visible(npp_interface().active_view());
            if (!first_restyle)
                restyling_caused_recheck_was_done = true;
            first_restyle = false;
        }
        else if (notify_code->updated &
            (SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL) &&
            recheck_done) // If scroll wasn't caused by user input...
        {
            get_spell_checker()->recheck_visible(npp_interface().active_view(), true);
            restyling_caused_recheck_was_done = false;
        }
        get_spell_checker()->hide_suggestion_box();
        break;

    case SCN_MODIFIED:
        if (notify_code->modificationType &
            (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT))
        {
            if (recheck_timer)
            {
                SetTimer(npp_data.npp_handle, recheck_timer, get_settings().recheck_delay, do_recheck);
                recheck_done = false;
            }
        }
        break;

    case NPPN_LANGCHANGED:
        {
            get_spell_checker()->lang_change();
        }
        break;

    case NPPN_TBMODIFICATION:
        {
            add_icons();
        }

    default:
        return;
    }
}

// Here you can process the Npp Messages
// I will make the messages accessible little by little, according to the need
// of plugin development.
// Please let me know if you need to access to some messages :

void init_needed_dialogs(WPARAM w_param)
{
    // A little bit of code duplication here :(
    auto menu_id = w_param;
    if ((!get_use_allocated_ids() && HIBYTE(menu_id) != DSPELLCHECK_MENU_ID &&
            HIBYTE(menu_id) != LANGUAGE_MENU_ID) ||
        (get_use_allocated_ids() && ((int)menu_id < get_context_menu_id_start() ||
            (int)menu_id > get_context_menu_id_start() + 350)))
        return;
    int used_menu_id;
    if (get_use_allocated_ids())
    {
        used_menu_id = ((int)menu_id < get_langs_menu_id_start()
                            ? DSPELLCHECK_MENU_ID
                            : LANGUAGE_MENU_ID);
    }
    else
    {
        used_menu_id = HIBYTE(menu_id);
    }
    switch (used_menu_id)
    {
    case LANGUAGE_MENU_ID:
        WPARAM result;
        if (!get_use_allocated_ids())
            result = LOBYTE(menu_id);
        else
            result = menu_id - get_langs_menu_id_start();
        if (result == DOWNLOAD_DICS)
            get_download_dics()->do_dialog();
        else if (result == CUSTOMIZE_MULTIPLE_DICS)
        {
            get_lang_list()->do_dialog();
        }
        else if (result == REMOVE_DICS)
        {
            get_remove_dics()->do_dialog();
        }
        break;
    }
}


extern "C" __declspec(dllexport) LRESULT
// ReSharper disable once CppInconsistentNaming
messageProc(UINT message, WPARAM w_param, LPARAM /*lParam*/)
{
    // NOLINT
    switch (message)
    {
    case WM_MOVE:
        if (get_spell_checker())
            get_spell_checker()->hide_suggestion_box();
        return false;
    case WM_COMMAND:
        {
            if (HIWORD(w_param) == 0 && get_use_allocated_ids())
            {
                init_needed_dialogs(w_param);
                get_spell_checker()->process_menu_result(w_param);
            }
        }
        break;
    }

    return false;
}

#ifdef UNICODE
// ReSharper disable once CppInconsistentNaming
extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; } // NOLINT
#endif // UNICODE
