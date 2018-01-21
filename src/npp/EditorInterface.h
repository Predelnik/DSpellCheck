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
#include <cassert>
#include <string>
#include <vector>
#include <optional>
#include <windows.h>

// To mock things and stuff

struct toolbarIcons;

enum class EditorViewType
{
    primary,
    secondary,

    COUNT,
};

enum class EditorCodepage
{
    ansi,
    utf8,

    COUNT,
};

class EditorRect
{
public:
    int x; int y;
    int w; int h;
};

class EditorInterface
{
public:
    static EditorViewType other_view(EditorViewType view)
    {
        switch (view)
        {
        case EditorViewType::primary: return EditorViewType::secondary;
        case EditorViewType::secondary: return EditorViewType::primary;
        case EditorViewType::COUNT: break;
        }
        assert(false);
        return EditorViewType::primary;
    }

    // non-const
    virtual bool open_document(std::wstring filename) = 0;
    virtual void activate_document(int index, EditorViewType view) = 0;
    virtual void activate_document(const std::wstring& filepath, EditorViewType view) = 0;
    virtual void switch_to_file(const std::wstring& path) = 0;
    virtual void move_active_document_to_other_view() = 0;
    virtual void add_toolbar_icon(int cmd_id, const toolbarIcons* tool_bar_icons_ptr) = 0;
    virtual void force_style_update (EditorViewType view, long from, long to) = 0;
    virtual void set_selection (EditorViewType view, long from, long to) = 0;
    void set_cursor_pos (EditorViewType view, long cursor_pos)
    {
        set_selection (view, cursor_pos, cursor_pos);
    }

    virtual void replace_selection(EditorViewType view, const char *str) = 0;
    virtual void set_indicator_style (EditorViewType view, int indicator_index, int style) = 0;
    virtual void set_indicator_foreground (EditorViewType view, int indicator_index, int style) = 0;
    virtual void set_current_indicator (EditorViewType view, int indicator_index) = 0;
    virtual void indicator_fill_range (EditorViewType view, long from, long to) = 0;
    virtual void indicator_clear_range (EditorViewType view, long from, long to) = 0;

    // const
    virtual std::vector<std::wstring> get_open_filenames(std::optional<EditorViewType> view = {}) const = 0;
    virtual EditorViewType active_view() const = 0;
    virtual bool is_opened(const std::wstring& filename) const = 0;
    virtual EditorCodepage get_encoding(EditorViewType view) const = 0;
    virtual std::wstring active_document_path() const = 0;
    virtual std::wstring active_file_directory() const = 0;
    virtual std::wstring plugin_config_dir() const = 0;
    virtual std::string selected_text(EditorViewType view) const = 0;
    virtual std::string get_current_line(EditorViewType view) const = 0;
    virtual std::string get_line(EditorViewType view, long line_number) const = 0;
    virtual int get_current_pos(EditorViewType view) const = 0;
    virtual int get_current_line_number(EditorViewType view) const = 0;
    virtual int get_text_height (EditorViewType view, int line) const = 0;
    virtual int line_from_position(EditorViewType view, int position) const = 0;
    virtual long get_line_start_position(EditorViewType view, int line) const = 0;
    virtual long get_line_end_position(EditorViewType view, int line) const = 0;
    virtual int get_lexer (EditorViewType view) const = 0;
    virtual std::optional<long> char_position_from_point(EditorViewType view, int x, int y) const = 0;
    virtual long get_selection_start (EditorViewType view) const = 0;
    virtual long get_selection_end (EditorViewType view) const = 0;
    virtual HWND get_editor_handle() const = 0 ;
    virtual int get_style_at (EditorViewType view, long position) const = 0;
    virtual std::wstring get_full_current_path () const = 0;
    // is current style used for links (hotspots):
    virtual bool is_style_hotspot (EditorViewType view, int style) const = 0;
    virtual long get_active_document_length (EditorViewType view) const = 0;
    virtual std::string get_text_range (EditorViewType view, long from, long to) const = 0;
    virtual long get_line_length(EditorViewType view, int line) const = 0;
    virtual int get_point_x_from_position (EditorViewType view, long position) const = 0;
    virtual int get_point_y_from_position (EditorViewType view, long position) const = 0;
    virtual long get_first_visible_line (EditorViewType view) const = 0;
    virtual long get_lines_on_screen (EditorViewType view) const = 0;
    virtual long get_document_line_from_visible (EditorViewType view, long visible_line) const = 0;
    virtual long get_document_line_count (EditorViewType view) const = 0;
    virtual std::string get_active_document_text(EditorViewType view) const = 0;

    int get_current_pos_in_line(EditorViewType view) const
    {
        return get_current_pos(view) - get_line_start_position(view, get_current_line_number(view));
    }
    virtual ~EditorInterface () = default;
};

