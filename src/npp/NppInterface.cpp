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

#include "NppInterface.h"

#include "menuCmdID.h"
#include "Notepad_plus_msgs.h"
#include "PluginInterface.h"
#include "plugin/Constants.h"

#include <cassert>

NppInterface::NppInterface(const NppData *nppData)
  : m_npp_data{*nppData} {
}

std::wstring NppInterface::get_npp_directory() {
  std::vector<wchar_t> npp_path(MAX_PATH);
  send_msg_to_npp(NPPM_GETNPPDIRECTORY, MAX_PATH, reinterpret_cast<LPARAM>(npp_path.data()));
  return npp_path.data();
}

bool NppInterface::is_allocate_cmdid_supported() const {
  return send_msg_to_npp(NPPM_ALLOCATESUPPORTED) != 0;
}

int NppInterface::allocate_cmdid(int requested_number) {
  int id;
  send_msg_to_npp(NPPM_ALLOCATECMDID, requested_number, reinterpret_cast<LPARAM>(&id));
  return id;
}

void NppInterface::set_menu_item_check(int cmd_id, bool checked) { send_msg_to_npp(NPPM_SETMENUITEMCHECK, cmd_id, static_cast<LPARAM>(checked)); }

HMENU NppInterface::get_menu_handle(int menu_type) const { return reinterpret_cast<HMENU>(send_msg_to_npp(NPPM_GETMENUHANDLE, menu_type)); }

int NppInterface::get_target_view() const {
  return static_cast<int>(m_target_view);
}

int NppInterface::get_indicator_value_at(int indicator_id, TextPosition position) const {
  return static_cast<int>(send_msg_to_scintilla(SCI_INDICATORVALUEAT, indicator_id, position));
}

LRESULT NppInterface::send_msg_to_npp(UINT Msg, WPARAM wParam, LPARAM lParam) const { return SendMessage(m_npp_data.npp_handle, Msg, wParam, lParam); }

HWND NppInterface::get_view_hwnd() const {
  auto handle = m_npp_data.scintilla_main_handle;
  switch (m_target_view) {
  case NppViewType::primary:
    handle = m_npp_data.scintilla_main_handle;
    break;
  case NppViewType::secondary:
    handle = m_npp_data.scintilla_second_handle;
    break;
  case NppViewType::COUNT:
    break;
  }
  return handle;
}

LRESULT NppInterface::send_msg_to_scintilla(UINT msg, WPARAM w_param, LPARAM l_param) const {
  assert(m_target_view != NppViewType::COUNT && "Outside view scope");
  return SendMessage(get_view_hwnd(), msg, w_param, l_param);
}

void NppInterface::post_msg_to_scintilla(UINT msg, WPARAM w_param, LPARAM l_param) const {
  assert(m_target_view != NppViewType::COUNT && "Outside view scope");
  PostMessage(get_view_hwnd(), msg, w_param, l_param);
}

std::wstring NppInterface::get_dir_msg(UINT msg) const {
  std::vector<wchar_t> buf(MAX_PATH);
  send_msg_to_npp(msg, MAX_PATH, reinterpret_cast<LPARAM>(buf.data()));
  return {buf.data()};
}

int NppInterface::to_index(NppViewType target) {
  switch (target) {
  case NppViewType::primary:
    return 0;
  case NppViewType::secondary:
    return 1;
  case NppViewType::COUNT:
    break;
  }
  assert(false);
  return 0;
}

int NppInterface::active_view() const {
  int cur_scintilla = 0;
  send_msg_to_npp(NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&cur_scintilla));
  return cur_scintilla;
}

std::optional<POINT> NppInterface::get_mouse_cursor_pos() const {
  POINT p;
  if (GetCursorPos(&p) == FALSE)
    return std::nullopt;

  return p;
}

bool NppInterface::open_document(std::wstring filename) { return send_msg_to_npp(NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(filename.data())) == TRUE; }

bool NppInterface::is_opened(const std::wstring &filename) const {
  auto filenames = get_open_filenames();
  return std::find(filenames.begin(), filenames.end(), filename) != filenames.end();
}

EditorCodepage NppInterface::get_encoding() const {
  auto CodepageId = static_cast<int>(send_msg_to_scintilla(SCI_GETCODEPAGE, 0, 0));
  if (CodepageId == SC_CP_UTF8)
    return EditorCodepage::utf8;

  return EditorCodepage::ansi;
}

