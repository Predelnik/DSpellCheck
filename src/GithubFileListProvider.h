#pragma once

#include "FileListProvider.h"
#include "TaskWrapper.h"

class GitHubFileListProvider : public FileListProvider
{
public:
	GitHubFileListProvider(HWND parent);
	void set_root_path(const std::wstring &root_path);
	void update_file_list() override;
        void download_dictionary(const std::wstring &aff_filepath, const std::wstring &target_path, std::shared_ptr<ProgressData> progress_data) override;
        void cancel_download();

private:
	std::wstring m_root_path;
	TaskWrapper m_get_file_list_task;
	TaskWrapper m_download_file_task;
};
