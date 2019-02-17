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

#include <fcntl.h>
#include <io.h>

#include "CheckedList/CheckedList.h"
#include "CommonFunctions.h"
#include "Definements.h"
#include "DownloadDicsDlg.h"

#include "ConnectionSettingsDialog.h"
#include "FTPClient.h"
#include "FTPDataTypes.h"
#include "GithubFileListProvider.h"
#include "HunspellInterface.h"
#include "MainDefs.h"
#include "Plugin.h"
#include "ProgressData.h"
#include "ProgressDlg.h"
#include "Settings.h"
#include "SpellerContainer.h"
#include "UrlHelpers.h"
#include "resource.h"
#include "unzip.h"
#include "utils/string_utils.h"
#include "utils/winapi.h"
#include "WindowsDefs.h"
#include <Wininet.h>
#include <variant>

void DownloadDicsDlg::do_dialog() {
  if (!isCreated()) {
    create(IDD_DOWNLOADDICS);
  } else {
    goToCenter();
    display();
  }
  SetFocus(m_h_file_list);
}

void DownloadDicsDlg::fill_file_list() {
  auto url = *current_address();
  prepare_file_list_update();
  switch (selected_url_type()) {
  case UrlType::ftp:
    update_file_list_async(url);
    break;
  case UrlType::ftp_web_proxy:
    update_file_list_async_web_proxy(url);
    break;
  case UrlType::github:
    m_github_provider->set_root_path(url);
    m_github_provider->update_file_list();
    break;
  case UrlType::unknown:
    break;
  }
}

void DownloadDicsDlg::on_display_action() { fill_file_list(); }

DownloadDicsDlg::~DownloadDicsDlg() {
  if (m_refresh_icon != nullptr)
    DestroyIcon(m_refresh_icon);
}

DownloadDicsDlg::DownloadDicsDlg(HINSTANCE h_inst, HWND parent, const Settings &settings, SpellerContainer &speller_container)
    : m_settings(settings), m_github_provider(std::make_unique<GitHubFileListProvider>(parent, settings)), m_speller_container(speller_container) {
  m_github_provider->file_list_received.connect([this](const std::vector<FileDescription> &list) { on_new_file_list(list); });
  m_github_provider->error_happened.connect([this](const std::string &description) {
    auto wstr = to_wstring(description);
    replace_all_inplace(wstr, L"\r\n", L" ");
    update_status(wstring_printf(rc_str(IDS_STATUS_NETWORK_ERROR_PS).c_str(), wstr.c_str()).c_str(), COLOR_FAIL);
  });
  m_github_provider->file_downloaded.connect([this] {
    m_message += prev(m_cur)->title + L'\n';
    ++m_downloaded_count;
    start_next_download();
  });
  m_ftp_operation_task = TaskWrapper{parent};
  Window::init(h_inst, parent);
  m_default_server_names[0] = L"https://github.com/LibreOffice/dictionaries";
  m_default_server_names[1] = L"ftp://ftp.snt.utwente.nl/pub/software/openoffice/contrib/dictionaries/";
  m_settings.settings_changed.connect([this] {
    update_controls();
    update_list_box();
  });
}

void DownloadDicsDlg::indicate_that_saving_might_be_needed() { m_check_if_saving_is_needed = true; }

LRESULT DownloadDicsDlg::ask_replacement_message(const wchar_t *dic_name) {
  std::wstring name = apply_alias(dic_name, m_settings.language_name_style);
  return MessageBox(_hParent, wstring_printf(rc_str(IDS_DICT_PS_EXISTS_BODY).c_str(), name.c_str()).c_str(), rc_str(IDS_DICT_EXISTS_HEADER).c_str(), MB_YESNO);
}

static std::wstring get_temp_path() {
  std::vector<wchar_t> temp_path_buf(MAX_PATH);
  GetTempPath(MAX_PATH, temp_path_buf.data());
  return temp_path_buf.data();
}

bool DownloadDicsDlg::prepare_downloading() {
  m_downloaded_count = 0;
  m_supposed_downloaded_count = 0;
  m_failure = false;
  m_message = rc_str(IDS_DICTS_COPIED);
  m_to_download.clear();

  // If path isn't exist we're gonna try to create it else it's finish
  if (!check_for_directory_existence(get_temp_path(), false, _hParent)) {
    MessageBox(_hParent, rc_str(IDS_TEMPORARY_PATH_INVALID_BODY).c_str(), rc_str(IDS_TEMPORARY_PATH_INVALID_HEADER).c_str(), MB_OK | MB_ICONEXCLAMATION);
    return false;
  }
  m_cancel_pressed = false;
  ProgressDlg *p = get_progress();
  p->get_progress_data()->set(0, L"");
  get_progress()->update();
  p->set_top_message(L"");
  if (!check_for_directory_existence(m_settings.download_install_dictionaries_for_all_users ? m_settings.hunspell_system_path : m_settings.hunspell_user_path,
                                     false,
                                     _hParent)) // If path doesn't exist we're gonna try to create it
                                                // else it's finish
  {
    MessageBox(_hParent, rc_str(IDS_CANT_CREATE_DOWNLOAD_DIR).c_str(), rc_str(IDS_NO_DICTS_DOWNLOADED).c_str(), MB_OK | MB_ICONEXCLAMATION);
    return false;
  }
  for (int i = 0; i < ListBox_GetCount(m_h_file_list); i++) {
    if (CheckedListBox_GetCheckState(m_h_file_list, i) == BST_CHECKED) {
      m_to_download.push_back(m_current_list[i]);
      ++m_supposed_downloaded_count;
    }
  }
  m_cur = m_to_download.begin();
  return true;
}

