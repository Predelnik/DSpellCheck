#pragma once

#include <npp/EditorInterface.h>

#include "utils/enum_array.h"
#include <vector>

class MockedIndicatorInfo {
public:
  int style;
  int foreground;
  std::vector<bool> set_for;
};

class MockedDocumentInfo {
public:
  std::wstring path;
  std::wstring data;
  std::array<long, 2> selection;
  EditorCodepage codepage;
  std::vector<MockedIndicatorInfo> indicator_info;
  std::vector<int> style;
  int lexer = 0;
  int hotspot_tyle = 123;
  int current_indicator;
  long current_pos;
  std::array<long, 2> visible_lines;
};

class MockEditorInterface : public EditorInterface {
public:
  void move_active_document_to_other_view() override;
  void add_toolbar_icon(int cmd_id,
                        const toolbarIcons *tool_bar_icons_ptr) override;
  void force_style_update(EditorViewType view, long from, long to) override;
  void set_selection(EditorViewType view, long from, long to) override;
  void replace_selection(EditorViewType view, const char *str) override;
  void set_indicator_style(EditorViewType view, int indicator_index,
                           int style) override;
  void set_indicator_foreground(EditorViewType view, int indicator_index,
                                int style) override;
  void set_current_indicator(EditorViewType view, int indicator_index) override;
  void indicator_fill_range(EditorViewType view, long from, long to) override;
  void indicator_clear_range(EditorViewType view, long from, long to) override;
  EditorViewType active_view() const override;
  EditorCodepage get_encoding(EditorViewType view) const override;
  int get_current_pos(EditorViewType view) const override;
  int get_current_line_number(EditorViewType view) const override;
  int get_text_height(EditorViewType view, int line) const override;
  int line_from_position(EditorViewType view, int position) const override;
  long get_line_start_position(EditorViewType view, int line) const override;
  long get_line_end_position(EditorViewType view, int line) const override;
  int get_lexer(EditorViewType view) const override;
  long get_selection_start(EditorViewType view) const override;
  long get_selection_end(EditorViewType view) const override;
  int get_style_at(EditorViewType view, long position) const override;
  bool is_style_hotspot(EditorViewType view, int style) const override;
  long get_active_document_length(EditorViewType view) const override;
  long get_line_length(EditorViewType view, int line) const override;
  int get_point_x_from_position(EditorViewType view,
                                long position) const override;
  int get_point_y_from_position(EditorViewType view,
                                long position) const override;
  long get_first_visible_line(EditorViewType view) const override;
  long get_lines_on_screen(EditorViewType view) const override;
  long get_document_line_from_visible(EditorViewType view,
                                      long visible_line) const override;
  long get_document_line_count(EditorViewType view) const override;
  bool open_document(std::wstring filename) override;
  void activate_document(int index, EditorViewType view) override;
  void activate_document(const std::wstring &filepath,
                         EditorViewType view) override;
  void switch_to_file(const std::wstring &path) override;
  std::vector<std::wstring>
  get_open_filenames(std::optional<EditorViewType> view) const override;
  bool is_opened(const std::wstring &filename) const override;
  std::wstring active_document_path() const override;
  std::wstring active_file_directory() const override;
  std::wstring plugin_config_dir() const override;
  std::string selected_text(EditorViewType view) const override;
  std::string get_current_line(EditorViewType view) const override;
  std::string get_line(EditorViewType view, long line_number) const override;
  std::optional<long> char_position_from_point(EditorViewType view, int x,
                                               int y) const override;
  HWND app_handle() const override;
  std::wstring get_full_current_path() const override;
  std::string get_text_range(EditorViewType view, long from,
                             long to) const override;
    std::string
    get_active_document_text(EditorViewType view) const override;
  MockEditorInterface();
  ~MockEditorInterface();

private:
  MockedDocumentInfo &active_document(EditorViewType view);
  const MockedDocumentInfo &active_document(EditorViewType view) const;
  std::string convert_from_wstring(EditorViewType view,
                                   const wchar_t *str) const;
  std::wstring convert_to_wstring(EditorViewType view, const char *str) const;

private:
  enum_array<EditorViewType, std::vector<MockedDocumentInfo>> m_documents;
  enum_array<EditorViewType, int> m_active_document_index;
  EditorViewType m_active_view = EditorViewType::primary;
};
