// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "Plugin.h"

//
// put the headers you need here
//

#include "Constants.h"
#include "menuCmdID.h"
#include "resource.h"
#include "Settings.h"
#include "CheckedList/CheckedList.h"
#include "common/raii.h"
#include "common/winapi.h"
#include "core/SpellChecker.h"
#include "core/SpellCheckerHelpers.h"
#include "npp/NppInterface.h"
#include "spellers/HunspellInterface.h"
#include "spellers/SpellerContainer.h"
#include "ui/AboutDialog.h"
#include "ui/AspellOptionsDialog.h"
#include "ui/ConnectionSettingsDialog.h"
#include "ui/ContextMenuHandler.h"
#include "ui/DownloadDictionariesDialog.h"
#include "ui/SelectMultipleLanguagesDialog.h"
#include "ui/MenuItem.h"
#include "ui/ProgressDialog.h"
#include "ui/RemoveDictionariesDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/SuggestionMenuButton.h"

#include <chrono>

#ifdef VLD_BUILD
#include <vld.h>
#endif // VLD_BUILD

FuncItem func_item[nb_func];
bool resources_inited = false;

FuncItem *get_func_item() {
  // Well maybe we should lock mutex here
  return func_item;
}

//
// The data of Notepad++ that you can use in your plugin commands
//

NppData npp_data;

namespace {
wchar_t ini_file_path[MAX_PATH];
DWORD custom_gui_message_ids[static_cast<int>(CustomGuiMessage::max)] = {0};
bool do_close_tag = false;
std::unique_ptr<SpellChecker> spell_checker;
std::unique_ptr<const SpellerContainer> speller_container;
std::unique_ptr<ContextMenuHandler> context_menu_handler;
std::unique_ptr<SettingsDialog> settings_dlg;
std::unique_ptr<SuggestionMenuButton> suggestions_button;
std::unique_ptr<SelectMultipleLanguagesDialog> lang_list_instance;
std::unique_ptr<RemoveDictionariesDialog> remove_dics_dlg;
std::unique_ptr<AspellOptionsDialog> aspell_options_dlg;
std::unique_ptr<ConnectionSettingsDialog> select_proxy_dlg;
std::unique_ptr<ProgressDialog> progress_dlg;
std::unique_ptr<DownloadDictionariesDialog> download_dics_dlg;
std::unique_ptr<AboutDialog> about_dlg;
std::unique_ptr<const Settings> settings;
int context_menu_id_start;
int langs_menu_id_start = 0;
bool use_allocated_ids;
std::unique_ptr<NppInterface> npp;

HANDLE h_module = nullptr;
HHOOK h_mouse_hook = nullptr;

} // namespace