void DownloadDicsDlg::finalize_downloading() {
  get_progress()->display(false);
  if (m_failure) {
    MessageBox(_hParent, rc_str(IDS_CANT_WRITE_DICT_FILES).c_str(), rc_str(IDS_NO_DICTS_DOWNLOADED).c_str(), MB_OK | MB_ICONEXCLAMATION);
  } else if (m_downloaded_count > 0) {
    MessageBox(_hParent, m_message.c_str(), rc_str(IDS_DICTS_DOWNLOADED).c_str(), MB_OK | MB_ICONINFORMATION);
  } else if (m_supposed_downloaded_count > 0) // Otherwise - silent
  {
    MessageBox(_hParent, rc_str(IDS_ZERO_DICTS_DOWNLOAD).c_str(), rc_str(IDS_NO_DICTS_DOWNLOADED).c_str(), MB_OK | MB_ICONEXCLAMATION);
  }
  for (int i = 0; i < ListBox_GetCount(m_h_file_list); i++)
    CheckedListBox_SetCheckState(m_h_file_list, i, BST_UNCHECKED);
  m_settings.settings_changed();
}

static const auto buf_size_for_copy = 10240;

void DownloadDicsDlg::on_zip_file_downloaded() {
  wchar_t prog_message[default_buf_size];
  std::map<std::string, int> files_found; // 0x01 - .aff found, 0x02 - .dic found
  auto downloaded = prev(m_cur);
  const auto local_path_ansi = to_string(get_temp_path() + L"/" + downloaded->path);
  unzFile fp = unzOpen(local_path_ansi.c_str());
  unz_file_info f_info;
  if (unzGoToFirstFile(fp) != UNZ_OK)
    goto clean_and_continue;
  do {
    std::string dic_file_name_ansi;
    {
      unzGetCurrentFileInfo(fp, &f_info, nullptr, 0, nullptr, 0, nullptr, 0);
      std::vector<char> buf(f_info.size_filename + 1);
      unzGetCurrentFileInfo(fp, &f_info, buf.data(), static_cast<uLong>(buf.size()), nullptr, 0, nullptr, 0);
      dic_file_name_ansi = buf.data();
    }

    if (dic_file_name_ansi.length() < 4)
      continue;
    const auto ext = std::string_view(dic_file_name_ansi).substr(dic_file_name_ansi.length() - 4);
    bool is_aff_file = ext == ".aff";
    bool is_dic_file = ext == ".dic";
    if (is_aff_file || is_dic_file) {
      auto filename = dic_file_name_ansi.substr(0, dic_file_name_ansi.length() - 4);
      files_found[filename] |= (is_aff_file ? 0x01 : 0x02);
      unzOpenCurrentFile(fp);
      auto dic_file_local_path = get_temp_path();
      auto dic_file_name = to_wstring(filename.c_str());
      dic_file_local_path += dic_file_name;
      if (is_aff_file)
        dic_file_local_path += L".aff";
      else
        dic_file_local_path += L".dic";

      SetFileAttributes(dic_file_local_path.c_str(), FILE_ATTRIBUTE_NORMAL);
      auto local_dic_file_handle = _wopen(dic_file_local_path.c_str(), _O_CREAT | _O_BINARY | _O_WRONLY);
      if (local_dic_file_handle == -1)
        continue;

      swprintf(prog_message, IDS_EXTRACTING_PS, dic_file_name.c_str());
      get_progress()->set_top_message(prog_message);
      DWORD bytes_total = 0;
      int bytes_copied;
      std::vector<char> file_copy_buf(buf_size_for_copy);
      while ((bytes_copied = unzReadCurrentFile(fp, file_copy_buf.data(), static_cast<unsigned int>(file_copy_buf.size()))) != 0) {
        _write(local_dic_file_handle, file_copy_buf.data(), bytes_copied);
        bytes_total += bytes_copied;
        swprintf(prog_message, rc_str(IDS_PS_OF_PS_BYTES_EXTRACTED_PS).c_str(), bytes_total, f_info.uncompressed_size,
                 bytes_total * 100 / f_info.uncompressed_size);
        get_progress()->get_progress_data()->set(bytes_total * 100 / f_info.uncompressed_size, prog_message);
      }
      unzCloseCurrentFile(fp);
      _close(local_dic_file_handle);
    }
  } while (unzGoToNextFile(fp) == UNZ_OK);
  // Now we're gonna check what's exactly we extracted with using FilesFound
  // map
  for (auto &[file_name_ansi, mask] : files_found) {
    auto file_name = to_wstring(file_name_ansi.c_str());
    if (mask != 0x03) // Some of .aff/.dic is missing
    {
      auto dic_file_local_path = get_temp_path();
      switch (mask) {
      case 1:
        dic_file_local_path += L".aff";
        break;
      case 2:
        dic_file_local_path += L".dic";
        break;
      }
      WinApi::delete_file(dic_file_local_path.c_str());
    } else {
      auto dic_file_local_path = get_temp_path();
      (dic_file_local_path += file_name) += L".aff";
      std::wstring hunspell_dic_path = m_settings.download_install_dictionaries_for_all_users ? m_settings.hunspell_system_path : m_settings.hunspell_user_path;
      hunspell_dic_path += L"\\";
      hunspell_dic_path += file_name;
      hunspell_dic_path += L".aff";
      bool confirmation = true;
      bool replace_question_was_asked = false;
      if (PathFileExists(hunspell_dic_path.c_str())) {
        auto answer = ask_replacement_message(file_name.c_str());
        replace_question_was_asked = true;
        if (answer == IDNO) {
          confirmation = false;
          WinApi::delete_file(dic_file_local_path.c_str());
        } else {
          m_speller_container.get_hunspell_speller().dictionary_removed(hunspell_dic_path);
          WinApi::delete_file(hunspell_dic_path.c_str());
        }
      }

      if (confirmation && !move_file_and_reset_security_descriptor(dic_file_local_path.c_str(), hunspell_dic_path.c_str())) {
        WinApi::delete_file(dic_file_local_path.c_str());
        m_failure = true;
      }
      dic_file_local_path = dic_file_local_path.substr(0, dic_file_local_path.length() - 4) + L".dic";
      hunspell_dic_path = hunspell_dic_path.substr(0, hunspell_dic_path.length() - 4) + L".dic";
      if (!confirmation) {
        WinApi::delete_file(dic_file_local_path.c_str());
      } else if (PathFileExists(hunspell_dic_path.c_str())) {
        bool res;
        if (replace_question_was_asked)
          res = !confirmation;
        else {
          res = (ask_replacement_message(file_name.c_str()) == IDNO);
        }
        if (res) {
          WinApi::delete_file(dic_file_local_path.c_str());
          confirmation = false;
        } else {
          WinApi::delete_file(hunspell_dic_path.c_str());
        }
      }

      if (confirmation && !move_file_and_reset_security_descriptor(dic_file_local_path.c_str(), hunspell_dic_path.c_str())) {
        WinApi::delete_file(dic_file_local_path.c_str());
        m_failure = true;
      }
      auto converted_dic_name = apply_alias(file_name, m_settings.language_name_style);
      if (m_failure)
        goto clean_and_continue;

      if (confirmation) {
        m_message += converted_dic_name + L"\n";
        m_downloaded_count++;
      }
      if (!confirmation)
        m_supposed_downloaded_count--;
    }
  }
clean_and_continue:
  files_found.clear();
  unzClose(fp);
  auto local_path = get_temp_path() + L"/" + downloaded->path;
  WinApi::delete_file(local_path.c_str());
  if (m_failure)
    return finalize_downloading();

  start_next_download();
}

