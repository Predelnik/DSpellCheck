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

#include "MainDefs.h"
#include "Notepad_plus_msgs.h"
#include "PluginInterface.h"
#include "menuCmdID.h"
#include <cassert>

NppInterface::NppInterface(const NppData *nppData) : m_npp_data{*nppData} {}

std::wstring NppInterface::get_npp_directory() {
  std::vector<wchar_t> npp_path(MAX_PATH);
  send_msg_to_npp(NPPM_GETNPPDIRECTORY, MAX_PATH, reinterpret_cast<LPARAM>(npp_path.data()));
  return npp_path.data();
}

bool NppInterface::is_allocate_cmdid_supported() const { return send_msg_to_npp(NPPM_ALLOCATESUPPORTED) != 0; }

int NppInterface::allocate_cmdid(int start_number) {
  int id;
  send_msg_to_npp(NPPM_ALLOCATECMDID, start_number, reinterpret_cast<LPARAM>(&id));
  return id;
}

void NppInterface::set_menu_item_check(int cmd_id, bool checked) { send_msg_to_npp(NPPM_SETMENUITEMCHECK, cmd_id, static_cast<LPARAM>(checked)); }

HMENU NppInterface::get_menu_handle(int menu_type) const { return reinterpret_cast<HMENU>(send_msg_to_npp(NPPM_GETMENUHANDLE, menu_type)); }

LRESULT NppInterface::send_msg_to_npp(UINT Msg, WPARAM wParam, LPARAM lParam) const { return SendMessage(m_npp_data.npp_handle, Msg, wParam, lParam); }

HWND NppInterface::get_scintilla_hwnd(EditorViewType view) const {
  auto handle = m_npp_data.scintilla_main_handle;
  switch (view) {
  case EditorViewType::primary:
    handle = m_npp_data.scintilla_main_handle;
    break;
  case EditorViewType::secondary:
    handle = m_npp_data.scintilla_second_handle;
    break;
  case EditorViewType::COUNT:
    break;
  }
  return handle;
}

LRESULT NppInterface::send_msg_to_scintilla(EditorViewType view, UINT msg, WPARAM w_param, LPARAM l_param) const {
  return SendMessage(get_scintilla_hwnd(view), msg, w_param, l_param);
}

void NppInterface::post_msg_to_scintilla(EditorViewType view, UINT msg, WPARAM w_param, LPARAM l_param) const {
  PostMessage(get_scintilla_hwnd(view), msg, w_param, l_param);
}

std::wstring NppInterface::get_dir_msg(UINT msg) const {
  std::vector<wchar_t> buf(MAX_PATH);
  send_msg_to_npp(msg, MAX_PATH, reinterpret_cast<LPARAM>(buf.data()));
  return {buf.data()};
}

int NppInterface::to_index(EditorViewType target) {
  switch (target) {
  case EditorViewType::primary:
    return 0;
  case EditorViewType::secondary:
    return 1;
  case EditorViewType::COUNT:
    break;
  }
  assert(false);
  return 0;
}

EditorViewType NppInterface::active_view() const {
  int cur_scintilla = 0;
  send_msg_to_npp(NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&cur_scintilla));
  if (cur_scintilla == 0)
    return EditorViewType::primary;
  if (cur_scintilla == 1)
    return EditorViewType::secondary;

  assert(false);
  return EditorViewType::primary;
}

bool NppInterface::open_document(std::wstring filename) { return send_msg_to_npp(NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(filename.data())) == TRUE; }

bool NppInterface::is_opened(const std::wstring &filename) const {
  auto filenames = get_open_filenames();
  return std::find(filenames.begin(), filenames.end(), filename) != filenames.end();
}

EditorCodepage NppInterface::get_encoding(EditorViewType view) const {
  auto CodepageId = static_cast<int>(send_msg_to_scintilla(view, SCI_GETCODEPAGE, 0, 0));
  if (CodepageId == SC_CP_UTF8)
    return EditorCodepage::utf8;

  return EditorCodepage::ansi;
}

void NppInterface::activate_document(int index, EditorViewType view) { send_msg_to_npp(NPPM_ACTIVATEDOC, to_index(view), index); }