void NppInterface::activate_document(int index) { send_msg_to_npp(NPPM_ACTIVATEDOC, to_index(m_target_view), index); }

void NppInterface::activate_document(const std::wstring &filepath) {
  auto fnames = get_open_filenames();
  auto it = std::find(fnames.begin(), fnames.end(), filepath);
  if (it == fnames.end()) // exception prbbly
    return;

  activate_document(static_cast<int>(it - fnames.begin()));
}

std::wstring NppInterface::active_document_path() const { return get_dir_msg(NPPM_GETFULLCURRENTPATH); }

void NppInterface::switch_to_file(const std::wstring &path) { send_msg_to_npp(NPPM_SWITCHTOFILE, 0, reinterpret_cast<LPARAM>(path.data())); }

std::wstring NppInterface::active_file_directory() const { return get_dir_msg(NPPM_GETCURRENTDIRECTORY); }

void NppInterface::do_command(int id) { send_msg_to_npp(WM_COMMAND, id); }

// [!] constant from Notepad++ sources which might be subject to change
constexpr auto MARK_BOOKMARK = 24;

void NppInterface::add_bookmark(TextPosition line) {
  send_msg_to_scintilla(SCI_MARKERADD, line, MARK_BOOKMARK);
}

constexpr auto npp_view_count = 2;

int NppInterface::get_view_count() const {
  return npp_view_count;
}

void NppInterface::set_target_view(int view_index) const {
  m_target_view = static_cast<NppViewType>(view_index);
}

int NppInterface::get_text_height(int line) const { return static_cast<int>(send_msg_to_scintilla(SCI_TEXTHEIGHT, line)); }

int NppInterface::get_point_x_from_position(TextPosition position) const {
  return static_cast<int>(send_msg_to_scintilla(SCI_POINTXFROMPOSITION, 0, position));
}

int NppInterface::get_point_y_from_position(TextPosition position) const {
  return static_cast<int>(send_msg_to_scintilla(SCI_POINTYFROMPOSITION, 0, position));
}

bool NppInterface::is_line_visible(TextPosition line) const {
  return send_msg_to_scintilla(SCI_GETLINEVISIBLE, line);
}

TextPosition NppInterface::find_next(TextPosition from_position, const char *needle) {
  Sci_TextToFind ttf;
  ttf.chrg.cpMin = static_cast<Sci_PositionCR>(from_position);
  ttf.chrg.cpMax = static_cast<Sci_PositionCR>(get_active_document_length());
  ttf.lpstrText = needle;
  return send_msg_to_scintilla(SCI_FINDTEXT, 0, reinterpret_cast<LPARAM>(&ttf));
}

void NppInterface::replace_text(TextPosition from, TextPosition to, std::string_view replacement) {
  send_msg_to_scintilla(SCI_SETTARGETSTART, from);
  send_msg_to_scintilla(SCI_SETTARGETEND, to);
  send_msg_to_scintilla(SCI_REPLACETARGET, replacement.size(), reinterpret_cast<LPARAM>(replacement.data()));
}

void NppInterface::set_selection(TextPosition from, TextPosition to) { post_msg_to_scintilla(SCI_SETSEL, from, to); }

void NppInterface::replace_selection(const char *str) { send_msg_to_scintilla(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(str)); }

void NppInterface::set_indicator_style(int indicator_index, int style) {
  send_msg_to_scintilla(SCI_INDICSETSTYLE, indicator_index, style);
}

void NppInterface::set_indicator_foreground(int indicator_index, int style) {
  send_msg_to_scintilla(SCI_INDICSETFORE, indicator_index, style);
}

void NppInterface::set_current_indicator(int indicator_index) { post_msg_to_scintilla(SCI_SETINDICATORCURRENT, indicator_index); }

void NppInterface::indicator_fill_range(TextPosition from, TextPosition to) { post_msg_to_scintilla(SCI_INDICATORFILLRANGE, from, to - from); }

void NppInterface::indicator_clear_range(TextPosition from, TextPosition to) { post_msg_to_scintilla(SCI_INDICATORCLEARRANGE, from, to - from); }