void DownloadDicsDlg::start_next_download() {
  if (m_cur == m_to_download.end() || m_cancel_pressed)
    return finalize_downloading();

  get_progress()->set_top_message(wstring_printf(rc_str(IDS_DOWNLOADING_PS).c_str(), m_cur->title.c_str()).c_str());
  download_file();
  ++m_cur;
}

UrlType DownloadDicsDlg::selected_url_type() {
  const auto address = *current_address();
  if (UrlHelpers::is_github_url(address))
    return UrlType::github;

  if (UrlHelpers::is_ftp_url(address)) {
    if (m_settings.use_proxy && m_settings.proxy_type == ProxyType::web_proxy)
      return UrlType::ftp_web_proxy;

    return UrlType::ftp;
  }

  return UrlType::unknown;
}

void DownloadDicsDlg::download_github_file(const std::wstring &title, const std::wstring &path, std::shared_ptr<ProgressData> progress_data) {
  auto local_path = std::wstring(m_settings.get_dictionary_download_path());
  auto local_name = local_path + L"\\" + path.substr(path.rfind(L'/'));
  if (PathFileExists(local_name.c_str())) {
    if (ask_replacement_message(title.c_str()) == IDNO) {
      --m_supposed_downloaded_count;
      return;
    }
    m_speller_container.get_hunspell_speller().dictionary_removed(local_name);
  }

  m_github_provider->download_dictionary(path, local_path, progress_data);
}

void DownloadDicsDlg::download_file() {
  const auto ftp_path = *current_address() + m_cur->path;
  const auto ftp_location = get_temp_path() + L"/" + m_cur->path;

  switch (selected_url_type()) {
  case UrlType::ftp:
    download_file_async(ftp_path, ftp_location);
    break;
  case UrlType::ftp_web_proxy:
    download_file_async_web_proxy(ftp_path, ftp_location);
    break;
  case UrlType::github:
    download_github_file(m_cur->title, m_cur->path, get_progress()->get_progress_data());
    break;
  case UrlType::unknown:
    break;
  }
}

void DownloadDicsDlg::download_selected() {
  if (!prepare_downloading())
    return;

  start_next_download();
}

void ftp_trim(std::wstring &ftp_address) {
  trim_inplace(ftp_address);
  for (auto &c : ftp_address) {
    if (c == L'\\')
      c = L'/';
  }
  std::wstring ftp_prefix = L"ftp://";
  if (ftp_address.find(ftp_prefix) == 0)
    ftp_address.erase(ftp_address.begin(), ftp_address.begin() + ftp_prefix.length());

  std::transform(ftp_address.begin(), std::find(ftp_address.begin(), ftp_address.end(), L'/'), ftp_address.begin(),
                 &towlower); // In dir names upper/lower case could matter
}

std::pair<std::wstring, std::wstring> ftp_split(std::wstring full_path) {
  ftp_trim(full_path);
  auto it = std::find(full_path.begin(), full_path.end(), L'/');
  return {{full_path.begin(), it}, {it != full_path.end() ? next(it) : it, full_path.end()}};
}

void DownloadDicsDlg::update_list_box() {
  if (m_h_file_list == nullptr || m_current_list.empty())
    return;

  ListBox_ResetContent(m_h_file_list);
  for (auto &info : m_current_list) {
    if (m_settings.download_show_only_recognized_dictionaries && !info.was_alias_applied)
      continue;
    ListBox_AddString(m_h_file_list, info.title.c_str());
  }
}

// Copy of CFile with ability to kick progress bar
class ProgressObserver : public nsFTP::CFTPClient::CNotification {
  std::shared_ptr<ProgressData> m_progress_data;
  std::wstring m_target_path;
  TextPosition m_bytes_received = 0;
  TextPosition m_target_size;
  concurrency::cancellation_token m_token;
  nsFTP::CFTPClient &m_client;

public:
  ProgressObserver(std::shared_ptr<ProgressData> progress_data, std::wstring target_path, TextPosition target_size, nsFTP::CFTPClient &client,
                   concurrency::cancellation_token token)
      : m_progress_data(std::move(progress_data)), m_target_path(std::move(target_path)), m_target_size(target_size), m_token(std::move(token)),
        m_client(client) {}

