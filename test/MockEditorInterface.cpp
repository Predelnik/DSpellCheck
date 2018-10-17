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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
// USA.

#include "MockEditorInterface.h"
#include "CommonFunctions.h"
#include "utils/enum_range.h"
#include <filesystem>

void MockedDocumentInfo::set_data(const std::wstring &data_arg) {
  switch (codepage)
  {
  case EditorCodepage::ansi:
    data = to_string (data_arg);
    break;
  case EditorCodepage::utf8:
    data = to_utf8_string (data_arg);
    break;
  case EditorCodepage::COUNT: break;
  }
  style.resize(data.size());
  selection = {0, 0};
  cursor_pos = 0;
  for (auto &info : indicator_info)
    std::fill(info.set_for.begin(), info.set_for.end(), false);
}

void MockEditorInterface::move_active_document_to_other_view() {
  auto &view = m_documents[m_active_view];
  auto doc = std::move(view[m_active_document_index[m_active_view]]);
  view.erase(view.begin() + m_active_document_index[m_active_view]);
  auto other = other_view(m_active_view);
  m_documents[other].push_back(std::move(doc));
  m_active_document_index[other] =
      static_cast<int>(m_documents[other].size()) - 1;
}

void MockEditorInterface::add_toolbar_icon(
    int /*cmd_id*/, const toolbarIcons * /*tool_bar_icons_ptr*/) {}

void MockEditorInterface::force_style_update(EditorViewType /*view*/,
                                             long /*from*/, long /*to*/) {}

void MockEditorInterface::set_selection(EditorViewType view, long from,
                                        long to) {
  auto doc = active_document(view);
  if (!doc)
    return;
  doc->cursor_pos = to;
  if (from < to)
    doc->selection = {from, to};
  else
    doc->selection = {to, from};
}

void MockEditorInterface::replace_selection(EditorViewType view,
                                            const char *str) {
  auto doc = active_document(view);
  if (!doc)
    return;
  auto &d = doc->data;
  d.replace(doc->selection[0], doc->selection[1], str);
}

void MockEditorInterface::set_indicator_style(EditorViewType view,
                                              int indicator_index, int style) {
  auto doc = active_document(view);
  if (!doc)
    return;
  if (indicator_index >= static_cast<int>(doc->indicator_info.size()))
    doc->indicator_info.resize(indicator_index + 1);
  doc->indicator_info[indicator_index].style = style;
}

void MockEditorInterface::set_indicator_foreground(EditorViewType view,
                                                   int indicator_index,
                                                   int style) {
  auto doc = active_document(view);
  if (!doc)
    return;
  if (indicator_index >= static_cast<int>(doc->indicator_info.size()))
    doc->indicator_info.resize(indicator_index + 1);
  doc->indicator_info[indicator_index].foreground = style;
}

void MockEditorInterface::set_current_indicator(EditorViewType view,
                                                int indicator_index) {
  auto doc = active_document(view);
  if (!doc)
    return;
  if (indicator_index >= static_cast<int>(doc->indicator_info.size()))
    doc->indicator_info.resize(indicator_index + 1);
  doc->current_indicator = indicator_index;
}

void MockEditorInterface::indicator_fill_range(EditorViewType view, long from,
                                               long to) {
  auto doc = active_document(view);
  if (!doc)
    return;
  auto &s = doc->indicator_info[doc->current_indicator].set_for;
  if (to >= static_cast<int>(s.size()))
    s.resize(to + 1);
  std::fill(s.begin() + from, s.begin() + to, true);
}

void MockEditorInterface::indicator_clear_range(EditorViewType view, long from,
                                                long to) {
  auto doc = active_document(view);
  if (!doc)
    return;
  auto &s = doc->indicator_info[doc->current_indicator].set_for;
  if (to >= static_cast<long>(s.size()))
    s.resize(to + 1);
  std::fill(s.begin() + from, s.begin() + to, false);
}

EditorViewType MockEditorInterface::active_view() const {
  return m_active_view;
}

EditorCodepage MockEditorInterface::get_encoding(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return EditorCodepage::utf8;
  return doc->codepage;
}

int MockEditorInterface::get_current_pos(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->cursor_pos;
}

int MockEditorInterface::get_current_line_number(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return static_cast<int>(std::count(
      doc->data.begin(), doc->data.begin() + get_current_pos(view), '\n'));
}

int MockEditorInterface::get_text_height(EditorViewType /*view*/,
                                         int /*line*/) const {
  return text_height;
}

int MockEditorInterface::line_from_position(EditorViewType view,
                                            int position) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return static_cast<int>(
      std::count(doc->data.begin(), doc->data.begin() + position, '\n'));
}