TextPosition NppInterface::get_first_visible_line() const { return send_msg_to_scintilla(SCI_GETFIRSTVISIBLELINE); }

TextPosition NppInterface::get_lines_on_screen() const { return send_msg_to_scintilla(SCI_LINESONSCREEN); }

TextPosition NppInterface::get_document_line_from_visible(TextPosition visible_line) const {
  return send_msg_to_scintilla(SCI_DOCLINEFROMVISIBLE, visible_line);
}

TextPosition NppInterface::get_document_line_count() const { return send_msg_to_scintilla(SCI_GETLINECOUNT); }

std::string NppInterface::get_active_document_text() const {
  TARGET_VIEW_BLOCK(*this, static_cast<int> (m_target_view));
  auto buf_len = get_active_document_length() + 1;
  std::vector<char> buf(buf_len);
  send_msg_to_scintilla(SCI_GETTEXT, buf_len, reinterpret_cast<LPARAM>(buf.data()));
  return buf.data();
}

std::wstring NppInterface::get_full_current_path() const {
  std::vector<wchar_t> full_path(MAX_PATH);
  send_msg_to_npp(NPPM_GETFULLCURRENTPATH, MAX_PATH, reinterpret_cast<LPARAM>(full_path.data()));
  return full_path.data();
}

RECT NppInterface::editor_rect() const {
  RECT out;
  GetWindowRect(get_view_hwnd(), &out);
  OffsetRect(&out, -out.left, -out.top);
  return out;
}

void NppInterface::delete_range(TextPosition start, TextPosition length) {
  send_msg_to_scintilla(SCI_DELETERANGE, start, length);
}

void NppInterface::begin_undo_action() {
  send_msg_to_scintilla(SCI_BEGINUNDOACTION);
}

void NppInterface::end_undo_action() {
  send_msg_to_scintilla(SCI_ENDUNDOACTION);
}

void NppInterface::undo() {
  send_msg_to_scintilla(SCI_UNDO);
}

std::optional<TextPosition> NppInterface::char_position_from_global_point(int x, int y) const {
  POINT p;
  p.x = x;
  p.y = y;
  auto hwnd = get_view_hwnd();
  if (WindowFromPoint(p) != hwnd)
    return std::nullopt;
  ScreenToClient(hwnd, &p);
  return send_msg_to_scintilla(SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);
}

TextPosition NppInterface::char_position_from_point(const POINT &pnt) const {
  return send_msg_to_scintilla(SCI_CHARPOSITIONFROMPOINT, pnt.x, pnt.y);
}

TextPosition NppInterface::get_selection_start() const { return send_msg_to_scintilla(SCI_GETSELECTIONSTART); }

TextPosition NppInterface::get_selection_end() const { return send_msg_to_scintilla(SCI_GETSELECTIONEND); }

TextPosition NppInterface::get_line_length(int line) const { return send_msg_to_scintilla(SCI_LINELENGTH, line); }

std::string NppInterface::get_line(TextPosition line_number) const {
  auto buf_size = static_cast<int>(send_msg_to_scintilla(SCI_LINELENGTH, line_number) + 1);
  std::vector<char> buf(buf_size);
  auto ret = static_cast<int>(send_msg_to_scintilla(SCI_GETLINE, line_number, reinterpret_cast<LPARAM>(buf.data())));
  static_cast<void>(ret);
  assert(ret == buf_size - 1);
  return buf.data();
}

void NppInterface::move_active_document_to_other_view() { do_command(IDM_VIEW_GOTO_ANOTHER_VIEW); }

void NppInterface::add_toolbar_icon(int cmdId, const toolbarIcons *toolBarIconsPtr) {
  send_msg_to_npp(NPPM_ADDTOOLBARICON, static_cast<WPARAM>(cmdId), reinterpret_cast<LPARAM>(toolBarIconsPtr));
}

std::string NppInterface::selected_text() const {
  auto sel_buf_size = static_cast<int>(send_msg_to_scintilla(SCI_GETSELTEXT, 0, 0)) + 1;
  if (sel_buf_size > 1) // Because it includes terminating '\0'
  {
    std::vector<char> buf(sel_buf_size);
    send_msg_to_scintilla(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(buf.data()));
    return buf.data();
  }
  return {};
}

std::string NppInterface::get_current_line() const { return get_line(get_current_line_number()); }