void NppInterface::activate_document(const std::wstring &filepath, EditorViewType view) {
  auto fnames = get_open_filenames();
  auto it = std::find(fnames.begin(), fnames.end(), filepath);
  if (it == fnames.end()) // exception prbbly
    return;

  activate_document(static_cast<int>(it - fnames.begin()), view);
}

std::wstring NppInterface::active_document_path() const { return get_dir_msg(NPPM_GETFULLCURRENTPATH); }

void NppInterface::switch_to_file(const std::wstring &path) { send_msg_to_npp(NPPM_SWITCHTOFILE, 0, reinterpret_cast<LPARAM>(path.data())); }

std::wstring NppInterface::active_file_directory() const { return get_dir_msg(NPPM_GETCURRENTDIRECTORY); }

void NppInterface::do_command(int id) { send_msg_to_npp(WM_COMMAND, id); }

int NppInterface::get_text_height(EditorViewType view, int line) const { return static_cast<int>(send_msg_to_scintilla(view, SCI_TEXTHEIGHT, line)); }

int NppInterface::get_point_x_from_position(EditorViewType view, TextPosition position) const {
  return static_cast<int>(send_msg_to_scintilla(view, SCI_POINTXFROMPOSITION, 0, position));
}

int NppInterface::get_point_y_from_position(EditorViewType view, TextPosition position) const {
  return static_cast<int>(send_msg_to_scintilla(view, SCI_POINTYFROMPOSITION, 0, position));
}

bool NppInterface::is_line_visible(EditorViewType view, TextPosition line) const {
  return send_msg_to_scintilla(view, SCI_GETLINEVISIBLE, line);
}

TextPosition NppInterface::find_next(EditorViewType view, TextPosition from_position, const char* needle) 
{
  Sci_TextToFind ttf;
  ttf.chrg.cpMin = from_position;
  ttf.chrg.cpMax = get_active_document_length(view);
  ttf.lpstrText = needle;
  return static_cast<TextPosition> (send_msg_to_scintilla(view, SCI_FINDTEXT, 0, reinterpret_cast<LPARAM> (&ttf)));
}

void NppInterface::replace_text(EditorViewType view, TextPosition from, TextPosition to, std::string_view replacement) {
  send_msg_to_scintilla (view, SCI_SETTARGETSTART, from);
  send_msg_to_scintilla (view, SCI_SETTARGETEND, to);
  send_msg_to_scintilla (view, SCI_REPLACETARGET, replacement.size(), reinterpret_cast<LPARAM> (replacement.data()));
}

void NppInterface::set_selection(EditorViewType view, TextPosition from, TextPosition to) { post_msg_to_scintilla(view, SCI_SETSEL, from, to); }

void NppInterface::replace_selection(EditorViewType view, const char *str) { send_msg_to_scintilla(view, SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(str)); }

void NppInterface::set_indicator_style(EditorViewType view, int indicator_index, int style) {
  send_msg_to_scintilla(view, SCI_INDICSETSTYLE, indicator_index, style);
}

void NppInterface::set_indicator_foreground(EditorViewType view, int indicator_index, int style) {
  send_msg_to_scintilla(view, SCI_INDICSETFORE, indicator_index, style);
}

void NppInterface::set_current_indicator(EditorViewType view, int indicator_index) { post_msg_to_scintilla(view, SCI_SETINDICATORCURRENT, indicator_index); }

void NppInterface::indicator_fill_range(EditorViewType view, TextPosition from, TextPosition to) { post_msg_to_scintilla(view, SCI_INDICATORFILLRANGE, from, to - from); }

void NppInterface::indicator_clear_range(EditorViewType view, TextPosition from, TextPosition to) { post_msg_to_scintilla(view, SCI_INDICATORCLEARRANGE, from, to - from); }

TextPosition NppInterface::get_first_visible_line(EditorViewType view) const { return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_GETFIRSTVISIBLELINE)); }

TextPosition NppInterface::get_lines_on_screen(EditorViewType view) const { return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_LINESONSCREEN)); }

TextPosition NppInterface::get_document_line_from_visible(EditorViewType view, TextPosition visible_line) const {
  return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_DOCLINEFROMVISIBLE, visible_line));
}

TextPosition NppInterface::get_document_line_count(EditorViewType view) const { return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_GETLINECOUNT)); }

