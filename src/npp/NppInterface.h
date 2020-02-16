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

#pragma once
#include "npp/EditorInterface.h"

#include "utils/enum_array.h"
#include <vector>

struct SCNotification;
class NppData;

class NppInterface : public EditorInterface {
public:
  explicit NppInterface(const NppData *nppData);
  std::wstring get_npp_directory();
  bool is_allocate_cmdid_supported() const;
  int allocate_cmdid(int start_number);
  void set_menu_item_check(int cmd_id, bool checked);

  static int to_index(NppViewType target);

  // non-const
  bool open_document(std::wstring filename) override;
  void activate_document(int index) override;
  void activate_document(const std::wstring &filepath) override;
  void switch_to_file(const std::wstring &path) override;
  void move_active_document_to_other_view() override;
  void add_toolbar_icon(int cmdId, const toolbarIcons *toolBarIconsPtr) override;
  void set_selection(TextPosition from, TextPosition to) override;
  void replace_selection(const char *str) override;
  void set_indicator_style(int indicator_index, int style) override;
  void set_indicator_foreground(int indicator_index, int style) override;
  void set_current_indicator(int indicator_index) override;
  void indicator_fill_range(TextPosition from, TextPosition to) override;
  void indicator_clear_range(TextPosition from, TextPosition to) override;
  void delete_range(TextPosition start, TextPosition length) override;
  void begin_undo_action(NppViewType view) override;
  void end_undo_action(NppViewType view) override;
  void undo() override;

  // const
  std::vector<std::wstring> get_open_filenames() const override;
  std::vector<std::wstring> get_open_filenames_all_views() const override;
  NppViewType active_view() const override;
  bool is_opened(const std::wstring &filename) const override;
  EditorCodepage get_encoding() const override;
  std::wstring active_document_path() const override;
  std::wstring active_file_directory() const override;
  std::string selected_text(NppViewType view) const override;
  std::string get_current_line(NppViewType view) const override;
  TextPosition get_current_pos(NppViewType view) const override;
  int get_current_line_number(NppViewType view) const override;
  int line_from_position(NppViewType view, TextPosition position) const override;
  std::wstring plugin_config_dir() const override;
  TextPosition get_line_start_position(NppViewType view, TextPosition line) const override;
  TextPosition get_line_end_position(NppViewType view, TextPosition line) const override;
  HWND get_editor_hwnd() const override;
  // please hook it up to notify to simplify usage of certain caches
  void notify(SCNotification *notify_code);
  int get_lexer(NppViewType view) const override;
  int get_style_at(NppViewType view, TextPosition position) const override;
  bool is_style_hotspot(NppViewType view, int style) const override;
  TextPosition get_active_document_length(NppViewType view) const override;
  std::string get_text_range(NppViewType view, TextPosition from, TextPosition to) const override;
  void force_style_update(TextPosition from, TextPosition to) override;
  std::optional<TextPosition> char_position_from_global_point(NppViewType view, int x, int y) const override;
  TextPosition char_position_from_point(NppViewType view, const POINT &pnt) const override;
  TextPosition get_selection_start(NppViewType view) const override;
  TextPosition get_selection_end(NppViewType view) const override;
  TextPosition get_line_length(NppViewType view, int line) const override;
  std::string get_line(NppViewType view, TextPosition line_number) const override;
  TextPosition get_first_visible_line(NppViewType view) const override;
  TextPosition get_lines_on_screen(NppViewType view) const override;
  TextPosition get_document_line_from_visible(NppViewType view, TextPosition visible_line) const override;
  TextPosition get_document_line_count(NppViewType view) const override;
  std::string get_active_document_text(NppViewType view) const override;
  std::wstring get_full_current_path() const override;
  RECT editor_rect(NppViewType view) const override;
  int get_text_height(NppViewType view, int line) const override;
  int get_point_x_from_position(NppViewType view, TextPosition position) const override;
  int get_point_y_from_position(NppViewType view, TextPosition position) const override;
  bool is_line_visible(NppViewType view, TextPosition line) const override;
  TextPosition find_next(TextPosition from_position, const char* needle) override;
  void replace_text(TextPosition from, TextPosition to, std::string_view replacement) override;
  void add_bookmark(TextPosition line) override;

  int get_view_count() const override;
  void set_target_view(int view_index) const override;

  HMENU get_menu_handle(int menu_type) const;
  int get_target_view() const override;

public:
  HWND get_scintilla_hwnd(NppViewType view) const;

private:
  LRESULT send_msg_to_npp(UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0) const;
  LRESULT send_msg_to_scintilla(NppViewType view, UINT msg, WPARAM w_param = 0, LPARAM l_param = 0) const;
  void post_msg_to_scintilla(NppViewType view, UINT msg, WPARAM w_param = 0, LPARAM l_param = 0) const;
  std::wstring get_dir_msg(UINT msg) const;
  void do_command(int id);
  std::vector<std::wstring> get_open_filenames_helper(int enum_val, int msg) const;

private:
  const NppData &m_npp_data;
  mutable enum_array<NppViewType, std::optional<int>> m_lexer_cache;
  mutable std::array<std::optional<bool>, 256> m_hotspot_cache;
  mutable NppViewType m_target_view = NppViewType::primary;
};