//
// Initialize your plug-in data here
// It will be called while plug-in loading
LRESULT CALLBACK mouse_proc(int n_code, WPARAM w_param, LPARAM l_param) {
  switch (w_param) {
  case WM_MOUSEMOVE:
    context_menu_handler->init_suggestions_box(*suggestions_button);
    break;
  default:
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

const Settings &get_settings() { return *settings; }

std::wstring get_default_hunspell_path() {
  std::wstring path = ini_file_path;
  return path.substr(0, path.rfind(L'\\')) + L"\\Hunspell";
}

void set_hmodule(HANDLE h_module_arg) {
  h_module = h_module_arg;
  // Init it all dialog classes:
}

SelectMultipleLanguagesDialog *get_lang_list() { return lang_list_instance.get(); }

RemoveDictionariesDialog *get_remove_dics() { return remove_dics_dlg.get(); }
AspellOptionsDialog *get_aspell_options_dlg() { return aspell_options_dlg.get(); }

ConnectionSettingsDialog *get_select_proxy() { return select_proxy_dlg.get(); }

ProgressDialog *get_progress_dlg() { return progress_dlg.get(); }

DownloadDictionariesDialog *get_download_dics() { return download_dics_dlg.get(); }

HANDLE get_h_module() { return h_module; }

void create_hooks() {
  h_mouse_hook = SetWindowsHookEx(WH_MOUSE, mouse_proc, nullptr, GetCurrentThreadId());
}

void plugin_clean_up() {
  {
    auto mut = speller_container->modify();
    mut->cleanup();
  }
  UnhookWindowsHookEx(h_mouse_hook);
  WinApi::delete_file(get_debug_log_path().c_str());
}

void register_custom_messages() {
  for (int i = 0; i < static_cast<int>(CustomGuiMessage::max); i++) {
    custom_gui_message_ids[i] = RegisterWindowMessage(custom_gui_messages_names[i]);
  }
}

void init_npp_interface() { npp = std::make_unique<NppInterface>(&npp_data); }

void notify(SCNotification *notify_code) { npp->notify(notify_code); }

NppInterface &npp_interface() { return *npp; }

void erase_misspellings() { spell_checker->erase_all_misspellings(); }

void show_spell_check_menu_at_cursor() {
  auto menu = CreatePopupMenu();
  context_menu_handler->update_word_under_cursor_data();
  if (context_menu_handler->is_word_under_cursor_correct())
    return;
  MenuItem::append_to_menu(menu, context_menu_handler->get_suggestion_menu_items());

  tagTPMPARAMS tpm_params;
  tpm_params.cbSize = sizeof(tagTPMPARAMS);
  ACTIVE_VIEW_BLOCK(*npp);
  auto start = npp->get_selection_start();
  auto x = npp->get_point_x_from_position(start);
  auto y = npp->get_point_y_from_position(start);
  POINT pnt{x, y};
  ClientToScreen(npp->get_view_hwnd(), &pnt);
  auto cmd = TrackPopupMenuEx(menu, TPM_HORIZONTAL | TPM_RIGHTALIGN | TPM_RETURNCMD, pnt.x, pnt.y, npp->get_view_hwnd(), &tpm_params);
  context_menu_handler->process_menu_result(cmd);
}

void replace_with_1st_suggestion() {
  SpellCheckerHelpers::replace_current_word_with_topmost_suggestion(*npp, *spell_checker, *speller_container);
}

void ignore_for_current_session() {
  TextPosition pos, length;
  if (!spell_checker->is_word_under_cursor_correct(pos, length, true)) {
    ACTIVE_VIEW_BLOCK(*npp);
    auto wstr = npp->get_mapped_wstring_range(pos, pos + length);
    {
      auto mut = speller_container->modify();
      mut->ignore_word(std::move(wstr.str));
    }
  }
}

void copy_misspellings_to_clipboard() {
  auto str = spell_checker->get_all_misspellings_as_string();
  const size_t len = (str.length() + 1) * 2;
  HGLOBAL h_mem = GlobalAlloc(GMEM_MOVEABLE, len);
  memcpy(GlobalLock(h_mem), str.c_str(), len);
  GlobalUnlock(h_mem);
  OpenClipboard(nullptr);
  EmptyClipboard();
  SetClipboardData(CF_UNICODETEXT, h_mem);
  CloseClipboard();
}

void mark_lines_with_misspelling() {
  spell_checker->mark_lines_with_misspelling();
}

void reload_hunspell_dictionaries() {
  speller_container->get_hunspell_speller().reset_spellers();
  settings->settings_changed();
}

DWORD get_custom_gui_message_id(CustomGuiMessage message_id) { return custom_gui_message_ids[static_cast<int>(message_id)]; }

void switch_auto_check_text() {
  auto mut = settings->modify(SettingsModificationStyle::ignore_file_errors);
  mut->data.auto_check_text = !settings->data.auto_check_text;
}

void switch_debug_logging() {
  auto mut = settings->modify(SettingsModificationStyle::ignore_file_errors);
  mut->data.write_debug_log = !mut->data.write_debug_log;
  if (mut->data.write_debug_log) {
    WinApi::delete_file(get_debug_log_path().c_str());
  }
}

void open_debug_log() {
  ShellExecute(nullptr, L"open", get_debug_log_path().c_str(), nullptr, nullptr, SW_SHOW);
}

void start_settings() { settings_dlg->do_dialog(); }

void start_manual() {
  ShellExecute(nullptr, L"open", L"https://github.com/Predelnik/DSpellCheck/wiki/Manual", nullptr, nullptr, SW_SHOW);
}

void start_about_dlg() { about_dlg->do_dialog(); }

void start_language_list() { lang_list_instance->do_dialog(); }

void recheck_visible() {
  print_to_log(L"recheck_visible ()", npp->get_editor_hwnd());
  ACTIVE_VIEW_BLOCK(npp_interface());
  spell_checker->recheck_visible();
}

void find_next_mistake() { spell_checker->find_next_mistake(); }

void find_prev_mistake() { spell_checker->find_prev_mistake(); }

void quick_lang_change_context() {
  POINT pos;
  GetCursorPos(&pos);
  TrackPopupMenu(get_langs_sub_menu(), 0, pos.x, pos.y, 0, npp_data.npp_handle, nullptr);
}

enum_array<Action, int> action_index = []() {
  enum_array<Action, int> val;
  val.fill(-1);
  return val;
}();

//
// Initialization of your plug-in commands
// You should fill your plug-ins commands here
void command_menu_init() {
  //
  // Firstly we get the parameters from your plugin config file (if any)
  //

  // get path of plugin configuration
  ::SendMessage(npp_data.npp_handle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, reinterpret_cast<LPARAM>(ini_file_path));

  // if config path doesn't exist, we create it
  if (PathFileExists(ini_file_path) == FALSE) {
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
  action_index[Action::toggle_auto_spell_check] = set_next_command(rc_str(IDS_AUTO_SPELL_CHECK).c_str(), switch_auto_check_text);
  action_index[Action::find_next_error] = set_next_command(rc_str(IDS_FIND_NEXT_ERROR).c_str(), find_next_mistake);
  action_index[Action::find_prev_error] = set_next_command(rc_str(IDS_FIND_PREV_ERROR).c_str(), find_prev_mistake);

  action_index[Action::quick_language_change] = set_next_command(rc_str(IDS_CHANGE_CURRENT_LANG).c_str(), quick_lang_change_context);

  set_next_command(L"---", nullptr);

  action_index[Action::settings] = set_next_command(rc_str(IDS_SETTINGS).c_str(), start_settings);
  action_index[Action::copy_all_misspellings] = set_next_command(rc_str(IDS_COPY_ALL_MISSPELLED).c_str(), copy_misspellings_to_clipboard);
  action_index[Action::erase_all_misspellings] = set_next_command(rc_str(IDS_ERASE_ALL_MISSPELLED).c_str(), erase_misspellings);
  action_index[Action::reload_user_dictionaries] = set_next_command(rc_str(IDS_RELOAD_HUNSPELL).c_str(), reload_hunspell_dictionaries);
  action_index[Action::toggle_debug_logging] = set_next_command(rc_str(IDS_SWITCH_DEBUG_LOGGING).c_str(), switch_debug_logging);
  action_index[Action::open_debug_log] = set_next_command(rc_str(IDS_OPEN_DEBUG_LOG).c_str(), open_debug_log);
  action_index[Action::online_manual] = set_next_command(rc_str(IDS_ONLINE_MANUAL).c_str(), start_manual);
  action_index[Action::about] = set_next_command(rc_str(IDS_ABOUT).c_str(), start_about_dlg);
  action_index[Action::show_spell_check_menu_at_cursor] =
      set_next_command(rc_str(IDS_SHOW_SPELL_CHECK_MENU_AT_CURSOR).c_str(), show_spell_check_menu_at_cursor);

  action_index[Action::replace_with_1st_suggestion] = set_next_command(rc_str(IDS_REPLACE_WITH_1ST_SUGGESTION).c_str(), replace_with_1st_suggestion);

  action_index[Action::ignore_for_current_session] = set_next_command(rc_str(IDS_IGNORE_WORD_AT_CURSOR).c_str(), ignore_for_current_session);
  action_index[Action::mark_lines_with_misspelling] = set_next_command(rc_str(IDS_BOOKMARK_LINES_WITH_MISSPELLING).c_str(), mark_lines_with_misspelling);
  // add further set_next_command at the bottom to avoid breaking configured hotkeys
}

void add_icons() {
  auto dpi = GetDeviceCaps(GetWindowDC(npp->get_editor_hwnd()), LOGPIXELSX);
  int size = 16 * dpi / 96;
  static ToolbarIconsWrapper auto_check_icon{static_cast<HINSTANCE>(h_module), MAKEINTRESOURCE(IDI_AUTOCHECK), IMAGE_ICON, size, size, 0};
  ::SendMessage(npp_data.npp_handle, NPPM_ADDTOOLBARICON, static_cast<WPARAM>(func_item[0].cmd_id), reinterpret_cast<LPARAM>(auto_check_icon.get()));
}

static std::wstring get_menu_item_text(HMENU menu, UINT index) {
  auto str_len = GetMenuString(menu, index, nullptr, 0, MF_BYPOSITION);
  std::vector<wchar_t> buf(str_len + 1);
  GetMenuString(menu, index, buf.data(), static_cast<int>(buf.size()), MF_BYPOSITION);
  return buf.data();
}

static HMENU dspellcheck_menu_cached = nullptr;

HMENU get_this_plugin_menu() {
  if (dspellcheck_menu_cached)
    return dspellcheck_menu_cached;
  HMENU plugins_menu = npp->get_menu_handle(NPPPLUGINMENU);
  HMENU dspellcheck_menu = nullptr;
  int count = GetMenuItemCount(plugins_menu);
  for (int i = 0; i < count; i++) {
    auto str = get_menu_item_text(plugins_menu, i);
    if (str == getName()) {
      MENUITEMINFO mif;
      mif.fMask = MIIM_SUBMENU;
      mif.cbSize = sizeof(MENUITEMINFO);
      bool res = GetMenuItemInfo(plugins_menu, i, TRUE, &mif) != FALSE;

      if (res)
        dspellcheck_menu = mif.hSubMenu;
      break;
    }
  }
  return dspellcheck_menu_cached = dspellcheck_menu;
}

HMENU get_submenu(int item_id) {
  auto dspellcheck_menu = get_this_plugin_menu();
  if (dspellcheck_menu == nullptr)
    return nullptr;

  MENUITEMINFO mif;

  mif.fMask = MIIM_SUBMENU;
  mif.cbSize = sizeof(MENUITEMINFO);

  bool res = GetMenuItemInfo(dspellcheck_menu, item_id, FALSE, &mif) == FALSE;
  if (res)
    return nullptr;

  return mif.hSubMenu;
}

HMENU get_langs_sub_menu() { return get_submenu(get_func_item()[action_index[Action::quick_language_change]].cmd_id); }

void on_settings_changed() {
  npp->set_menu_item_check(get_func_item()[action_index[Action::toggle_auto_spell_check]].cmd_id, settings->data.auto_check_text);
  npp->set_menu_item_check(get_func_item()[action_index[Action::toggle_debug_logging]].cmd_id, settings->data.write_debug_log);
}

void init_classes() {
  INITCOMMONCONTROLSEX icc;

  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icc);

  InitCheckedListBox(static_cast<HINSTANCE>(h_module));

  settings = std::make_unique<Settings>(ini_file_path);
  settings->settings_changed.connect(on_settings_changed);

  speller_container = std::make_unique<SpellerContainer>(settings.get(), &npp_data);

  spell_checker = std::make_unique<SpellChecker>(settings.get(), *npp, *speller_container);

  context_menu_handler = std::make_unique<ContextMenuHandler>(*settings, *speller_container, *npp, *spell_checker);

  suggestions_button = std::make_unique<SuggestionMenuButton>(static_cast<HINSTANCE>(h_module), npp_data.npp_handle, *npp, *context_menu_handler, *settings);
  suggestions_button->do_dialog();

  settings_dlg = std::make_unique<SettingsDialog>(static_cast<HINSTANCE>(h_module), npp_data.npp_handle, *npp, *settings, *speller_container);
  download_dics_dlg = std::make_unique<DownloadDictionariesDialog>(static_cast<HINSTANCE>(h_module), npp_data.npp_handle, *settings, *speller_container);

  settings_dlg->download_dics_dlg_requested.connect([]() { download_dics_dlg->do_dialog(); });

  about_dlg = std::make_unique<AboutDialog>();
  about_dlg->init(static_cast<HINSTANCE>(h_module), npp_data.npp_handle);

  progress_dlg = std::make_unique<ProgressDialog>(*download_dics_dlg);
  progress_dlg->init(static_cast<HINSTANCE>(h_module), npp_data.npp_handle);

  lang_list_instance = std::make_unique<SelectMultipleLanguagesDialog>(static_cast<HINSTANCE>(h_module), npp_data.npp_handle, *settings, *speller_container);

  select_proxy_dlg = std::make_unique<ConnectionSettingsDialog>(*settings, *download_dics_dlg);
  select_proxy_dlg->init(static_cast<HINSTANCE>(h_module), npp_data.npp_handle);

  remove_dics_dlg = std::make_unique<RemoveDictionariesDialog>(static_cast<HINSTANCE>(h_module), npp_data.npp_handle, *settings, *speller_container);
  aspell_options_dlg = std::make_unique<AspellOptionsDialog>(static_cast<HINSTANCE>(h_module), npp_data.npp_handle, *settings, *speller_container);

  {
    auto mut = settings->modify_without_saving();
    mut->load();
  }
  resources_inited = true;
}

void command_menu_clean_up() {
}

//
// Function that initializes plug-in commands
//
static int counter = 0;

std::vector<std::unique_ptr<ShortcutKey>> shortcut_storage;

int set_next_command(const wchar_t *cmd_name, Pfuncplugincmd p_func) {
  if (counter >= nb_func) {
    assert(false); // Less actions specified in nb_func constant than added
    return -1;
  }

  if (p_func == nullptr) {
    counter++;
    return counter - 1;
  }

  lstrcpy(func_item[counter].item_name, cmd_name);
  func_item[counter].p_func = p_func;
  func_item[counter].init2_check = false;
  func_item[counter].p_sh_key = nullptr;
  counter++;

  return counter - 1;
}

std::wstring get_debug_log_path() {
  std::vector<wchar_t> buf(MAX_PATH);
  GetTempPath(static_cast<DWORD>(buf.size()), buf.data());
  std::wstring path = buf.data();
  path += L"\\DSpellCheck_Debug_Log.txt";
  return path;
}

void rearrange_menu() {
  auto plugin_menu = get_this_plugin_menu();
  auto submenu = CreatePopupMenu();
  auto list = {
      Action::copy_all_misspellings, Action::erase_all_misspellings, Action::mark_lines_with_misspelling,
      Action::replace_with_1st_suggestion, Action::ignore_for_current_session,
      Action::show_spell_check_menu_at_cursor, Action::reload_user_dictionaries, Action::toggle_debug_logging, Action::open_debug_log};
  for (auto action : list) {
    MENUITEMINFO info;
    info.cbSize = sizeof(info);
    info.dwTypeData = nullptr;
    info.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE | MIIM_DATA; // everything
    auto get_info = [&] { GetMenuItemInfo(plugin_menu, get_func_item()[action_index[action]].cmd_id, FALSE, &info); };
    get_info();
    std::vector<wchar_t> buf(info.cch + 1);
    ++info.cch; // the worst API ever?
    info.dwTypeData = buf.data();
    get_info();
    InsertMenuItem(submenu, GetMenuItemCount(submenu), TRUE, &info);
  }
  int removed_cnt = 0;
  for (auto action : list) {
    RemoveMenu(plugin_menu, get_func_item()[action_index[action]].cmd_id, MF_BYCOMMAND);
    ++removed_cnt;
  }

  {
    MENUITEMINFO mif;
    mif.fMask = MIIM_SUBMENU | MIIM_STATE | MIIM_STRING;
    mif.cbSize = sizeof(MENUITEMINFO);
    auto action_name = rc_str(IDS_ADDITIONAL_ACTIONS);
    mif.dwTypeData = const_cast<wchar_t *>(action_name.c_str());
    mif.cch = static_cast<UINT>(wcslen(mif.dwTypeData)) + 1;
    mif.hSubMenu = submenu;
    mif.fState = MFS_ENABLED;
    InsertMenuItem(plugin_menu, get_func_item()[action_index[Action::settings]].cmd_id, FALSE, &mif);
  }
}

extern FuncItem func_item[nb_func]; // NOLINT
extern NppData npp_data;            // NOLINT
extern bool do_close_tag;           // NOLINT
std::vector<std::pair<TextPosition, TextPosition>> check_queue;
std::optional<WinApi::Timer> edit_recheck_timer;
std::optional<WinApi::Timer> scroll_recheck_timer;
constexpr int scroll_recheck_timer_resolution = 100;
bool restyling_caused_recheck_was_done = false; // Hack to avoid eternal cycle in case of scintilla bug
bool first_restyle = true;                      // hack to successfully avoid checking hyperlinks
// when they appear on program start

WPARAM last_hwnd = NULL;
LPARAM last_coords = 0;
std::vector<MenuItem> cur_menu_list;
// Ok, trying to use window subclassing to handle messages

LRESULT CALLBACK subclass_proc(HWND h_wnd, UINT message, WPARAM w_param, LPARAM l_param, UINT_PTR /*u_id_subclass*/, DWORD_PTR /*dw_ref_data*/) {
  LRESULT ret; // int->LRESULT, fix x64 issue, still compatible with x86
  switch (message) {
  case WM_INITMENUPOPUP: {
    // Checking that it isn't system menu and nor any main menu except 0th
    if (!cur_menu_list.empty() && LOWORD(l_param) == 0 && HIWORD(l_param) == 0) {
      // Special check for 0th main menu item
      MENUBARINFO info;
      info.cbSize = sizeof(MENUBARINFO);
      GetMenuBarInfo(npp_data.npp_handle, OBJID_MENU, 0, &info);
      HMENU main_menu = info.hMenu;
      MENUITEMINFO file_menu_item;
      file_menu_item.cbSize = sizeof(MENUITEMINFO);
      file_menu_item.fMask = MIIM_SUBMENU;
      GetMenuItemInfo(main_menu, 0, TRUE, &file_menu_item);

      const auto menu = reinterpret_cast<HMENU>(w_param);
      if (file_menu_item.hSubMenu != menu && get_langs_sub_menu() != menu) {
        MenuItem::prepend_to_menu(menu, cur_menu_list);
      }
    }
    cur_menu_list.clear();
  }
  break;

  case WM_COMMAND:
    if (HIWORD(w_param) == 0) {
      if (!get_use_allocated_ids())
        context_menu_handler->process_menu_result(w_param);

      if (LOWORD(w_param) == IDM_FILE_PRINTNOW || LOWORD(w_param) == IDM_FILE_PRINT) {
        // Disable autocheck while printing
        bool prev_value = get_settings().data.auto_check_text;

        auto mut_settings = get_settings().modify();
        mut_settings->data.auto_check_text = false;
        ret = ::DefSubclassProc(h_wnd, message, w_param, l_param);
        mut_settings->data.auto_check_text = prev_value;

        return ret;
      }
    }
    break;
  case WM_NOTIFY:
    // Removing possibility of adding items to tab bar menu.
    if (reinterpret_cast<LPNMHDR>(l_param)->code == NM_RCLICK) {
      cur_menu_list.clear();
    }
    break;
  case WM_CONTEXTMENU:
    last_hwnd = w_param;
    last_coords = l_param;
    suggestions_button->display(false);
    context_menu_handler->precalculate_menu();
    return TRUE;
  case WM_DISPLAYCHANGE: {
    suggestions_button->display(false);
  }
  break;
  }

  if (message != 0) {
    if (message == get_custom_gui_message_id(CustomGuiMessage::generic_callback)) {
      const auto callback_ptr = std::unique_ptr<CallbackData>(reinterpret_cast<CallbackData *>(w_param));
      if (callback_ptr->alive_status.expired())
        return FALSE;

      callback_ptr->callback();
      return TRUE;
    }
  }
  ret = ::DefSubclassProc(h_wnd, message, w_param, l_param);
  return ret;
}

LRESULT
show_calculated_menu(std::vector<MenuItem> &&menu_list) {
  cur_menu_list = std::move(menu_list);
  return ::DefSubclassProc(npp_data.npp_handle, WM_CONTEXTMENU, last_hwnd, last_coords);
}

void init_menu_ids() {
  bool res = npp->is_allocate_cmdid_supported();

  if (res) {
    set_use_allocated_ids(true);
    auto id = npp->allocate_cmdid(requested_menu_count);
    set_context_menu_id_start(id);
    set_langs_menu_id_start(id + 103);
  }
}

// ReSharper disable CppInconsistentNaming
extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) {
  npp_data = notpadPlusData;
  init_npp_interface();
  init_menu_ids();
  command_menu_init();
  SetWindowSubclass(npp_data.npp_handle, subclass_proc, 0, 0);
}

extern "C" __declspec(dllexport) const wchar_t *getName() {
  static std::wstring plugin_name = rc_str(IDS_PLUGIN_NAME);
  return plugin_name.c_str();
}

extern "C" __declspec(dllexport) FuncItem *getFuncsArray(int *nbF) {
  *nbF = nb_func;
  return func_item;
}

// ReSharper restore CppInconsistentNaming

bool is_any_timer_active() {
  if (edit_recheck_timer && edit_recheck_timer->is_set())
    return true;

  if (scroll_recheck_timer && scroll_recheck_timer->is_set())
    return true;

  return false;
}

// For now doesn't look like there is such a need in check modified, but code
// stays until thorough testing
void WINAPI edit_recheck_callback() {
  edit_recheck_timer->stop_timer();

  ACTIVE_VIEW_BLOCK(npp_interface());
  spell_checker->recheck_visible();
  if (!first_restyle)
    restyling_caused_recheck_was_done = true;
  first_restyle = false;
}

void WINAPI scroll_recheck_callback() {
  scroll_recheck_timer->stop_timer();

  ACTIVE_VIEW_BLOCK(npp_interface());
  spell_checker->recheck_visible();
  if (!first_restyle)
    restyling_caused_recheck_was_done = true;
  first_restyle = false;
}

std::wstring_view rc_str_view(UINT string_id) {
  const wchar_t *ret = nullptr;
  auto len = LoadString(static_cast<HINSTANCE>(h_module), string_id, reinterpret_cast<LPWSTR>(&ret), 0);
  return {ret, static_cast<size_t>(len)};
}

std::wstring rc_str(UINT string_id) { return std::wstring{rc_str_view(string_id)}; }

void print_to_log(std::wstring_view line, HWND parent_wnd) {
  if (!settings)
    return;

  if (!settings->data.write_debug_log)
    return;

  FILE *fp;

  auto err = _wfopen_s(&fp, get_debug_log_path().c_str(), L"a");
  if (err != 0) {
    MessageBox(parent_wnd, L"Error while writing to a log file", to_wstring(strerror(err)).c_str(), MB_OK);
    return;
  }
  _fwprintf_p(fp, L"%.*s\n", static_cast<int>(line.length()), line.data());
  fclose(fp);
}

void delete_log() {
  if (settings->data.write_debug_log)
    WinApi::delete_file(get_debug_log_path().c_str());
}

void update_on_visible_area_changed() {
  if (!is_any_timer_active() && scroll_recheck_timer) {
    scroll_recheck_timer->set_resolution(std::chrono::milliseconds(scroll_recheck_timer_resolution));
  }
}

// ReSharper disable once CppInconsistentNaming
extern "C" __declspec(dllexport) void beNotified(SCNotification *notify_code) {
  notify(notify_code);
  switch (notify_code->nmhdr.code) {
  case NPPN_SHUTDOWN: {
    print_to_log(L"NPPN_SHUTDOWN", npp->get_editor_hwnd());
    edit_recheck_timer.reset();
    scroll_recheck_timer.reset();
    command_menu_clean_up();

    plugin_clean_up();
    RemoveWindowSubclass(npp_data.npp_handle, subclass_proc, 0);
  }
  break;

  case NPPN_READY: {
    register_custom_messages();
    init_classes();
    print_to_log(L"NPPN_READY", npp->get_editor_hwnd());
    check_queue.clear();
    create_hooks();
    edit_recheck_timer.emplace(npp_data.npp_handle);
    edit_recheck_timer->on_timer_tick.connect(edit_recheck_callback);
    scroll_recheck_timer.emplace(npp_data.npp_handle);
    scroll_recheck_timer->on_timer_tick.connect(scroll_recheck_callback);
    spell_checker->recheck_visible_both_views();
    restyling_caused_recheck_was_done = false;
    suggestions_button->set_transparency();
    rearrange_menu();
  }
  break;

  case NPPN_BUFFERACTIVATED: {
    if (settings)
      print_to_log(L"NPPN_BUFFERACTIVATED", npp->get_editor_hwnd());
    if (!spell_checker)
      return;
    recheck_visible();
    restyling_caused_recheck_was_done = false;
  }
  break;

  case SCN_FOLDINGSTATECHANGED:
    update_on_visible_area_changed();
    break;

  case SCN_UPDATEUI:
    if (!spell_checker)
      return;
    if ((notify_code->updated & SC_UPDATE_CONTENT) != 0 && (!is_any_timer_active() || first_restyle) && !restyling_caused_recheck_was_done)
      // If restyling wasn't caused
      // by user input...
    {
      ACTIVE_VIEW_BLOCK(npp_interface());
      spell_checker->recheck_visible();
      if (!first_restyle)
        restyling_caused_recheck_was_done = true;
      first_restyle = false;
    } else if ((notify_code->updated & (SC_UPDATE_V_SCROLL | SC_UPDATE_H_SCROLL)) != 0) // If scroll wasn't caused by user input...
    {
      update_on_visible_area_changed();
    }
    if (!suggestions_button->is_pressed())
      suggestions_button->display(false);
    break;

  case SCN_ZOOM:
    update_on_visible_area_changed();
    break;

  case SCN_MODIFIED:
    if (!spell_checker)
      return;
    if (edit_recheck_timer && (notify_code->modificationType & (SC_MOD_DELETETEXT | SC_MOD_INSERTTEXT)) != 0) {
      edit_recheck_timer->set_resolution(std::chrono::milliseconds(get_settings().data.recheck_delay));
    }
    break;

  case NPPN_LANGCHANGED: {
    print_to_log(L"NPPN_LANGCHANGED", npp->get_editor_hwnd());
    if (!spell_checker)
      return;
    spell_checker->recheck_visible_on_active_view();
  }
  break;

  case NPPN_TBMODIFICATION: {
    if (settings)
      print_to_log(L"NPPN_TBMODIFICATION", npp->get_editor_hwnd());
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

void init_needed_dialogs(WPARAM w_param) {
  // A little bit of code duplication here :(
  auto menu_id = w_param;
  if ((!get_use_allocated_ids() && HIBYTE(menu_id) != menu_id::plguin_menu && HIBYTE(menu_id) != menu_id::language_menu) ||
      (get_use_allocated_ids() && (static_cast<int>(menu_id) < get_context_menu_id_start() || static_cast<int>(menu_id) > get_context_menu_id_start() + 350)))
    return;
  int used_menu_id;
  if (get_use_allocated_ids()) {
    used_menu_id = (static_cast<int>(menu_id) < get_langs_menu_id_start() ? menu_id::plguin_menu : menu_id::language_menu);
  } else {
    used_menu_id = HIBYTE(menu_id);
  }
  switch (used_menu_id) {
  case menu_id::language_menu:
    WPARAM result;
    if (!get_use_allocated_ids())
      result = LOBYTE(menu_id);
    else
      result = menu_id - get_langs_menu_id_start();
    if (result == menu_id::download_dictionaries)
      download_dics_dlg->do_dialog();
    else if (result == menu_id::customize_multiple_languages) {
      get_lang_list()->do_dialog();
    } else if (result == menu_id::remove_dictionaries) {
      get_remove_dics()->do_dialog();
    }
    break;
  }
}

extern "C" __declspec(dllexport) LRESULT
// ReSharper disable once CppInconsistentNaming
messageProc(UINT message, WPARAM w_param, LPARAM /*l_param*/) {
  // NOLINT
  switch (message) {
  case WM_MOVE:
    if (suggestions_button)
      suggestions_button->display(false);
    return FALSE;
  case WM_COMMAND: {
    if (HIWORD(w_param) == 0 && get_use_allocated_ids()) {
      init_needed_dialogs(w_param);
      context_menu_handler->process_menu_result(w_param);
    }
  }
  break;
  }

  return FALSE;
}

#ifdef UNICODE
// ReSharper disable once CppInconsistentNaming
extern "C" __declspec(dllexport) BOOL isUnicode() { return TRUE; } // NOLINT
#endif                                                             // UNICODE
