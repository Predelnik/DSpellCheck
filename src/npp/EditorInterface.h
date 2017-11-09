#pragma once
#include <assert.h>
// To mock things and stuff

struct toolbarIcons;

class EditorInterface {
public:
	enum class ViewType {
		primary,
		secondary,

		COUNT,
	};

	enum class Codepage {
		ansi,
		utf8,

		COUNT,
	};

	enum class ViewTarget {
		primary,
		secondary,
		both,
	};

	static ViewType OtherView(ViewType view) {
		switch (view) {
		case ViewType::primary: return ViewType::secondary;
		case ViewType::secondary: return ViewType::primary;
		}
		assert(false);
		return ViewType::primary;
	}

	// non-const
	virtual bool open_document(std::wstring filename) = 0;
	virtual void activate_document(int index, ViewType view) = 0;
	virtual void activate_document(const std::wstring& filepath, ViewType view) = 0;
	virtual void switch_to_file(const std::wstring& path) = 0;
	virtual void move_active_document_to_other_view() = 0;
	virtual void add_toolbar_icon(int cmdId, const toolbarIcons* toolBarIconsPtr) = 0;

	// const
	virtual std::vector<std::wstring> get_open_filenames(ViewTarget viewTarget = ViewTarget::both) const = 0;
	virtual ViewType active_view() const = 0;
	virtual bool is_opened(const std::wstring& filename) const = 0;
	virtual Codepage get_encoding(ViewType view) const = 0;
	virtual std::wstring active_document_path() const = 0;
	virtual std::wstring active_file_directory() const = 0;
	virtual std::wstring plugin_config_dir() const = 0;
	virtual std::vector<char> selected_text(ViewType view) const = 0;
	virtual std::vector<char> get_current_line(ViewType view) const = 0;
	virtual int get_current_pos(ViewType view) const = 0;
	virtual int get_current_line_number(ViewType view) const = 0;
	virtual int line_from_position(ViewType view, int position) const = 0;
	virtual int position_from_line(ViewType view, int line) const = 0;
	virtual HWND app_handle() = 0;

	int get_current_pos_in_line(ViewType view) const {
		return get_current_pos(view) - position_from_line(view, get_current_line_number(view));
	}

	virtual ~EditorInterface() {
	}
};

