#include "MockEditorInterface.h"
#include "CommonFunctions.h"

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
  active_document(view).selection = {from, to};
}

void MockEditorInterface::replace_selection(EditorViewType view,
                                            const char *str) {
  auto &doc = active_document(view);
  auto &d = doc.data;
  d.replace(doc.selection[0], doc.selection[1], to_wstring(str));
}

void MockEditorInterface::set_indicator_style(EditorViewType view,
                                              int indicator_index, int style) {
  auto &doc = active_document(view);
  doc.indicator_info.resize(indicator_index + 1);
  doc.indicator_info[indicator_index].style = style;
}

void MockEditorInterface::set_indicator_foreground(EditorViewType view,
                                                   int indicator_index,
                                                   int style) {
  auto &doc = active_document(view);
  doc.indicator_info.resize(indicator_index + 1);
  doc.indicator_info[indicator_index].foreground = style;
}

void MockEditorInterface::set_current_indicator(EditorViewType view,
                                                int indicator_index) {
  auto &doc = active_document(view);
  doc.indicator_info.resize(indicator_index + 1);
  doc.current_indicator = indicator_index;
}

void MockEditorInterface::indicator_fill_range(EditorViewType view, long from,
                                               long to) {
  auto &doc = active_document(view);
  auto &s = doc.indicator_info[doc.current_indicator].set_for;
  s.resize(to + 1);
  std::fill(s.begin() + from, s.end() + to, true);
}

void MockEditorInterface::indicator_clear_range(EditorViewType view, long from,
                                                long to) {
  auto &doc = active_document(view);
  auto &s = doc.indicator_info[doc.current_indicator].set_for;
  s.resize(to + 1);
  std::fill(s.begin() + from, s.end() + to, false);
}

EditorViewType MockEditorInterface::active_view() const {
  return m_active_view;
}

EditorCodepage MockEditorInterface::get_encoding(EditorViewType view) const {
  return active_document(view).codepage;
}

int MockEditorInterface::get_current_pos(EditorViewType view) const {
  return active_document(view).current_pos;
}

int MockEditorInterface::get_current_line_number(EditorViewType view) const {
  auto &doc = active_document(view);
  return static_cast<int>(
      std::count(doc.data.begin(), doc.data.begin() + doc.current_pos, '\n'));
}

int MockEditorInterface::get_text_height(EditorViewType /*view*/,
                                         int /*line*/) const {
  return 13;
}

int MockEditorInterface::line_from_position(EditorViewType view,
                                            int position) const {
  auto &doc = active_document(view);
  return static_cast<int>(
      std::count(doc.data.begin(), doc.data.begin() + position, '\n'));
}

long MockEditorInterface::get_line_start_position(EditorViewType view,
                                                  int line) const {
  auto &doc = active_document(view);
  size_t index = 0;
  for (int i = 0; i < line; ++i) {
    index = doc.data.find('\n', index);
    if (index == std::wstring::npos)
      return static_cast<long>(doc.data.size());

    ++index;
  }
  return static_cast<long> (index);
}

long MockEditorInterface::get_line_end_position(EditorViewType view,
                                                int line) const {
  auto &doc = active_document(view);
  size_t index = 0;
  for (int i = 0; i < line; ++i) {
    index = doc.data.find('\n', index);
    if (index == std::wstring::npos)
      return static_cast<long> (doc.data.size());
    ++index;
  }
  return static_cast<long> (index);
}

int MockEditorInterface::get_lexer(EditorViewType view) const {
  return active_document(view).lexer;
}

long MockEditorInterface::get_selection_start(EditorViewType view) const {
  return active_document(view).selection[0];
}

long MockEditorInterface::get_selection_end(EditorViewType view) const {
  return active_document(view).selection[1];
}

int MockEditorInterface::get_style_at(EditorViewType view,
                                      long position) const {
  return active_document(view).style[position];
}

bool MockEditorInterface::is_style_hotspot(EditorViewType view,
                                           int style) const {
  return active_document(view).hotspot_tyle == style;
}

long MockEditorInterface::get_active_document_length(
    EditorViewType view) const {
  return static_cast<long> (active_document(view).data.length());
}

long MockEditorInterface::get_line_length(EditorViewType view, int line) const {
  size_t index = 0;
  auto &doc = active_document(view);
  for (int i = 0; i < line; ++i) {
    index = doc.data.find('\n', index);
    if (index == std::wstring::npos)
      return -1;
    ++index;
  }
  return static_cast<long> (doc.data.find('\n', index) - index);
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
  return active_document(view).visible_lines[0];
}

long MockEditorInterface::get_lines_on_screen(EditorViewType view) const {
  auto &l = active_document(view).visible_lines;
  return l[1] - l[0] + 1;
}

long MockEditorInterface::get_document_line_from_visible(
    EditorViewType view, long visible_line) const {
  auto &doc = active_document(view);
  return doc.visible_lines[0] + visible_line;
}

long MockEditorInterface::get_document_line_count(EditorViewType view) const {
  auto &doc = active_document(view);
    return static_cast<long> (std::count(doc.data.begin(), doc.data.begin() + doc.current_pos, '\n'));
}

MockEditorInterface::~MockEditorInterface() = default;

MockedDocumentInfo &MockEditorInterface::active_document(EditorViewType view) {
  return m_documents[view][m_active_document_index[view]];
}

const MockedDocumentInfo &
MockEditorInterface::active_document(EditorViewType view) const {
  return m_documents[view][m_active_document_index[view]];
}

std::wstring MockEditorInterface::convert_to_wstring(EditorViewType view,
                                                     const char *str) {
  switch (active_document(view).codepage) {
  case EditorCodepage::ansi:
    return to_wstring(str);
  case EditorCodepage::utf8:
    return utf8_to_wstring(str);
  case EditorCodepage::COUNT:
    break;
  }
  assert(!"wrong codepage");
  abort ();
}
