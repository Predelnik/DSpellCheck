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

#define NOMINMAX
#include "common/Utility.h"
#include "plugin/Constants.h"

#include <optional>
#include <string>
#include <vector>
#include <Windows.h>

// To mock things and stuff

struct toolbarIconsWithDarkMode;

enum class EditorCodepage {
  ansi,
  utf8,

  COUNT,
};

class EditorRect {
public:
  int x;
  int y;
  int w;
  int h;
};

class EditorInterface {
public:
  // non-const
  virtual bool open_document(std::wstring filename) = 0;
  virtual void activate_document(int index) = 0;
  virtual void activate_document(const std::wstring &filepath) = 0;
  virtual void switch_to_file(const std::wstring &path) = 0;
  virtual void move_active_document_to_other_view() = 0;
  virtual void add_toolbar_icon(int cmd_id, const toolbarIconsWithDarkMode *tool_bar_icons_ptr) = 0;
  virtual void force_style_update(TextPosition from, TextPosition to) = 0;
  virtual void set_selection(TextPosition from, TextPosition to) = 0;
  virtual TextPosition find_next(TextPosition from_position, const char *needle) = 0;

  void set_cursor_pos(TextPosition cursor_pos) {
    set_selection(cursor_pos, cursor_pos);
  }

  virtual void replace_selection(const char *str) = 0;
  virtual void delete_range(TextPosition start, TextPosition length) = 0;
  virtual void set_indicator_style(int indicator_index,
                                   int style) = 0;
  virtual void set_indicator_foreground(
      int indicator_index, int style) = 0;
  virtual void set_current_indicator(
      int indicator_index) = 0;
  virtual void indicator_fill_range(TextPosition from,
                                    TextPosition to) = 0;
  virtual void indicator_clear_range(TextPosition from,
                                     TextPosition to) = 0;
  virtual void undo() = 0;
  virtual void replace_text(TextPosition from, TextPosition to, std::string_view replacement) = 0;
  virtual void add_bookmark(TextPosition line) = 0;

  // const
  virtual std::vector<std::wstring> get_open_filenames() const = 0;
  virtual std::vector<std::wstring> get_open_filenames_all_views() const = 0;
  virtual bool is_opened(const std::wstring &filename) const = 0;
  virtual EditorCodepage get_encoding() const = 0;
  virtual std::wstring active_document_path() const = 0;
  virtual std::wstring active_file_directory() const = 0;
  virtual std::wstring plugin_config_dir() const = 0;
  virtual std::string selected_text() const = 0;
  virtual std::string get_current_line() const = 0;
  virtual std::string get_line(TextPosition line_number) const = 0;
  virtual TextPosition get_current_pos() const = 0;
  virtual int get_current_line_number() const = 0;
  virtual int get_text_height(int line) const = 0;
  virtual int line_from_position(TextPosition position) const = 0;
  virtual TextPosition get_line_start_position(TextPosition line) const = 0;
  virtual TextPosition get_line_end_position(TextPosition line) const = 0;
  virtual int get_lexer() const = 0;
  virtual std::optional<TextPosition>
  char_position_from_global_point(int x, int y) const = 0;
  virtual TextPosition char_position_from_point(
      const POINT &pnt) const = 0;
  virtual TextPosition get_selection_start() const = 0;
  virtual TextPosition get_selection_end() const = 0;
  virtual HWND get_editor_hwnd() const = 0;
  virtual HWND get_view_hwnd() const = 0;
  virtual int get_style_at(TextPosition position) const = 0;
  virtual int get_indicator_value_at(int indicator_id, TextPosition position) const = 0;
  virtual std::wstring get_full_current_path() const = 0;
  // is current style used for links (hotspots):
  virtual TextPosition get_active_document_length() const = 0;
  virtual std::string get_text_range(TextPosition from,
                                     TextPosition to) const = 0;
  virtual TextPosition get_line_length(int line) const = 0;
  virtual int get_point_x_from_position(
      TextPosition position) const = 0;
  virtual int get_point_y_from_position(
      TextPosition position) const = 0;
  POINT get_point_from_position(TextPosition position) const;
  virtual TextPosition get_first_visible_line() const = 0;
  virtual bool is_line_visible(TextPosition line) const = 0;
  virtual TextPosition get_lines_on_screen() const = 0;
  virtual TextPosition get_document_line_from_visible(
      TextPosition visible_line) const = 0;
  virtual TextPosition get_document_line_count() const = 0;
  virtual std::string get_active_document_text() const = 0;
  virtual RECT editor_rect() const = 0;

  virtual int get_view_count() const = 0;

  // winapi function moved here for easier testing
  virtual std::optional<POINT> get_mouse_cursor_pos() const = 0;

  TextPosition get_prev_valid_begin_pos(TextPosition pos) const;
  TextPosition get_next_valid_end_pos(TextPosition pos) const;
  MappedWstring to_mapped_wstring(const std::string &str);
  MappedWstring get_mapped_wstring_range(TextPosition from, TextPosition to);
  MappedWstring get_mapped_wstring_line(TextPosition line);
  std::string to_editor_encoding(std::wstring_view str) const;

  virtual ~EditorInterface() = default;

private:
  virtual void begin_undo_action() = 0; // use UNDO_BLOCK instead
  virtual void end_undo_action() = 0;
  virtual void set_target_view(int view_index) const = 0;
  virtual int get_target_view() const = 0;
  virtual int active_view() const = 0;

  friend class undo_block;
  friend class target_view_block;
};

class undo_block {
public:
  undo_block(EditorInterface &editor)
    : m_editor(editor) { m_editor.begin_undo_action(); }

  ~undo_block() { m_editor.end_undo_action(); }

private:
  EditorInterface &m_editor;
};

#define UNDO_BLOCK(...) undo_block anonymous_undo_block_ ## __COUNTER__ {__VA_ARGS__}

class target_view_block {
  using self_t = target_view_block;
public:
  target_view_block(const EditorInterface &editor, int view)
    : m_editor(editor) {
    m_original_view = m_editor.get_target_view();
    m_editor.set_target_view(view);
  }

  static self_t active_view(const EditorInterface &editor) {
    return {editor, editor.active_view()};
  }

  ~target_view_block() {
    m_editor.set_target_view(m_original_view);
  }

private:
  const EditorInterface &m_editor;
  int m_original_view;
};

#define TARGET_VIEW_BLOCK(...) target_view_block CONCATENATE(anonymous_target_view_block_, __COUNTER__) {__VA_ARGS__}
#define ACTIVE_VIEW_BLOCK(...) auto CONCATENATE(anonymous_target_view_block_, __COUNTER__) = target_view_block::active_view (__VA_ARGS__)
