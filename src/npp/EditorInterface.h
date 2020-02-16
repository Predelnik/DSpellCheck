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
#include <cassert>
#include <optional>
#include <string>
#include <vector>
#include <windows.h>
#include "CommonFunctions.h"
#include "MainDefs.h"

// To mock things and stuff

struct toolbarIcons;

enum class NppViewType {
  primary,
  secondary,

  COUNT,
};

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
  static NppViewType other_view(NppViewType view) {
    switch (view) {
    case NppViewType::primary:
      return NppViewType::secondary;
    case NppViewType::secondary:
      return NppViewType::primary;
    case NppViewType::COUNT:
      break;
    }
    assert(false);
    return NppViewType::primary;
  }

  // non-const
  virtual bool open_document(std::wstring filename) = 0;
  virtual void activate_document(int index) = 0;
  virtual void activate_document(const std::wstring &filepath) = 0;
  virtual void switch_to_file(const std::wstring &path) = 0;
  virtual void move_active_document_to_other_view() = 0;
  virtual void add_toolbar_icon(int cmd_id,
                                const toolbarIcons *tool_bar_icons_ptr) = 0;
  virtual void force_style_update(TextPosition from, TextPosition to) = 0;
  virtual void set_selection(TextPosition from, TextPosition to) = 0;
  virtual TextPosition find_next(NppViewType view, TextPosition from_position, const char* needle) = 0;
  void set_cursor_pos(NppViewType view, TextPosition cursor_pos) {
    set_target_view (static_cast<int> (view));
    set_selection(cursor_pos, cursor_pos);
  }

  virtual void replace_selection(NppViewType view, const char *str) = 0;
  virtual void delete_range (NppViewType view, TextPosition start, TextPosition length) = 0;
  virtual void set_indicator_style(NppViewType view, int indicator_index,
                                   int style) = 0;
  virtual void set_indicator_foreground(NppViewType view,
                                        int indicator_index, int style) = 0;
  virtual void set_current_indicator(NppViewType view,
                                     int indicator_index) = 0;
  virtual void indicator_fill_range(NppViewType view, TextPosition from,
                                    TextPosition to) = 0;
  virtual void indicator_clear_range(NppViewType view, TextPosition from,
                                     TextPosition to) = 0;
  virtual void undo(NppViewType view) = 0;
  virtual void replace_text(NppViewType view, TextPosition from, TextPosition to, std::string_view replacement) = 0;
  virtual void add_bookmark (NppViewType view, TextPosition line) = 0;

  // const
  virtual std::vector<std::wstring>
  get_open_filenames(std::optional<NppViewType> view = {}) const = 0;
  virtual bool is_opened(const std::wstring &filename) const = 0;
  virtual EditorCodepage get_encoding(NppViewType view) const = 0;
  virtual std::wstring active_document_path() const = 0;
  virtual std::wstring active_file_directory() const = 0;
  virtual std::wstring plugin_config_dir() const = 0;
  virtual std::string selected_text(NppViewType view) const = 0;
  virtual std::string get_current_line(NppViewType view) const = 0;
  virtual std::string get_line(NppViewType view, TextPosition line_number) const = 0;
  virtual TextPosition get_current_pos(NppViewType view) const = 0;
  virtual int get_current_line_number(NppViewType view) const = 0;
  virtual int get_text_height(NppViewType view, int line) const = 0;
  virtual int line_from_position(NppViewType view, TextPosition position) const = 0;
  virtual TextPosition get_line_start_position(NppViewType view, TextPosition line) const = 0;
  virtual TextPosition get_line_end_position(NppViewType view, TextPosition line) const = 0;
  virtual int get_lexer(NppViewType view) const = 0;
  virtual std::optional<TextPosition>
  char_position_from_global_point(NppViewType view, int x, int y) const = 0;
  virtual TextPosition char_position_from_point(NppViewType view,
                                        const POINT &pnt) const = 0;
  virtual TextPosition get_selection_start(NppViewType view) const = 0;
  virtual TextPosition get_selection_end(NppViewType view) const = 0;
  virtual HWND get_editor_hwnd() const = 0;
  virtual int get_style_at(NppViewType view, TextPosition position) const = 0;
  virtual std::wstring get_full_current_path() const = 0;
  // is current style used for links (hotspots):
  virtual bool is_style_hotspot(NppViewType view, int style) const = 0;
  virtual TextPosition get_active_document_length(NppViewType view) const = 0;
  virtual std::string get_text_range(NppViewType view, TextPosition from,
                                     TextPosition to) const = 0;
  virtual TextPosition get_line_length(NppViewType view, int line) const = 0;
  virtual int get_point_x_from_position(NppViewType view,
                                        TextPosition position) const = 0;
  virtual int get_point_y_from_position(NppViewType view,
                                        TextPosition position) const = 0;
  POINT get_point_from_position(NppViewType view, TextPosition position) const {
    return {get_point_x_from_position(view, position),
            get_point_y_from_position(view, position)};
  }
  virtual TextPosition get_first_visible_line(NppViewType view) const = 0;
  virtual bool is_line_visible(NppViewType view, TextPosition line) const = 0;
  virtual TextPosition get_lines_on_screen(NppViewType view) const = 0;
  virtual TextPosition get_document_line_from_visible(NppViewType view,
                                              TextPosition visible_line) const = 0;
  virtual TextPosition get_document_line_count(NppViewType view) const = 0;
  virtual std::string get_active_document_text(NppViewType view) const = 0;
  virtual RECT editor_rect(NppViewType view) const = 0;

  TextPosition get_current_pos_in_line(NppViewType view) const {
    return get_current_pos(view) -
           get_line_start_position(view, get_current_line_number(view));
  }

  virtual int get_view_count () const = 0;
  virtual void set_target_view (int view_index) = 0;
  virtual void target_active_view ();
  virtual NppViewType active_view() const = 0;

  TextPosition get_prev_valid_begin_pos(NppViewType view, TextPosition pos) const;
  TextPosition get_next_valid_end_pos(NppViewType view, TextPosition pos) const;
  MappedWstring to_mapped_wstring (NppViewType view, const std::string &str);
  MappedWstring get_mapped_wstring_range (NppViewType view, TextPosition from, TextPosition to);
  MappedWstring get_mapped_wstring_line (NppViewType view, TextPosition line);
  std::string to_editor_encoding (NppViewType view, std::wstring_view str) const;

  virtual ~EditorInterface() = default;

private:
  virtual void begin_undo_action (NppViewType view) = 0; // use UNDO_BLOCK instead
  virtual void end_undo_action (NppViewType view) = 0;

  friend class undo_block;
};

class undo_block
{
public:
  undo_block (EditorInterface &editor, NppViewType view) : m_editor(editor), m_view(view) { m_editor.begin_undo_action(m_view); }
  ~undo_block () { m_editor.end_undo_action(m_view); }

private:
  EditorInterface &m_editor;
  NppViewType m_view;
};

#define UNDO_BLOCK(...) undo_block anonymous_undo_block_ ## __COUNTER__ {__VA_ARGS__}