long MockEditorInterface::get_line_start_position(EditorViewType view,
                                                  int line) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  size_t index = 0;
  for (int i = 0; i < line; ++i) {
    index = doc->data.find('\n', index);
    if (index == std::wstring::npos)
      return static_cast<long>(doc->data.size());

    ++index;
  }
  return static_cast<long>(index);
}

long MockEditorInterface::get_line_end_position(EditorViewType view,
                                                int line) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  size_t index = 0;
  for (int i = 0; i < line + 1; ++i) {
    index = doc->data.find('\n', index);
    if (index == std::wstring::npos)
      return static_cast<long>(doc->data.size());
    if (i != line + 1)
      ++index;
  }
  return static_cast<long>(index);
}

int MockEditorInterface::get_lexer(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return 0;
  return doc->lexer;
}

long MockEditorInterface::get_selection_start(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->selection[0];
}

long MockEditorInterface::get_selection_end(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->selection[1];
}

int MockEditorInterface::get_style_at(EditorViewType view,
                                      long position) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->style[position];
}

bool MockEditorInterface::is_style_hotspot(EditorViewType view,
                                           int style) const {
  auto doc = active_document(view);
  if (!doc)
    return false;
  return doc->hotspot_style == style;
}

long MockEditorInterface::get_active_document_length(
    EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return static_cast<long>(doc->data.length());
}

long MockEditorInterface::get_line_length(EditorViewType view, int line) const {
  size_t index = 0;
  auto doc = active_document(view);
  if (!doc)
    return -1;
  for (int i = 0; i < line; ++i) {
    index = doc->data.find('\n', index);
    if (index == std::wstring::npos)
      return -1;
    ++index;
  }
  return static_cast<long>(doc->data.find('\n', index) - index);
}

int MockEditorInterface::get_point_x_from_position(EditorViewType view,
                                                   long position) const {
  return (position -
          get_line_start_position(view, line_from_position(view, position))) *
         10;
}

int MockEditorInterface::get_point_y_from_position(EditorViewType view,
                                                   long position) const {
  auto line = line_from_position(view, position);
  return line * get_text_height(view, line);
}

long MockEditorInterface::get_first_visible_line(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->visible_lines[0];
}

long MockEditorInterface::get_lines_on_screen(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  auto &l = doc->visible_lines;
  return l[1] - l[0] + 1;
}

long MockEditorInterface::get_document_line_from_visible(
    EditorViewType view, long visible_line) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return doc->visible_lines[0] + visible_line;
}

long MockEditorInterface::get_document_line_count(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return static_cast<long>(
      std::count(doc->data.begin(), doc->data.end(), '\n'));
}

bool MockEditorInterface::open_document(std::wstring filename) {
  assert(false);
  return false;
}

void MockEditorInterface::activate_document(int index, EditorViewType view) {
  m_active_view = view;
  m_active_document_index[view] = index;
}

void MockEditorInterface::activate_document(const std::wstring &filepath,
                                            EditorViewType view) {
  auto it =
      std::find_if(m_documents[view].begin(), m_documents[view].end(),
                   [&](const auto &data) { return data.path == filepath; });
  if (it != m_documents[view].end()) {
    m_active_view = view;
    m_active_document_index[view] =
        static_cast<int>(it - m_documents[view].begin());
  }
}

void MockEditorInterface::switch_to_file(const std::wstring &path) {
  for (auto view : enum_range<EditorViewType>())
    activate_document(path, view);
}

std::vector<std::wstring> MockEditorInterface::get_open_filenames(
    std::optional<EditorViewType> view) const {
  std::vector<std::wstring> out;
  for (auto view_it : enum_range<EditorViewType>())
    if (!view || view == view_it)
      std::transform(m_documents[view_it].begin(), m_documents[view_it].end(),
                     std::back_inserter(out),
                     [](const auto &data) { return data.path; });
  return out;
}

bool MockEditorInterface::is_opened(const std::wstring &filename) const {
  for (auto view : enum_range<EditorViewType>())
    if (std::find_if(m_documents[view].begin(), m_documents[view].end(),
                     [&](const auto &data) { return data.path == filename; }) !=
        m_documents[view].end())
      return true;
  return false;
}

std::wstring MockEditorInterface::active_document_path() const {
  auto doc = active_document(m_active_view);
  if (!doc)
    return L"";
  return doc->path;
}

std::wstring MockEditorInterface::active_file_directory() const {
  auto doc = active_document(m_active_view);
  if (!doc)
    return L"";
  return std::experimental::filesystem::path(doc->path).parent_path();
}

std::wstring MockEditorInterface::plugin_config_dir() const { return L""; }

std::string MockEditorInterface::selected_text(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc || doc->selection[0] < 0 || doc->selection[1] < 0)
    return "";
  return doc->data.substr(doc->selection[0], doc->selection[1] - doc->selection[0]);
}

