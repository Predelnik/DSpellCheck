// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
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

#pragma once
#include "npp/EditorInterface.h"

#include "utils/enum_array.h"
#include <vector>

struct SCNotification;
struct NppData;

class NppInterface : public EditorInterface {
public:
  explicit NppInterface(const NppData *nppData);
  std::wstring get_npp_directory();
  bool is_allocate_cmdid_supported() const;
  int allocate_cmdid(int start_number);
  void set_menu_item_check(int cmd_id, bool checked);

  static int to_index(EditorViewType target);

  // non-const
  bool open_document(std::wstring filename) override;
  void activate_document(int index, EditorViewType view) override;
  void activate_document(const std::wstring &filepath, EditorViewType view) override;
  void switch_to_file(const std::wstring &path) override;
  void move_active_document_to_other_view() override;
  void add_toolbar_icon(int cmdId, const toolbarIcons *toolBarIconsPtr) override;
  void set_selection(EditorViewType view, long from, long to) override;
  void replace_selection(EditorViewType view, const char *str) override;
  void set_indicator_style(EditorViewType view, int indicator_index, int style) override;
  void set_indicator_foreground(EditorViewType view, int indicator_index, int style) override;
  void set_current_indicator(EditorViewType view, int indicator_index) override;
  void indicator_fill_range(EditorViewType view, long from, long to) override;
  void indicator_clear_range(EditorViewType view, long from, long to) override;
  void delete_range(EditorViewType view, long start, long length) override;
  void begin_undo_action(EditorViewType view) override;
  void end_undo_action(EditorViewType view) override;
  void undo(EditorViewType view) override;

  // const
  std::vector<std::wstring> get_open_filenames(std::optional<EditorViewType> view = {}) const override;
  EditorViewType active_view() const override;
  bool is_opened(const std::wstring &filename) const override;
  EditorCodepage get_encoding(EditorViewType view) const override;
  std::wstring active_document_path() const override;
  std::wstring active_file_directory() const override;
  std::string selected_text(EditorViewType view) const override;
  std::string get_current_line(EditorViewType view) const override;
  int get_current_pos(EditorViewType view) const override;
  int get_current_line_number(EditorViewType view) const override;
  int line_from_position(EditorViewType view, int position) const override;
  std::wstring plugin_config_dir() const override;
  long get_line_start_position(EditorViewType view, int line) const override;
  long get_line_end_position(EditorViewType view, int line) const override;
  HWND get_editor_handle() const override;
  // please hook it up to notify to simplify usage of certain caches
  void notify(SCNotification *notify_code);
  int get_lexer(EditorViewType view) const override;
  int get_style_at(EditorViewType view, long position) const override;
  bool is_style_hotspot(EditorViewType view, int style) const override;
  long get_active_document_length(EditorViewType view) const override;
  std::string get_text_range(EditorViewType view, long from, long to) const override;
  void force_style_update(EditorViewType view, long from, long to) override;
  std::optional<long> char_position_from_global_point(EditorViewType view, int x, int y) const override;
  long char_position_from_point(EditorViewType view, const POINT &pnt) const override;
  long get_selection_start(EditorViewType view) const override;
  long get_selection_end(EditorViewType view) const override;
  long get_line_length(EditorViewType view, int line) const override;
  std::string get_line(EditorViewType view, long line_number) const override;
  long get_first_visible_line(EditorViewType view) const override;
  long get_lines_on_screen(EditorViewType view) const override;
  long get_document_line_from_visible(EditorViewType view, long visible_line) const override;
  long get_document_line_count(EditorViewType view) const override;
  std::string get_active_document_text(EditorViewType view) const override;
  std::wstring get_full_current_path() const override;
  RECT editor_rect(EditorViewType view) const override;
  int get_text_height(EditorViewType view, int line) const override;
  int get_point_x_from_position(EditorViewType view, long position) const override;
  int get_point_y_from_position(EditorViewType view, long position) const override;
  bool is_line_visible(EditorViewType view, long line) const override;

  HMENU get_menu_handle(int menu_type) const;

public:
  HWND get_scintilla_hwnd(EditorViewType view) const;

private:
  // these functions are const per se
  LRESULT send_msg_to_npp(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) const;
  LRESULT send_msg_to_scintilla(EditorViewType view, UINT msg, WPARAM w_param = 0, LPARAM l_param = 0) const;
  void post_msg_to_scintilla(EditorViewType view, UINT msg, WPARAM w_param = 0, LPARAM l_param = 0) const;
  std::wstring get_dir_msg(UINT msg) const;
  void do_command(int id);
private:
  const NppData &m_npp_data;
  mutable enum_array<EditorViewType, std::optional<int>> m_lexer_cache;
};