  void OnBytesReceived(const nsFTP::TByteVector & /*vBuffer*/, long l_received_bytes) override {
    if (m_token.is_canceled()) {
      WinApi::delete_file(m_target_path.c_str());
      m_client.Logout();
      return;
    }
    m_bytes_received += l_received_bytes;
    m_progress_data->set(m_bytes_received * 100 / m_target_size, wstring_printf(rc_str(IDS_PD_OF_PD_BYTES_DOWNLOADED_PD).c_str(), m_bytes_received,
                                                                                m_target_size, m_bytes_received * 100 / m_target_size));
  }
};

struct LogObserver : public nsFTP::CFTPClient::CNotification {
  LogObserver(const wchar_t *log_filename) : m_log_filename(log_filename) {}

  void OnInternalError(const tstring &error_msg, const tstring &error_filename, DWORD line) override {
    FILE *f;
    _wfopen_s(&f, m_log_filename, L"a");
    if (!f)
      return;
    wchar_t buf[default_buf_size];
    swprintf(buf, L"Fatal error %s in file %s on line %d\n", error_msg.c_str(), error_filename.c_str(), line);
    fwprintf(f, buf);
    fclose(f);
  }

  void OnSendCommand(const nsFTP::CCommand &cmd, const nsFTP::CArg &args) override {
    FILE *f;
    _wfopen_s(&f, m_log_filename, L"a");
    if (!f)
      return;
    std::wstring s = L"Command ";
    s += cmd.AsString();
    s += L" with args: ";
    for (int i = 0; i < static_cast<int>(args.size()); ++i) {
      s += args[i];
      if (i != static_cast<int>(args.size()) - 1)
        s += L", ";
    }
    s += L'\n';
    fwprintf(f, s.c_str());
    fclose(f);
  }

  void OnResponse(const nsFTP::CReply &reply) override {
    FILE *f;
    _wfopen_s(&f, m_log_filename, L"a");
    if (!f)
      return;
    wchar_t buf[default_buf_size];
    swprintf(buf, L"Got reply %s with value:\n%s\n", reply.Code().Value(), reply.Value().c_str());
    fwprintf(f, buf);
    fclose(f);
  }

private:
  const wchar_t *m_log_filename;
};

void DownloadDicsDlg::set_cancel_pressed(bool value) {
  m_cancel_pressed = value;
  m_ftp_operation_task->cancel();
  m_cur = m_to_download.end();
  m_github_provider->cancel_download();
  finalize_downloading();
}

std::optional<std::wstring> DownloadDicsDlg::current_address() const {
  if (m_h_address == nullptr)
    return std::nullopt;
  auto sel = ComboBox_GetCurSel(m_h_address);
  if (sel < 0) {
    auto text_len = ComboBox_GetTextLength(m_h_address);
    std::vector<wchar_t> buf(text_len + 1);
    ComboBox_GetText(m_h_address, buf.data(), text_len + 1);
    return buf.data();
  }
  auto text_len = ComboBox_GetLBTextLen(m_h_address, sel);
  std::vector<wchar_t> buf(text_len + 1);
  ComboBox_GetLBText(m_h_address, sel, buf.data());
  return buf.data();
}

static bool ftp_login(nsFTP::CFTPClient &client, const FtpOperationParams &params) {
  std::unique_ptr<nsFTP::CLogonInfo> logon_info;
  if (!params.use_proxy)
    logon_info = std::make_unique<nsFTP::CLogonInfo>(params.address);
  else
    logon_info = std::make_unique<nsFTP::CLogonInfo>(params.address, USHORT(21), L"anonymous", L"", L"", params.proxy_address, L"", L"",
                                                     static_cast<USHORT>(params.proxy_port), nsFTP::CFirewallType::UserWithNoLogon());
  return client.Login(*logon_info);
}

std::variant<FtpOperationErrorType, std::vector<std::wstring>> do_download_file_list_ftp(FtpOperationParams params) {
  nsFTP::CFTPClient client(nsSocket::CreateDefaultBlockingSocketInstance(), 500);
  auto log_observer = std::make_unique<LogObserver>(params.debug_log_path.c_str());
  if (params.write_debug_log) {
    client.AttachObserver(log_observer.get());
  }
  if (!ftp_login(client, params))
    return FtpOperationErrorType::login_failed;

  nsFTP::TFTPFileStatusShPtrVec list;
  if (!client.List(params.path, list, params.use_passive_mode))
    return FtpOperationErrorType::download_failed;

  std::vector<std::wstring> ret(list.size());
  std::transform(list.begin(), list.end(), ret.begin(), [](const nsFTP::TFTPFileStatusShPtr &status) { return status->Name(); });
  return ret;
}

std::optional<FtpOperationErrorType> do_download_file(FtpOperationParams params, const std::wstring &target_path, std::shared_ptr<ProgressData> progress_data,
                                                      concurrency::cancellation_token token) {
  nsFTP::CFTPClient client(nsSocket::CreateDefaultBlockingSocketInstance(), 500);
  auto log_observer = std::make_unique<LogObserver>(params.debug_log_path.c_str());
  if (params.write_debug_log) {
    client.AttachObserver(log_observer.get());
  }
  if (!ftp_login(client, params))
    return FtpOperationErrorType::login_failed;

  long file_size = 0;
  if (client.FileSize(params.path, file_size) != nsFTP::FTP_OK)
    return FtpOperationErrorType::download_failed;

  auto progress_updater = std::make_unique<ProgressObserver>(std::move(progress_data), target_path, file_size, client, token);
  client.AttachObserver(progress_updater.get());

  if (!client.DownloadFile(params.path, target_path, nsFTP::CRepresentation(nsFTP::CType::Image()), params.use_passive_mode)) {
    if (PathFileExists(target_path.c_str())) {
      WinApi::delete_file(target_path.c_str());
      return FtpOperationErrorType::download_cancelled;
    }
  }

  client.DetachObserver(progress_updater.get());
  return std::nullopt;
}