std::string MockEditorInterface::get_current_line(EditorViewType view) const {
  return get_line(view, get_current_line_number(view));
}

std::string MockEditorInterface::get_line(EditorViewType view,
                                          long line_number) const {
  auto start = get_line_start_position(view, line_number);
  auto end = get_line_end_position(view, line_number);
  return get_text_range(view, start, end);
}

std::optional<long> MockEditorInterface::char_position_from_global_point(
    EditorViewType /*view*/, int /*x*/, int /*y*/) const {
  return std::nullopt;
}

HWND MockEditorInterface::get_editor_handle() const { return nullptr; }

std::wstring MockEditorInterface::get_full_current_path() const {
  auto doc = active_document(m_active_view);
  if (!doc)
    return L"";
  return doc->path;
}

std::string MockEditorInterface::get_text_range(EditorViewType view, long from,
                                                long to) const {
  auto doc = active_document(view);
  if (!doc)
    return "";
  return doc->data.substr(from, to - from);
}

std::string
MockEditorInterface::get_active_document_text(EditorViewType view) const {
  auto doc = active_document(view);
  if (!doc)
    return "";
  return doc->data;
}

long MockEditorInterface::char_position_from_point(EditorViewType view,
                                                   const POINT &pnt) const {
  auto doc = active_document(view);
  if (!doc)
    return -1;
  return get_line_start_position(
             view, get_document_line_from_visible(view, pnt.y / text_height)) +
         pnt.x / text_width;
}

RECT MockEditorInterface::editor_rect(EditorViewType /*view*/) const {
  RECT r = {0, 0, 10000, 10000};
  return r;
}

MockEditorInterface::MockEditorInterface() {
  for (auto view : enum_range<EditorViewType>())
    m_active_document_index[view] = -1;
}

MockEditorInterface::~MockEditorInterface() = default;

void MockEditorInterface::open_virtual_document(EditorViewType view,
                                                const std::wstring &path,
                                                const std::wstring &data) {
  MockedDocumentInfo info;
  info.path = path;
  info.set_data(data);
  info.codepage = EditorCodepage::utf8;
  m_documents[view].push_back(std::move(info));
  m_active_view = view;
  m_active_document_index[view] =
      static_cast<int>(m_documents[view].size() - 1);
}

void MockEditorInterface::set_active_document_text(EditorViewType view,
                                                   const std::wstring &text) {
  auto doc = active_document(view);
  if (doc)
    doc->set_data(text);
}

std::vector<std::string>
MockEditorInterface::get_underlined_words(EditorViewType view,
                                          int indicator_id) const {
  auto doc = active_document(view);
  if (!doc)
    return {};
  if (indicator_id >= static_cast<int>(doc->indicator_info.size()))
    return {};
  auto &target = doc->indicator_info[indicator_id].set_for;
  auto it = target.begin(), jt = target.begin();
  std::vector<std::string> result;
  while (true) {
    it = std::find(jt, target.end(), true);
    if (it == target.end())
      return result;
    jt = std::find(it, target.end(), false);
    result.push_back(get_text_range(view,
                                    static_cast<long>(it - target.begin()),
                                    static_cast<long>(jt - target.begin())));
  }
}

void MockEditorInterface::make_all_visible(EditorViewType view) {
  auto doc = active_document(view);
  if (!doc)
    return;

  doc->visible_lines = {0, get_document_line_count(view)};
}

void MockEditorInterface::set_lexer(EditorViewType view, int lexer) {
  auto doc = active_document(view);
  if (!doc)
    return;
  doc->lexer = lexer;
}

void MockEditorInterface::set_whole_text_style(EditorViewType view, int style) {
  auto doc = active_document(view);
  if (!doc)
    return;
  std::fill(doc->style.begin(), doc->style.end(), style);
}

void MockEditorInterface::set_codepage(EditorViewType view,
                                       EditorCodepage codepage) {
  auto doc = active_document(view);
  if (!doc)
    return;
  doc->codepage = codepage;
}

void MockEditorInterface::delete_range(EditorViewType view, long start,
                                       long length) {
  auto doc = active_document(view);
  if (!doc)
    return;
  doc->data.erase(doc->data.begin() + start,
                  doc->data.begin() + start + length);
  doc->style.erase(doc->style.begin() + start,
    doc->style.begin() + start + length);
}

MockedDocumentInfo *MockEditorInterface::active_document(EditorViewType view) {
  if (m_documents[view].empty())
    return nullptr;
  return &m_documents[view][m_active_document_index[view]];
}

const MockedDocumentInfo *
MockEditorInterface::active_document(EditorViewType view) const {
  if (m_documents[view].empty())
    return nullptr;
  return &m_documents[view][m_active_document_index[view]];
}