std::string NppInterface::get_active_document_text(EditorViewType view) const {
  auto buf_len = get_active_document_length(view) + 1;
  std::vector<char> buf(buf_len);
  send_msg_to_scintilla(view, SCI_GETTEXT, buf_len, reinterpret_cast<LPARAM>(buf.data()));
  return buf.data();
}

std::wstring NppInterface::get_full_current_path() const {
  std::vector<wchar_t> full_path(MAX_PATH);
  send_msg_to_npp(NPPM_GETFULLCURRENTPATH, MAX_PATH, reinterpret_cast<LPARAM>(full_path.data()));
  return full_path.data();
}

RECT NppInterface::editor_rect(EditorViewType view) const {
  RECT out;
  GetWindowRect(get_scintilla_hwnd(view), &out);
  OffsetRect(&out, -out.left, -out.top);
  return out;
}

void NppInterface::delete_range(EditorViewType view, TextPosition start, TextPosition length) {
  send_msg_to_scintilla (view, SCI_DELETERANGE, start, length);
}

void NppInterface::begin_undo_action(EditorViewType view) {
  send_msg_to_scintilla (view, SCI_BEGINUNDOACTION);
}

void NppInterface::end_undo_action(EditorViewType view) {
  send_msg_to_scintilla (view, SCI_ENDUNDOACTION);
}

void NppInterface::undo(EditorViewType view) {
  send_msg_to_scintilla (view, SCI_UNDO);
}

std::optional<TextPosition> NppInterface::char_position_from_global_point(EditorViewType view, int x, int y) const {
  POINT p;
  p.x = x;
  p.y = y;
  auto hwnd = get_scintilla_hwnd(view);
  if (WindowFromPoint(p) != hwnd)
    return std::nullopt;
  ScreenToClient(hwnd, &p);
  return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y));
}

TextPosition NppInterface::char_position_from_point(EditorViewType view, const POINT &pnt) const {
  return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_CHARPOSITIONFROMPOINT, pnt.x, pnt.y));
}

TextPosition NppInterface::get_selection_start(EditorViewType view) const { return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_GETSELECTIONSTART)); }

TextPosition NppInterface::get_selection_end(EditorViewType view) const { return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_GETSELECTIONEND)); }

TextPosition NppInterface::get_line_length(EditorViewType view, int line) const { return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_LINELENGTH, line)); }

std::string NppInterface::get_line(EditorViewType view, TextPosition line_number) const {
  auto buf_size = static_cast<int>(send_msg_to_scintilla(view, SCI_LINELENGTH, line_number) + 1);
  std::vector<char> buf(buf_size);
  auto ret = static_cast<int>(send_msg_to_scintilla(view, SCI_GETLINE, line_number, reinterpret_cast<LPARAM>(buf.data())));
  static_cast<void>(ret);
  assert(ret == buf_size - 1);
  return buf.data();
}

void NppInterface::move_active_document_to_other_view() { do_command(IDM_VIEW_GOTO_ANOTHER_VIEW); }

void NppInterface::add_toolbar_icon(int cmdId, const toolbarIcons *toolBarIconsPtr) {
  send_msg_to_npp(NPPM_ADDTOOLBARICON, static_cast<WPARAM>(cmdId), reinterpret_cast<LPARAM>(toolBarIconsPtr));
}

std::string NppInterface::selected_text(EditorViewType view) const {
  auto sel_buf_size = static_cast<int>(send_msg_to_scintilla(view, SCI_GETSELTEXT, 0, 0));
  if (sel_buf_size > 1) // Because it includes terminating '\0'
  {
    std::vector<char> buf(sel_buf_size);
    send_msg_to_scintilla(view, SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(buf.data()));
    return buf.data();
  }
  return {};
}

std::string NppInterface::get_current_line(EditorViewType view) const { return get_line(view, get_current_line_number(view)); }

int NppInterface::get_current_pos(EditorViewType view) const { return static_cast<int>(send_msg_to_scintilla(view, SCI_GETCURRENTPOS, 0, 0)); }

int NppInterface::get_current_line_number(EditorViewType view) const { return line_from_position(view, get_current_pos(view)); }

int NppInterface::line_from_position(EditorViewType view, int position) const {
  return static_cast<int>(send_msg_to_scintilla(view, SCI_LINEFROMPOSITION, position));
}