TextPosition NppInterface::get_current_pos() const { return static_cast<int>(send_msg_to_scintilla(SCI_GETCURRENTPOS, 0, 0)); }

int NppInterface::get_current_line_number() const {
  return line_from_position(get_current_pos());
}

int NppInterface::line_from_position(TextPosition position) const {
  return static_cast<int>(send_msg_to_scintilla(SCI_LINEFROMPOSITION, position));
}

std::wstring NppInterface::plugin_config_dir() const { return get_dir_msg(NPPM_GETPLUGINSCONFIGDIR); }

TextPosition NppInterface::get_line_start_position(TextPosition line) const {
  return static_cast<int>(send_msg_to_scintilla(SCI_POSITIONFROMLINE, line));
}

TextPosition NppInterface::get_line_end_position(TextPosition line) const {
  return static_cast<int>(send_msg_to_scintilla(SCI_GETLINEENDPOSITION, line));
}

HWND NppInterface::get_editor_hwnd() const { return m_npp_data.npp_handle; }

void NppInterface::notify(SCNotification *notify_code) {
  switch (notify_code->nmhdr.code) {
  case NPPN_LANGCHANGED: {
    for (auto &val : m_lexer_cache)
      val = std::nullopt;
  }
  break;
  case NPPN_BUFFERACTIVATED: {
    for (auto &val : m_lexer_cache)
      val = std::nullopt;
  }
  break;
  }
}

int NppInterface::get_lexer() const {
  if (m_lexer_cache[m_target_view])
    return *m_lexer_cache[m_target_view];
  return *(m_lexer_cache[m_target_view] = static_cast<int>(send_msg_to_scintilla(SCI_GETLEXER)));
}

int NppInterface::get_style_at(TextPosition position) const { return static_cast<int>(send_msg_to_scintilla(SCI_GETSTYLEAT, position)); }

TextPosition NppInterface::get_active_document_length() const { return send_msg_to_scintilla(SCI_GETLENGTH); }

std::string NppInterface::get_text_range(TextPosition from, TextPosition to) const {
  if (from > to) {
    assert(false); // Incorrect request to Scintilla. Prevent possible crash.
    return {};
  }
  Sci_TextRange range;
  range.chrg.cpMin = static_cast<Sci_PositionCR>(from);
  range.chrg.cpMax = static_cast<Sci_PositionCR>(to);
  std::vector<char> buf(range.chrg.cpMax - range.chrg.cpMin + 1);
  range.lpstrText = buf.data();
  send_msg_to_scintilla(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&range));
  return buf.data();
}

void NppInterface::force_style_update(TextPosition from, TextPosition to) { send_msg_to_scintilla(SCI_COLOURISE, from, to); }

std::vector<std::wstring> NppInterface::get_open_filenames_helper(int enum_val, int msg) const {

  ASSERT_RETURN(enum_val >= 0, {});
  auto count = static_cast<int>(send_msg_to_npp(NPPM_GETNBOPENFILES, 0, enum_val));

  auto paths = new wchar_t *[count];
  for (int i = 0; i < count; ++i)
    paths[i] = new wchar_t[MAX_PATH];

  {
    auto ret = send_msg_to_npp(msg, reinterpret_cast<WPARAM>(paths), count);
    static_cast<void>(ret);
    assert(ret == count);
  }

  std::vector<std::wstring> res;
  for (int i = 0; i < count; ++i) {
    res.emplace_back(paths[i]);
    delete[] paths[i];
  }
  delete[] paths;
  return res;
}

std::vector<std::wstring> NppInterface::get_open_filenames_all_views() const {
  return get_open_filenames_helper(ALL_OPEN_FILES, NPPM_GETOPENFILENAMES);
}

std::vector<std::wstring> NppInterface::get_open_filenames() const {
  switch (m_target_view) {
  case NppViewType::primary:
    return get_open_filenames_helper(PRIMARY_VIEW, NPPM_GETOPENFILENAMESPRIMARY);
    break;
  case NppViewType::secondary:
    return get_open_filenames_helper(SECOND_VIEW, NPPM_GETOPENFILENAMESSECOND);
    break;
  case NppViewType::COUNT:
    break;
  }
  assert(!"Incorrect target_view");
  return {};
}
