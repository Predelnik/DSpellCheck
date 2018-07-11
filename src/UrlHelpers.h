#pragma once

namespace UrlHelpers
{
	bool is_ftp_url(std::wstring s);

	bool is_github_url(std::wstring url);

	std::wstring github_url_to_api_recursive_tree_url(std::wstring github_url);
        std::wstring github_url_to_contents_url(std::wstring github_url);
        std::wstring github_file_url_to_download_url(std::wstring github_url);
}
