#pragma once
#include "npp/EditorInterface.h"

#include <vector>

struct NppData;

class NppInterface : public EditorInterface {
public:
	explicit NppInterface(const NppData* npp_data);

	static int to_index(ViewType target);

	// non-const
	bool open_document(std::wstring filename) override;
	void activate_document(int index, ViewType view) override;
	void activate_document(const std::wstring& filepath, ViewType view) override;
	void switch_to_file(const std::wstring& path) override;
	void move_active_document_to_other_view() override;
	void add_toolbar_icon(int cmd_id, const toolbarIcons* tool_bar_icons_ptr) override;

	// const
	std::vector<std::wstring> get_open_filenames(ViewTarget viewTarget = ViewTarget::both) const override;
	ViewType active_view() const override;
	bool is_opened(const std::wstring& filename) const override;
	Codepage get_encoding(ViewType view) const override;
	std::wstring active_document_path() const override;
	std::wstring active_file_directory() const override;
	std::vector<char> selected_text(ViewType view) const override;
	std::vector<char> get_current_line(ViewType view) const override;
	int get_current_pos(ViewType view) const override;
	int get_current_line_number(ViewType view) const override;
	int line_from_position(ViewType view, int position) const override;
	std::wstring plugin_config_dir() const override;
	int position_from_line(ViewType view, int line) const override;
	HWND app_handle() override;

private:
	// these functions are const per se
	LRESULT send_msg_to_npp(UINT msg, WPARAM w_param = 0, LPARAM l_param = 0) const;
	LRESULT send_msg_to_scintilla(ViewType view, UINT msg, WPARAM w_param = 0, LPARAM lParam = 0) const;
	std::wstring get_dir_msg(UINT msg) const;
	void do_command(int id);

private:
	const NppData& m_npp_data;
};

