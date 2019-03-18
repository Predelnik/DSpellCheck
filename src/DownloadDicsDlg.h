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

#include "FTPFileStatus.h"
#include "StaticDialog\StaticDialog.h"
#include "TaskWrapper.h"
#include <optional>
#include "FileListProvider.h"
#include "utils/WinApiControls.h"

enum class UrlType
{
	ftp,
	ftp_web_proxy,
	github,
	unknown,
};

class LanguageInfo;
class GitHubFileListProvider;
class SpellerContainer;
class ProgressData;

void ftp_trim(std::wstring &ftp_address);

enum class FtpOperationType {
  fill_file_list = 0,
  download_file,
};

enum class FtpWebOperationErrorType {
  none,
  http_client_cannot_be_initialized,
  url_cannot_be_opened,
  querying_status_code_failed,
  proxy_authorization_required,
  http_error,
  html_cannot_be_parsed,
  file_is_not_writeable,
  download_cancelled,
};

struct FtpWebOperationError {
  FtpWebOperationErrorType type;
  int status_code;
};

enum class FtpOperationErrorType {
  none,
  login_failed,
  download_failed,
  download_cancelled,
};

class SpellChecker;

struct FtpOperationParams {
  std::wstring address;
  std::wstring path;
  bool use_proxy;
  std::wstring proxy_address;
  int proxy_port;
  bool anonymous;
  std::wstring proxy_username;
  std::wstring proxy_password;
  bool write_debug_log;
  std::wstring debug_log_path;
  bool use_passive_mode;
};

class DownloadDicsDlg : public StaticDialog {
  enum { refresh_timer_id = 0 };

public:
  ~DownloadDicsDlg() override;
  DownloadDicsDlg(HINSTANCE h_inst, HWND parent, const Settings &settings, SpellerContainer &speller_container);
  void do_dialog();
  // Maybe hunspell interface should be passed here
  INT_PTR WINAPI run_dlg_proc(UINT message, WPARAM w_param,
                              LPARAM l_param) override;
  void update_list_box();
  void on_new_file_list(std::vector<FileDescription> list);
  void preserve_current_address_index(Settings &settings);
  void reset_download_combobox();
  void add_user_server(std::wstring server);
  void prepare_file_list_update();
  FtpOperationParams spawn_ftp_operation_params(const std::wstring &full_path) const;
  void update_file_list_async_web_proxy(const std::wstring &full_path);
  void update_file_list_async(const std::wstring &full_path);
  void download_file_async(const std::wstring &full_path,
                           const std::wstring &target_location);
  void download_file_async_web_proxy(const std::wstring &full_path,
                                     const std::wstring &target_location);
  void do_ftp_operation(FtpOperationType type, const std::wstring &full_path,
                        const std::wstring &file_name = L"",
                        const std::wstring &location = L"");
  void refresh();
  void start_next_download();
  UrlType selected_url_type();
  void download_github_file(const std::wstring &title, const std::wstring &path, std::shared_ptr<ProgressData> progress_data);
  void download_file();
  void download_selected();
  void fill_file_list();
  void on_display_action();
  void indicate_that_saving_might_be_needed();
  void update_controls();
  void update_settings(Settings &settings);
  void update_download_button_availability();
  void set_cancel_pressed(bool value);
  LRESULT ask_replacement_message(const wchar_t *dic_name);
  bool prepare_downloading();
  void finalize_downloading();
  void on_zip_file_downloaded();
  std::optional<std::wstring> current_address() const;
  void update_status(const wchar_t *text, COLORREF status_color);
  void process_file_list_error(FtpOperationErrorType error);
  void process_file_list_error(const FtpWebOperationError &error);

private:
  std::vector<FileDescription> m_current_list;
  HBRUSH m_default_brush;
  COLORREF m_status_color;
  HWND m_h_file_list;
  HWND m_h_address = nullptr;
  HWND m_h_status;
  HWND m_h_install_selected;
  HWND m_h_show_only_known;
  HWND m_h_refresh_btn;
  HWND m_h_install_system;
  HICON m_refresh_icon;
  bool m_cancel_pressed;
  bool m_check_if_saving_is_needed;
  std::optional<TaskWrapper> m_ftp_operation_task;
  WinApi::ComboBox m_address_cmb;

  bool m_failure;
  int m_downloaded_count;
  int m_supposed_downloaded_count;
  std::wstring m_message;
  std::vector<FileDescription> m_to_download;
  decltype(m_to_download)::iterator m_cur;
  std::array<std::wstring, 2> m_default_server_names;
  const Settings &m_settings;
  bool m_address_is_set = false;
  std::unique_ptr<GitHubFileListProvider> m_github_provider;
  SpellerContainer &m_speller_container;
};
