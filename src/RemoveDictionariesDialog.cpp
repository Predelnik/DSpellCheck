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

#include "RemoveDictionariesDialog.h"

#include "CheckedList/CheckedList.h"
#include "MainDef.h"
#include "HunspellInterface.h"
#include "Plugin.h"
#include "SpellChecker.h"

#include "resource.h"
#include "LanguageInfo.h"
#include "Settings.h"

RemoveDictionariesDialog::RemoveDictionariesDialog(HINSTANCE h_inst, HWND parent, const Settings& settings) :
    m_settings(settings)
{
    m_settings.settings_changed.connect([this] { update_list(); });
    get_spell_checker()->speller_status_changed.connect([this] { update_list(); });
    Window::init(h_inst, parent);
    m_settings.settings_changed.connect([this] { update_controls(); });
}

void RemoveDictionariesDialog::do_dialog()
{
    if (!isCreated())
    {
        create(IDD_REMOVE_DICS);
    }
    else
    {
        goToCenter();
        display();
    }
    SetFocus(m_lang_list);
}

void RemoveDictionariesDialog::update_list()
{
    if (!m_lang_list)
        return;

    ListBox_ResetContent(m_lang_list);
    for (auto& lang : get_spell_checker()->get_available_languages())
        ListBox_AddString(m_lang_list,
        (std::wstring (lang.get_aliased_name(m_settings.use_language_name_aliases)) + (lang.for_all_users ?
            L" [!For All Users]" : L"")).c_str ());
}


HWND RemoveDictionariesDialog::get_list_box() { return m_lang_list; }

void RemoveDictionariesDialog::remove_selected(SpellChecker* spell_checker_instance)
{
    int count = 0;
    bool need_single_reset = false;
    bool need_multi_reset = false;
    bool single_temp, multi_temp;
    for (int i = 0; i < ListBox_GetCount(m_lang_list); i++)
    {
        if (CheckedListBox_GetCheckState(m_lang_list, i) == BST_CHECKED)
        {
            wchar_t file_name[MAX_PATH];
            for (int j = 0; j < 1 + m_settings.remove_system_dictionaries ? 1 : 0;
                 j++)
            {
                *file_name = L'\0';
                wcscat(file_name,
                       ((j == 0)
                            ? m_settings.hunspell_user_path
                            : m_settings.hunspell_system_path).c_str());
                wcscat(file_name, L"\\");
                wcscat(file_name, spell_checker_instance->get_available_languages()[i].orig_name.c_str());
                wcscat(file_name, L".aff");
                SetFileAttributes(file_name, FILE_ATTRIBUTE_NORMAL);
                bool success = DeleteFile(file_name);
                wcsncpy(file_name + wcslen(file_name) - 4, L".dic", 4);
                SetFileAttributes(file_name, FILE_ATTRIBUTE_NORMAL);
                success = success && DeleteFile(file_name);
                if (m_settings.remove_user_dictionaries)
                {
                    wcsncpy(file_name + wcslen(file_name) - 4, L".usr", 4);
                    SetFileAttributes(file_name, FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(file_name); // Success doesn't matter in that case, 'cause
                    // dictionary might not exist.
                }
                if (success)
                {
                    file_name[wcslen(file_name) - 4] = L'\0';
                    spell_checker_instance->get_hunspell_speller()->update_on_dic_removal(
                        file_name, single_temp, multi_temp);
                    need_single_reset |= single_temp;
                    need_multi_reset |= multi_temp;
                    count++;
                }
            }
        }
    }
    for (int i = 0; i < ListBox_GetCount(m_lang_list); i++)
        CheckedListBox_SetCheckState(m_lang_list, i, BST_UNCHECKED);
    if (count > 0)
    {
        spell_checker_instance->hunspell_reinit_settings(true);
        update_list();
        spell_checker_instance->do_plugin_menu_inclusion();
        spell_checker_instance->recheck_visible_both_views();
        wchar_t buf[DEFAULT_BUF_SIZE];
        swprintf(buf, L"%d dictionary(ies) has(ve) been successfully removed",
                 count);
        MessageBox(_hParent, buf, L"Dictionaries were removed",
                   MB_OK | MB_ICONINFORMATION);
    }
}

void RemoveDictionariesDialog::update_options()
{
    auto mut_settings = m_settings.modify();
    mut_settings->remove_user_dictionaries = Button_GetCheck(m_remove_user_dics) ==
        BST_CHECKED;
    mut_settings->remove_system_dictionaries = Button_GetCheck(m_remove_system) ==
        BST_CHECKED;
}

void RemoveDictionariesDialog::update_controls()
{
    Button_SetCheck(m_remove_user_dics,
        m_settings.remove_user_dictionaries ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_remove_system, m_settings.remove_system_dictionaries ? BST_CHECKED : BST_UNCHECKED);
}

INT_PTR RemoveDictionariesDialog::run_dlg_proc(UINT message, WPARAM w_param,
                                               LPARAM /*lParam*/)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            m_lang_list = ::GetDlgItem(_hSelf, IDC_REMOVE_LANGLIST);
            m_remove_user_dics = ::GetDlgItem(_hSelf, IDC_REMOVE_USER_DICS);
            m_remove_system = ::GetDlgItem(_hSelf, IDC_REMOVE_SYSTEM);
            update_list();
            update_controls();
            return true;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(w_param))
            {
            case IDC_REMOVE_USER_DICS:
            case IDC_REMOVE_SYSTEM:
                {
                    if (HIWORD(w_param) == BN_CLICKED)
                    {
                        update_options();
                    }
                }
                break;
            case IDOK:
                if (HIWORD(w_param) == BN_CLICKED)
                {
                    display(false);
                    remove_selected(get_spell_checker());
                }
                break;
            case IDCANCEL:
                if (HIWORD(w_param) == BN_CLICKED)
                {
                    display(false);
                    update_list(); // reset checkboxes
                }

                break;
            }
        }
        break;
    };
    return false;
}