static std::variant<HINTERNET, FtpWebOperationError> open_url(HINTERNET win_inet_handle, const std::wstring &url, const FtpOperationParams &params) {
  const auto url_handle =
      InternetOpenUrl(win_inet_handle, url.c_str(), nullptr, 0, INTERNET_FLAG_PASSIVE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
  if (url_handle == nullptr)
    return FtpWebOperationError{FtpWebOperationErrorType::url_cannot_be_opened, -1};

  if (!params.anonymous) {
    InternetSetOption(url_handle, INTERNET_OPTION_PROXY_USERNAME, const_cast<wchar_t *>(params.proxy_username.c_str()),
                      static_cast<DWORD>(params.proxy_username.length() + 1));
    InternetSetOption(url_handle, INTERNET_OPTION_PROXY_PASSWORD, const_cast<wchar_t *>(params.proxy_password.c_str()),
                      static_cast<DWORD>(params.proxy_password.length() + 1));
    InternetSetOption(url_handle, INTERNET_OPTION_PROXY_SETTINGS_CHANGED, nullptr, 0);
  }
  HttpSendRequest(url_handle, nullptr, 0, nullptr, 0);

  DWORD code = 0;
  DWORD dummy = 0;
  DWORD size = sizeof(DWORD);

  if (!HttpQueryInfo(url_handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &code, &size, &dummy))
    return FtpWebOperationError{FtpWebOperationErrorType::querying_status_code_failed, -1};

  if (code != HTTP_STATUS_OK) {
    if (code == HTTP_STATUS_PROXY_AUTH_REQ)
      return FtpWebOperationError{FtpWebOperationErrorType::proxy_authorization_required, static_cast<int>(code)};

    return FtpWebOperationError{FtpWebOperationErrorType::http_error, static_cast<int>(code)};
  }
  return url_handle;
}

static std::variant<HINTERNET, FtpWebOperationError> ftp_web_proxy_login(const FtpOperationParams &params) {
  std::wstring proxy_final_string = params.proxy_address + L":" + std::to_wstring(params.proxy_port);
  const auto win_inet_handle = InternetOpen(static_plugin_name, INTERNET_OPEN_TYPE_PROXY, proxy_final_string.c_str(), L"", 0);
  if (win_inet_handle == nullptr)
    return FtpWebOperationError{FtpWebOperationErrorType::http_client_cannot_be_initialized, -1};
  DWORD time_out = 15000;
  for (auto option : {INTERNET_OPTION_CONNECT_TIMEOUT, INTERNET_OPTION_SEND_TIMEOUT, INTERNET_OPTION_RECEIVE_TIMEOUT})
    InternetSetOption(win_inet_handle, option, &time_out, sizeof(DWORD));
  return win_inet_handle;
}

inline auto initial_buffer_size = 50 * 1024;

std::variant<FtpWebOperationError, std::vector<std::wstring>> do_download_file_list_ftp_web_proxy(FtpOperationParams params) {
  auto result = ftp_web_proxy_login(params);
  if (auto error = std::get_if<FtpWebOperationError>(&result))
    return *error;

  auto handle = std::get<HINTERNET>(result);
  result = open_url(handle, L"ftp://" + params.address + L"/" + params.path, params);
  if (auto error = std::get_if<FtpWebOperationError>(&result))
    return *error;

  auto url_handle = std::get<HINTERNET>(result);

  std::vector<char> file_buffer(initial_buffer_size);
  std::size_t cur_index = 0;
  DWORD bytes_read = 0;
  DWORD bytes_read_total = 0;
  DWORD bytes_to_read = 0;
  unsigned int cur_buf_size = initial_buffer_size;
  while (true) {
    InternetQueryDataAvailable(url_handle, &bytes_to_read, 0, 0);
    if (bytes_to_read == 0)
      break;
    if (bytes_read_total + bytes_to_read > cur_buf_size) {
      file_buffer.resize(file_buffer.size() * 2);
    }

    InternetReadFile(url_handle, file_buffer.data() + cur_index, bytes_to_read, &bytes_read);
    if (bytes_read == 0)
      break;
    bytes_read_total += bytes_read;
    cur_index += bytes_read;
  }
  int count = 0;
  // Bad Parsing. Really, really bad. I'm sorry :(
  auto cur_pos = file_buffer.data();
  std::vector<std::wstring> out;
  while (static_cast<size_t>(cur_pos - file_buffer.data()) < bytes_read_total) {
    char *temp_cur_pos = cur_pos;
    cur_pos = strstr(cur_pos, "</A>");
    if (cur_pos == nullptr)
      cur_pos = strstr(temp_cur_pos, "</a>");

    if (cur_pos == nullptr)
      break;
    temp_cur_pos = cur_pos;
    while (*temp_cur_pos != '>' && temp_cur_pos > file_buffer.data())
      temp_cur_pos--;

    if (temp_cur_pos == nullptr)
      return FtpWebOperationError{FtpWebOperationErrorType::html_cannot_be_parsed, -1};
    temp_cur_pos++;
    cur_pos--;
    if (cur_pos <= temp_cur_pos) {
      cur_pos += 2;
      continue;
    }
    std::vector<char> temp_buf(cur_pos - temp_cur_pos + 1 + 1);
    strncpy(temp_buf.data(), temp_cur_pos, cur_pos - temp_cur_pos + 1);
    temp_buf[cur_pos - temp_cur_pos + 1] = '\0';
    cur_pos += 2;
    if (PathMatchSpecA(temp_buf.data(), "*.zip") == FALSE)
      continue;

    count++;
    out.push_back(to_wstring(temp_buf.data()));
  }
  return out;
}

std::optional<FtpWebOperationError> do_download_file_web_proxy(FtpOperationParams params, const std::wstring &target_path, ProgressData &progress_data,
                                                               const concurrency::cancellation_token &token) {
  auto result = ftp_web_proxy_login(params);
  if (auto error = std::get_if<FtpWebOperationError>(&result))
    return *error;

  auto handle = std::get<HINTERNET>(result);
  result = open_url(handle, L"ftp://" + params.address + L"/" + params.path, params);
  if (auto error = std::get_if<FtpWebOperationError>(&result))
    return *error;

  auto url_handle = std::get<HINTERNET>(result);

  std::vector<char> file_buffer;
  DWORD bytes_to_read = 0;
  DWORD bytes_read;
  DWORD bytes_read_total = 0;
  int file_handle = _wopen(target_path.c_str(), _O_CREAT | _O_BINARY | _O_WRONLY);
  if (file_handle == NULL)
    return FtpWebOperationError{FtpWebOperationErrorType::file_is_not_writeable, -1};

  get_progress()->set_marquee(true);
  while (true) {
    if (token.is_canceled()) {
      _close(file_handle);
      if (PathFileExists(target_path.c_str())) {
        WinApi::delete_file(target_path.c_str());
      }
      return FtpWebOperationError{FtpWebOperationErrorType::download_cancelled, -1};
    }
    InternetQueryDataAvailable(url_handle, &bytes_to_read, 0, 0);
    if (bytes_to_read == 0)
      break;
    if (bytes_to_read > file_buffer.size()) {
      file_buffer.resize(bytes_to_read);
    }

    InternetReadFile(url_handle, file_buffer.data(), bytes_to_read, &bytes_read);
    if (bytes_read == 0)
      break;

    write(file_handle, file_buffer.data(), bytes_read);
    bytes_read_total += bytes_read;

    progress_data.set(0, wstring_printf(rc_str(IDS_PD_BYTES_DOWNLOADED).c_str(), bytes_read_total), true);
  }
  _close(file_handle);
  return std::nullopt;
}

void DownloadDicsDlg::update_status(const wchar_t *text, COLORREF status_color) {
  m_status_color = status_color;
  Static_SetText(m_h_status, text);
}

void DownloadDicsDlg::on_new_file_list(std::vector<FileDescription> list) {
  int count = 0;

  for (auto &element : list) {
    count++;
    auto name = element;
  }

  if (count == 0) {
    return update_status(rc_str(IDS_STATUS_DIRECTORY_EMPTY).c_str(), COLOR_WARN);
  }

  m_current_list.clear();
  for (auto &element : list) {
    element.title = apply_alias(element.title, m_settings.language_name_style);
    m_current_list.push_back(element);
  }

  std::sort(m_current_list.begin(), m_current_list.end(), [](const FileDescription &lhs, const FileDescription &rhs) { return lhs.title < rhs.title; });
  update_list_box(); // Used only here and on filter change
  // If it is success when we perhaps should add this address to our list.
  if (m_check_if_saving_is_needed) {
    add_user_server(*current_address());
  }
  update_status(rc_str(IDS_STATUS_LIST_SUCCESS).c_str(), COLOR_OK);
  EnableWindow(m_h_install_selected, TRUE);
  update_download_button_availability();
}

void DownloadDicsDlg::preserve_current_address_index(Settings &settings) {
  auto mb_address = current_address();
  if (!mb_address)
    return;
  auto address = *mb_address;
  ftp_trim(address);
  int i = 0;
  for (auto def_server : m_default_server_names) {
    ftp_trim(def_server);
    if (address == def_server) {
      settings.last_used_address_index = i;
      return;
    }
    ++i;
  };
  i = 0;
  for (auto &server : m_settings.server_names) {
    if (address == server) {
      settings.last_used_address_index = USER_SERVER_CONST + i;
      return;
    }
    ++i;
  }
  settings.last_used_address_index = 0;
}

void DownloadDicsDlg::reset_download_combobox() {
  HWND target_combobox = GetDlgItem(getHSelf(), IDC_ADDRESS);
  wchar_t buf[default_buf_size];
  ComboBox_GetText(target_combobox, buf, default_buf_size);
  if (m_address_is_set) {
    auto mut_settings = m_settings.modify();
    preserve_current_address_index(*mut_settings);
  }
  ComboBox_ResetContent(target_combobox);
  for (auto &server_name : m_default_server_names) {
    ComboBox_AddString(target_combobox, server_name.c_str());
  }
  for (auto &server_name : m_settings.server_names) {
    if (!server_name.empty())
      ComboBox_AddString(target_combobox, server_name.c_str());
  }
  if (m_settings.last_used_address_index < USER_SERVER_CONST)
    ComboBox_SetCurSel(target_combobox, m_settings.last_used_address_index);
  else
    ComboBox_SetCurSel(target_combobox, m_settings.last_used_address_index - USER_SERVER_CONST + m_default_server_names.size());
  m_address_is_set = true;
}

void DownloadDicsDlg::add_user_server(std::wstring server) {
  {
    ftp_trim(server);
    for (auto def_server : m_default_server_names) {
      ftp_trim(def_server);
      if (server == def_server)
        goto add_user_server_cleanup; // Nothing is done in this case
    }
    for (auto added_server : m_settings.server_names) {
      ftp_trim(added_server);
      if (server == added_server)
        goto add_user_server_cleanup; // Nothing is done in this case
    }
    auto settings_mut = m_settings.modify();
    // Then we're adding finally
    auto &names = settings_mut->server_names;
    std::move(std::next(names.rbegin()), names.rend(), names.rbegin());
    names.front() = server;
  }
add_user_server_cleanup:
  reset_download_combobox();
}

void DownloadDicsDlg::process_file_list_error(FtpOperationErrorType error) {
  switch (error) {
  case FtpOperationErrorType::none:
    break;
  case FtpOperationErrorType::login_failed:
    return update_status(rc_str(IDS_STATUS_BAD_CONNECTION).c_str(), COLOR_FAIL);
  case FtpOperationErrorType::download_failed:
    return update_status(rc_str(IDS_STATUS_CANT_LIST_FILES).c_str(), COLOR_FAIL);
  case FtpOperationErrorType::download_cancelled:
    return update_status(rc_str(IDS_STATUS_DOWNLOAD_CANCELLED).c_str(), COLOR_WARN);
  }
}

void DownloadDicsDlg::process_file_list_error(const FtpWebOperationError &error) {
  switch (error.type) {
  case FtpWebOperationErrorType::none:
    break;
  case FtpWebOperationErrorType::http_client_cannot_be_initialized:
    return update_status(rc_str(IDS_STATUS_HTTP_CLIENT_INIT_FAIL).c_str(), COLOR_FAIL);
  case FtpWebOperationErrorType::url_cannot_be_opened:
    return update_status(rc_str(IDS_STATUS_URL_OPEN_FAIL).c_str(), COLOR_FAIL);
  case FtpWebOperationErrorType::querying_status_code_failed:
    return update_status(rc_str(IDS_STATUS_HTTP_CODE_QUERY_FAIL).c_str(), COLOR_FAIL);
  case FtpWebOperationErrorType::proxy_authorization_required:
    return update_status(rc_str(IDS_STATUS_PROXY_AUTH_REQUIRED).c_str(), COLOR_FAIL);
  case FtpWebOperationErrorType::http_error:
    return update_status(wstring_printf(rc_str(IDS_STATUS_HTTP_ERROR_PD).c_str(), error.status_code).c_str(), COLOR_FAIL);
  case FtpWebOperationErrorType::html_cannot_be_parsed:
    return update_status(rc_str(IDS_STATUS_HTML_PARSING_FAIL).c_str(), COLOR_FAIL);
  case FtpWebOperationErrorType::file_is_not_writeable:
    return update_status(rc_str(IDS_STATUS_FILE_CANNOT_BE_WRITTEN).c_str(), COLOR_FAIL);
  case FtpWebOperationErrorType::download_cancelled:
    return update_status(rc_str(IDS_STATUS_FILE_CANNOT_BE_WRITTEN).c_str(), COLOR_WARN);
  }
}

void DownloadDicsDlg::prepare_file_list_update() {
  EnableWindow(m_h_install_selected, FALSE);
  m_status_color = COLOR_NEUTRAL;
  Static_SetText(m_h_status, rc_str(IDS_STATUS_LOADING).c_str());
  ListBox_ResetContent(m_h_file_list);
  m_current_list.clear();
}

FtpOperationParams DownloadDicsDlg::spawn_ftp_operation_params(const std::wstring &full_path) const {
  FtpOperationParams params;
  std::tie(params.address, params.path) = ftp_split(full_path);
  params.use_proxy = m_settings.use_proxy;
  params.proxy_port = m_settings.proxy_port;
  params.proxy_address = m_settings.proxy_host_name;
  params.anonymous = m_settings.proxy_is_anonymous;
  params.proxy_username = m_settings.proxy_user_name;
  params.proxy_password = m_settings.proxy_password;
  params.write_debug_log = m_settings.write_debug_log;
  params.debug_log_path = get_debug_log_path();
  params.use_passive_mode = m_settings.ftp_use_passive_mode;
  return params;
}

static std::vector<FileDescription> transform_zip_list(const std::vector<std::wstring> &list) {
  std::vector<FileDescription> l;
  for (auto &f : list) {
    if (!PathMatchSpec(f.c_str(), L"*.zip"))
      continue;
    FileDescription fd;
    fd.path = f;
    fd.title = f.substr(0, f.length() - 4);
    l.push_back(fd);
  }
  return l;
}

void DownloadDicsDlg::update_file_list_async_web_proxy(const std::wstring &full_path) {
  // temporary workaround for xsmf_control.h bug
  static_assert(std::is_copy_constructible_v<std::variant<FtpOperationErrorType, std::vector<std::wstring>>>);
  m_ftp_operation_task->do_deferred([params = spawn_ftp_operation_params(full_path)](auto) { return do_download_file_list_ftp_web_proxy(params); },
                                    [this](std::variant<FtpWebOperationError, std::vector<std::wstring>> res) {
                                      if (auto error = std::get_if<FtpWebOperationError>(&res)) {
                                        return this->process_file_list_error(*error);
                                      }
                                      auto &list = std::get<std::vector<std::wstring>>(res);
                                      on_new_file_list(transform_zip_list(list));
                                    });
}

void DownloadDicsDlg::update_file_list_async(const std::wstring &full_path) {
  // temporary workaround for xsmf_control.h bug
  static_assert(std::is_copy_constructible_v<std::variant<FtpOperationErrorType, std::vector<std::wstring>>>);
  m_ftp_operation_task->do_deferred([params = spawn_ftp_operation_params(full_path)](auto) { return do_download_file_list_ftp(params); },
                                    [this](std::variant<FtpOperationErrorType, std::vector<std::wstring>> res) {
                                      if (auto error = std::get_if<FtpOperationErrorType>(&res)) {
                                        return this->process_file_list_error(*error);
                                      }
                                      on_new_file_list(transform_zip_list(std::get<std::vector<std::wstring>>(res)));
                                    });
}

void DownloadDicsDlg::download_file_async(const std::wstring &full_path, const std::wstring &target_location) {
  m_ftp_operation_task->do_deferred([params = spawn_ftp_operation_params(full_path), target_location, progressData = get_progress()->get_progress_data()](
                                        auto token) { return do_download_file(params, target_location, progressData, token); },
                                    [this](std::optional<FtpOperationErrorType>) { on_zip_file_downloaded(); });
}

void DownloadDicsDlg::download_file_async_web_proxy(const std::wstring &full_path, const std::wstring &target_location) {
  m_ftp_operation_task->do_deferred([params = spawn_ftp_operation_params(full_path), target_location, progressData = get_progress()->get_progress_data()](
                                        auto token) { return do_download_file_web_proxy(params, target_location, *progressData, token); },
                                    [this](std::optional<FtpWebOperationError>) { on_zip_file_downloaded(); });
}

void DownloadDicsDlg::refresh() {
  indicate_that_saving_might_be_needed();
  on_display_action();
  KillTimer(_hSelf, refresh_timer_id);
}

void DownloadDicsDlg::update_controls() {
  Button_SetCheck(m_h_show_only_known, m_settings.download_show_only_recognized_dictionaries ? BST_CHECKED : BST_UNCHECKED);
  Button_SetCheck(m_h_install_system, m_settings.download_install_dictionaries_for_all_users ? BST_CHECKED : BST_UNCHECKED);
}

void DownloadDicsDlg::update_settings(Settings &settings) {
  settings.download_show_only_recognized_dictionaries = Button_GetCheck(m_h_show_only_known) == BST_CHECKED;
  settings.download_install_dictionaries_for_all_users = Button_GetCheck(m_h_install_system) == BST_CHECKED;
}

void DownloadDicsDlg::update_download_button_availability() {
  int cnt = 0;
  for (int i = 0; i < ListBox_GetCount(m_h_file_list); i++)
    cnt += ((CheckedListBox_GetCheckState(m_h_file_list, i) == BST_CHECKED) ? 1 : 0);
  EnableWindow(m_h_install_selected, cnt > 0 ? TRUE : FALSE);
}

INT_PTR DownloadDicsDlg::run_dlg_proc(UINT message, WPARAM w_param, LPARAM l_param) {
  switch (message) {
  case WM_TIMER: {
    switch (w_param) {
    case refresh_timer_id:
      refresh();
      return TRUE;
    }
  }
  case WM_INITDIALOG: {
    m_h_file_list = ::GetDlgItem(_hSelf, IDC_FILE_LIST);
    m_h_address = ::GetDlgItem(_hSelf, IDC_ADDRESS);
    m_h_status = ::GetDlgItem(_hSelf, IDC_SERVER_STATUS);
    m_h_install_selected = ::GetDlgItem(_hSelf, IDOK);
    m_h_show_only_known = ::GetDlgItem(_hSelf, IDC_SHOWONLYKNOWN);
    m_h_refresh_btn = ::GetDlgItem(_hSelf, IDC_REFRESH);
    m_h_install_system = ::GetDlgItem(_hSelf, IDC_INSTALL_SYSTEM);
    RECT rc;
    GetClientRect(m_h_refresh_btn, &rc);
    int icon_size = std::min(rc.bottom - rc.top, rc.right - rc.left) * 4 / 5;
    m_refresh_icon = static_cast<HICON>(LoadImage(_hInst, MAKEINTRESOURCE(IDI_REFRESH), IMAGE_ICON, icon_size, icon_size, 0));
    SendMessage(m_h_refresh_btn, BM_SETIMAGE, static_cast<WPARAM>(IMAGE_ICON), reinterpret_cast<LPARAM>(m_refresh_icon));
    WinApi::create_tooltip(IDC_REFRESH, _hSelf, rc_str(IDS_REFRESH_DICTIONARY_LIST_TOOLTIP).c_str());
    reset_download_combobox();
    fill_file_list();
    update_controls();
    m_default_brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    update_download_button_availability();
  }
    return TRUE;
  case WM_COMMAND: {
    switch (LOWORD(w_param)) {
    case IDC_FILE_LIST:
      if (HIWORD(w_param) == LBCN_ITEMCHECK) {
        update_download_button_availability();
      }
      break;
    case IDOK:
      if (HIWORD(w_param) == BN_CLICKED) {
        download_selected();
        get_progress()->do_dialog();
        display(false);
      }
      break;
    case IDCANCEL:
      if (HIWORD(w_param) == BN_CLICKED) {
        for (int i = 0; i < ListBox_GetCount(m_h_file_list); i++)
          CheckedListBox_SetCheckState(m_h_file_list, i, BST_UNCHECKED);
        display(false);
      }

      break;
    case IDC_ADDRESS:
      if (HIWORD(w_param) == CBN_EDITCHANGE) {
        SetTimer(_hSelf, refresh_timer_id, 1000, nullptr);
      } else if (HIWORD(w_param) == CBN_SELCHANGE) {
        {
          auto mut_settings = m_settings.modify();
          preserve_current_address_index(*mut_settings);
        }
        refresh();
        m_check_if_saving_is_needed = false;
      }
      break;
    case IDC_REFRESH: {
      refresh();
    } break;
    case IDC_INSTALL_SYSTEM:
      if (HIWORD(w_param) == BN_CLICKED) {
        auto mut_settings = m_settings.modify();
        update_settings(*mut_settings);
      }
      break;
    case IDC_SHOWONLYKNOWN:
      if (HIWORD(w_param) == BN_CLICKED) {
        auto mut_settings = m_settings.modify();
        update_settings(*mut_settings);
      }
      break;
    case IDC_SELECTPROXY:
      if (HIWORD(w_param) == BN_CLICKED) {
        get_select_proxy()->do_dialog();
        get_select_proxy()->display();
      }
    }
  } break;
  case WM_CTLCOLORSTATIC:
    if (m_h_status == reinterpret_cast<HWND>(l_param)) {
      auto h_dc = reinterpret_cast<HDC>(w_param);
      SetBkColor(h_dc, GetSysColor(COLOR_BTNFACE));
      SetTextColor(h_dc, m_status_color);
      return reinterpret_cast<INT_PTR>(m_default_brush);
    }
    break;
  case WM_CLOSE:
    DeleteObject(m_default_brush);
    break;
  };
  return FALSE;
}