std::wstring NppInterface::plugin_config_dir() const { return get_dir_msg(NPPM_GETPLUGINSCONFIGDIR); }

TextPosition NppInterface::get_line_start_position(EditorViewType view, int line) const {
  return static_cast<int>(send_msg_to_scintilla(view, SCI_POSITIONFROMLINE, line));
}

TextPosition NppInterface::get_line_end_position(EditorViewType view, int line) const {
  return static_cast<int>(send_msg_to_scintilla(view, SCI_GETLINEENDPOSITION, line));
}

HWND NppInterface::get_editor_handle() const { return m_npp_data.npp_handle; }

void NppInterface::notify(SCNotification *notify_code) {
  switch (notify_code->nmhdr.code) {
  case NPPN_LANGCHANGED: {
    for (auto &val : m_lexer_cache)
      val = std::nullopt;
  } break;
  case NPPN_BUFFERACTIVATED: {
    for (auto &val : m_lexer_cache)
      val = std::nullopt;
  } break;
  }
}

int NppInterface::get_lexer(EditorViewType view) const {
  if (m_lexer_cache[view])
    return *m_lexer_cache[view];
  return *(m_lexer_cache[view] = static_cast<int>(send_msg_to_scintilla(view, SCI_GETLEXER)));
}

int NppInterface::get_style_at(EditorViewType view, TextPosition position) const { return static_cast<int>(send_msg_to_scintilla(view, SCI_GETSTYLEAT, position)); }

bool NppInterface::is_style_hotspot(EditorViewType view, int style) const {
  // TODO: implement cache
  return send_msg_to_scintilla(view, SCI_STYLEGETHOTSPOT, style) != 0;
}

TextPosition NppInterface::get_active_document_length(EditorViewType view) const { return static_cast<TextPosition>(send_msg_to_scintilla(view, SCI_GETLENGTH)); }

std::string NppInterface::get_text_range(EditorViewType view, TextPosition from, TextPosition to) const {
  if (from > to) {
    assert (false); // Incorrect request to Scintilla. Prevent possible crash.
    return {};
  }
  Sci_TextRange range;
  range.chrg.cpMin = from;
  range.chrg.cpMax = to;
  std::vector<char> buf(range.chrg.cpMax - range.chrg.cpMin + 1);
  range.lpstrText = buf.data();
  send_msg_to_scintilla(view, SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&range));
  return buf.data();
}

void NppInterface::force_style_update(EditorViewType view, TextPosition from, TextPosition to) { send_msg_to_scintilla(view, SCI_COLOURISE, from, to); }

std::vector<std::wstring> NppInterface::get_open_filenames(std::optional<EditorViewType> view) const {
  int enum_val = -1;

  if (!view)
    enum_val = ALL_OPEN_FILES;
  else
    switch (*view) {
    case EditorViewType::primary:
      enum_val = PRIMARY_VIEW;
      break;
    case EditorViewType::secondary:
      enum_val = SECOND_VIEW;
      break;
    case EditorViewType::COUNT:
      break;
    }

  ASSERT_RETURN(enum_val >= 0, {});
  auto count = static_cast<int>(send_msg_to_npp(NPPM_GETNBOPENFILES, 0, enum_val));

  auto paths = new wchar_t *[count];
  for (int i = 0; i < count; ++i)
    paths[i] = new wchar_t[MAX_PATH];

  int msg = -1;
  if (!view)
    msg = NPPM_GETOPENFILENAMES;
  else
    switch (*view) {
    case EditorViewType::primary:
      msg = NPPM_GETOPENFILENAMESPRIMARY;
      break;
    case EditorViewType::secondary:
      msg = NPPM_GETOPENFILENAMESSECOND;
      break;
    case EditorViewType::COUNT:
      break;
    default:;
    }

  ASSERT_RETURN(msg >= 0, {});
  {
    auto ret = send_msg_to_npp(msg, reinterpret_cast<WPARAM>(paths), count);
    ASSERT_RETURN(ret == count, {});
  }

  std::vector<std::wstring> res;
  for (int i = 0; i < count; ++i) {
    res.emplace_back(paths[i]);
    delete[] paths[i];
  }
  delete[] paths;
  return res;
}
