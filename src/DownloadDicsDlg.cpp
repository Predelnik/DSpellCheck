/*
This file is part of DSpellCheck Plug-in for Notepad++
Copyright (C)2013 Sergey Semushin <Predelnik@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <io.h>
#include <fcntl.h>

#include "CheckedList/CheckedList.h"
#include "CommonFunctions.h"
#include "Definements.h"
#include "DownloadDicsDlg.h"

#include "FTPClient.h"
#include "FTPDataTypes.h"
#include "LanguageName.h"
#include "Plugin.h"
#include "ProgressDlg.h"
#include "resource.h"
#include "SelectProxyDialog.h"
#include "SpellChecker.h"
#include "MainDef.h"
#include "unzip.h"
#include "ProgressData.h"
#include <variant>
#include <Wininet.h>

void DownloadDicsDlg::do_dialog() {
    if (!isCreated()) {
        create(IDD_DOWNLOADDICS);
    }
    else {
        goToCenter();
        display();
    }
    SetFocus(m_h_file_list);
}

void DownloadDicsDlg::fill_file_list() {
    get_download_dics()->do_ftp_operation(FtpOperationType::fill_file_list, current_address()->c_str());
}

void DownloadDicsDlg::on_display_action() {
    fill_file_list();
}

DownloadDicsDlg::DownloadDicsDlg(): m_default_brush(nullptr), m_status_color(0), m_spell_checker_instance(nullptr),
                                    m_lib_combo(nullptr), m_h_status(nullptr), m_h_install_selected(nullptr),
                                    m_h_show_only_known(nullptr), m_h_refresh(nullptr), m_h_install_system(nullptr),
                                    m_cancel_pressed(false), m_failure(0), m_downloaded_count(0),
                                    m_supposed_downloaded_count(0) {
    m_timer = nullptr;
    m_refresh_icon = nullptr;
    m_check_if_saving_is_needed = 0;
    m_h_file_list = nullptr;
}

void DownloadDicsDlg::init_dlg(HINSTANCE h_inst, HWND parent,
                               SpellChecker* spell_checker_instance_arg) {
    m_ftp_operation_task = TaskWrapper{parent};
    m_spell_checker_instance = spell_checker_instance_arg;
    return Window::init(h_inst, parent);
}

DownloadDicsDlg::~DownloadDicsDlg() {
    if (m_timer)
        DeleteTimerQueueTimer(nullptr, m_timer, nullptr);
    if (m_refresh_icon)
        DestroyIcon(m_refresh_icon);
}

void DownloadDicsDlg::indicate_that_saving_might_be_needed() {
    m_check_if_saving_is_needed = 1;
}

LRESULT DownloadDicsDlg::ask_replacement_message(const wchar_t* dic_name) {
    std::wstring name;
    std::tie(name, std::ignore) = apply_alias(dic_name);
    return MessageBox(_hParent, wstring_printf(
                          L"Looks like %s dictionary is already present. Do you want to replace it?",
                          name.c_str()
                      ).c_str(), L"Dictionary already exists", MB_YESNO);
}

static std::wstring get_temp_path() {
    std::vector<wchar_t> temp_path_buf(MAX_PATH);
    GetTempPath(MAX_PATH, temp_path_buf.data());
    return temp_path_buf.data();
}

bool DownloadDicsDlg::prepare_downloading() {
    m_downloaded_count = 0;
    m_supposed_downloaded_count = 0;
    m_failure = 0;
    m_message = L"Dictionaries copied:\n";
    m_to_download.clear();

    // If path isn't exist we're gonna try to create it else it's finish
    if (!check_for_directory_existence(get_temp_path().c_str(), false, _hParent)) {
        MessageBox(
            _hParent, L"Path defined as temporary dir doesn't exist and couldn't "
            L"be created, probably one of subdirectories have limited "
            L"access, please make temporary path valid.",
            L"Temporary Path is Broken", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    m_cancel_pressed = false;
    ProgressDlg* p = get_progress();
    p->get_progress_data()->set(0, L"");
    get_progress()->update();
    p->set_top_message(L"");
    if (!check_for_directory_existence(
        m_spell_checker_instance->get_install_system()
            ? m_spell_checker_instance->get_hunspell_additional_path()
            : m_spell_checker_instance->get_hunspell_path(),
        false, _hParent)) // If path doesn't exist we're gonna try to create it
        // else it's finish
    {
        MessageBox(
            _hParent, L"Directory for dictionaries doesn't exist and couldn't be "
            L"created, probably one of subdirectories have limited "
            L"access, please choose accessible directory for "
            L"dictionaries",
            L"Dictionaries Haven't Been Downloaded", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    for (int i = 0; i < ListBox_GetCount(m_h_file_list); i++) {
        if (CheckedListBox_GetCheckState(m_h_file_list, i) == BST_CHECKED) {
            DownloadRequest req;
            req.file_name = m_current_langs_filtered[i].orig_name + L".zip"s;
            req.target_path = get_temp_path() + req.file_name;
            m_to_download.push_back(req);
            ++m_supposed_downloaded_count;
        }
    }
    m_cur = m_to_download.begin();
    return true;
}

void DownloadDicsDlg::finalize_downloading() {
    get_progress()->display(false);
    if (m_failure == 1) {
        MessageBox(
            _hParent, L"Access denied to dictionaries directory, either try to run "
            L"Notepad++ as administrator or select some different, "
            L"accessible dictionary path",
            L"Dictionaries Haven't Been Downloaded", MB_OK | MB_ICONEXCLAMATION);
    }
    else if (m_downloaded_count) {
        MessageBox(_hParent, m_message.c_str(), L"Dictionaries Downloaded",
                   MB_OK | MB_ICONINFORMATION);
    }
    else if (m_supposed_downloaded_count) // Otherwise - silent
    {
        MessageBox(
            _hParent, L"Sadly, no dictionaries were copied, please try again",
            L"Dictionaries Haven't Been Downloaded", MB_OK | MB_ICONEXCLAMATION);
    }
    for (int i = 0; i < ListBox_GetCount(m_h_file_list); i++)
        CheckedListBox_SetCheckState(m_h_file_list, i, BST_UNCHECKED);
    m_spell_checker_instance->hunspell_reinit_settings(
        true); // Calling the update for Hunspell dictionary list
    m_spell_checker_instance->reinit_language_lists(true);
    m_spell_checker_instance->do_plugin_menu_inclusion();
    m_spell_checker_instance->recheck_visible_both_views();
}

static const auto buf_size_for_copy = 10240;

void DownloadDicsDlg::on_file_downloaded() {
    wchar_t prog_message[DEFAULT_BUF_SIZE];
    std::map<std::string, int> files_found; // 0x01 - .aff found, 0x02 - .dic found
    auto local_path_ansi = to_string(m_cur->target_path.c_str());
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
            auto local_dic_file_handle =
                _wopen(dic_file_local_path.c_str(), _O_CREAT | _O_BINARY | _O_WRONLY);
            if (local_dic_file_handle == -1)
                continue;

            swprintf(prog_message, L"Extracting %s...", dic_file_name.c_str());
            get_progress()->set_top_message(prog_message);
            DWORD bytes_total = 0;
            int bytes_copied;
            std::vector<char> file_copy_buf(buf_size_for_copy);
            while ((bytes_copied = unzReadCurrentFile(fp, file_copy_buf.data(),
                                                      static_cast<unsigned int>(file_copy_buf.size()))) != 0) {
                _write(local_dic_file_handle, file_copy_buf.data(), bytes_copied);
                bytes_total += bytes_copied;
                swprintf(prog_message, L"%d / %d bytes extracted (%d %%)",
                         bytes_total, f_info.uncompressed_size,
                         bytes_total * 100 / f_info.uncompressed_size);
                get_progress()->get_progress_data()->set(bytes_total * 100 / f_info.uncompressed_size, prog_message);
            }
            unzCloseCurrentFile(fp);
            _close(local_dic_file_handle);
        }
    }
    while (unzGoToNextFile(fp) == UNZ_OK);
    // Now we're gonna check what's exactly we extracted with using FilesFound
    // map
    for (auto& p : files_found) {
        auto& file_name_ansi = p.first; // TODO: change to structured binding when Resharper supports them
        auto mask = p.second;
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
            SetFileAttributes(dic_file_local_path.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFile(dic_file_local_path.c_str());
        }
        else {
            auto dic_file_local_path = get_temp_path();
            (dic_file_local_path += file_name) += L".aff";
            std::wstring hunspell_dic_path =
                m_spell_checker_instance->get_install_system()
                    ? m_spell_checker_instance->get_hunspell_additional_path()
                    : m_spell_checker_instance->get_hunspell_path();
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
                    SetFileAttributes(dic_file_local_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(dic_file_local_path.c_str());
                }
                else {
                    SetFileAttributes(hunspell_dic_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(hunspell_dic_path.c_str());
                }
            }

            if (confirmation && !MoveFile(dic_file_local_path.c_str(), hunspell_dic_path.c_str())) {
                SetFileAttributes(dic_file_local_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFile(dic_file_local_path.c_str());
                m_failure = 1;
            }
            dic_file_local_path = dic_file_local_path.substr(0, dic_file_local_path.length() - 4) + L".dic";
            hunspell_dic_path = hunspell_dic_path.substr(0, hunspell_dic_path.length() - 4) + L".dic";
            if (!confirmation) {
                SetFileAttributes(dic_file_local_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFile(dic_file_local_path.c_str());
            }
            else if (PathFileExists(hunspell_dic_path.c_str())) {
                int res;
                if (replace_question_was_asked)
                    res = !confirmation;
                else {
                    res = (ask_replacement_message(file_name.c_str()) == IDNO);
                }
                if (res) {
                    SetFileAttributes(dic_file_local_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(dic_file_local_path.c_str());
                    confirmation = false;
                }
                else {
                    SetFileAttributes(hunspell_dic_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(hunspell_dic_path.c_str());
                }
            }

            if (confirmation && !MoveFile(dic_file_local_path.c_str(), hunspell_dic_path.c_str())) {
                SetFileAttributes(dic_file_local_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFile(dic_file_local_path.c_str());
                m_failure = 1;
            }
            std::wstring converted_dic_name;
            std::tie(converted_dic_name, std::ignore) = apply_alias(file_name);
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
    SetFileAttributes(m_cur->target_path.c_str(), FILE_ATTRIBUTE_NORMAL);
    DeleteFile(m_cur->target_path.c_str()); // Removing downloaded .zip file
    if (m_failure)
        return finalize_downloading();

    ++m_cur;
    start_next_download();
}

void DownloadDicsDlg::start_next_download() {
    if (m_cur == m_to_download.end() || m_cancel_pressed)
        return finalize_downloading();

    get_progress()->set_top_message(wstring_printf(L"Downloading %s...", m_cur->file_name.c_str()).c_str());
    if (PathFileExists(m_cur->target_path.c_str())) {
        SetFileAttributes(m_cur->target_path.c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFile(m_cur->target_path.c_str());
    }
    do_ftp_operation(FtpOperationType::download_file, *current_address(), m_cur->file_name, m_cur->target_path);
}

void DownloadDicsDlg::download_selected() {
    if (!prepare_downloading())
        return;

    start_next_download();
}

void ftp_trim(std::wstring& ftp_address) {
    trim(ftp_address);
    for (auto& c : ftp_address) {
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
    if (!m_h_file_list || m_current_langs.empty())
        return;

    m_current_langs_filtered.clear();
    for (auto& lang : m_current_langs) {
        if (m_spell_checker_instance->get_show_only_known() &&
            !lang.alias_applied)
            continue;
        m_current_langs_filtered.push_back(lang);
    }
    ListBox_ResetContent(m_h_file_list);
    for (auto& lang : m_current_langs_filtered) {
        ListBox_AddString(m_h_file_list, m_spell_checker_instance->get_decode_names()
            ? lang.alias_name.c_str ()
            : lang.orig_name.c_str ());
    }
}

// Copy of CFile with ability to kick progress bar
class Observer : public nsFTP::CFTPClient::CNotification {
    std::shared_ptr<ProgressData> m_progress_data;
    std::wstring m_target_path;
    long m_bytes_received = 0;
    long m_target_size;
    concurrency::cancellation_token m_token;
    nsFTP::CFTPClient& m_client;

public:
    Observer(std::shared_ptr<ProgressData> progress_data, const std::wstring& target_path,
             long target_size, nsFTP::CFTPClient& client,
             concurrency::cancellation_token token)
        : m_progress_data(progress_data), m_target_path(target_path), m_target_size(target_size), m_token(token),
          m_client(client) {
    }

    virtual void OnBytesReceived(const nsFTP::TByteVector& /*vBuffer*/,
                                 long l_received_bytes) override {
        if (m_token.is_canceled()) {
            DeleteFile(m_target_path.c_str());
            m_client.Logout();
            return;
        }
        m_bytes_received += l_received_bytes;
        m_progress_data->set(m_bytes_received * 100 / m_target_size,
                             wstring_printf(L"%d / %d bytes downloaded (%d %%)", m_bytes_received,
                                            m_target_size, m_bytes_received * 100 / m_target_size));
    }
};

void DownloadDicsDlg::set_cancel_pressed(bool value) {
    m_cancel_pressed = value;
    m_ftp_operation_task->cancel();
    m_cur = m_to_download.end();
    finalize_downloading();
}

#define INITIAL_BUFFER_SIZE 50 * 1024
#define INITIAL_SMALL_BUFFER_SIZE 10 * 1024

std::optional<std::wstring> DownloadDicsDlg::current_address() const {
    if (!m_h_address)
        return std::nullopt;
    auto sel = ComboBox_GetCurSel (m_h_address);
    if (sel < 0) {
        auto text_len = ComboBox_GetTextLength (m_h_address);
        std::vector<wchar_t> buf(text_len + 1);
        ComboBox_GetText (m_h_address, buf.data (), text_len + 1);
        return buf.data();
    }
    auto text_len = ComboBox_GetLBTextLen (m_h_address, sel);
    std::vector<wchar_t> buf(text_len + 1);
    ComboBox_GetLBText (m_h_address, sel, buf.data ());
    return buf.data();
}


static bool ftp_login(nsFTP::CFTPClient& client, const FtpOperationParams& params) {
    std::unique_ptr<nsFTP::CLogonInfo> logon_info;
    if (!params.use_proxy)
        logon_info = std::make_unique<nsFTP::CLogonInfo>(params.address);
    else
        logon_info = std::make_unique<nsFTP::CLogonInfo>(
            params.address, USHORT(21), L"anonymous", L"", L"",
            params.proxy_address, L"", L"",
            static_cast<USHORT>(params.proxy_port),
            nsFTP::CFirewallType::UserWithNoLogon());
    return client.Login(*logon_info);
}


std::variant<FtpOperationErrorType, std::vector<std::wstring>> do_download_file_list_ftp(FtpOperationParams params) {
    nsFTP::CFTPClient client(nsSocket::CreateDefaultBlockingSocketInstance(), 3);
    if (!ftp_login(client, params))
        return FtpOperationErrorType::login_failed;

    nsFTP::TFTPFileStatusShPtrVec list;
    if (!client.List(params.path, list, true))
        return FtpOperationErrorType::download_failed;

    std::vector<std::wstring> ret(list.size());
    std::transform(list.begin(), list.end(), ret.begin(),
                   [](const nsFTP::TFTPFileStatusShPtr& status) { return status->Name(); });
    return ret;
}

std::optional<FtpOperationErrorType> do_download_file(FtpOperationParams params, const std::wstring& target_path,
                                                      std::shared_ptr<ProgressData> progress_data, concurrency::
                                                      cancellation_token token) {
    nsFTP::CFTPClient client(nsSocket::CreateDefaultBlockingSocketInstance(), 3);
    if (!ftp_login(client, params))
        return FtpOperationErrorType::login_failed;

    long file_size = 0;
    if (client.FileSize(params.path, file_size) != nsFTP::FTP_OK)
        return FtpOperationErrorType::download_failed;

    auto progress_updater = std::make_unique<Observer>(std::move(progress_data), target_path, file_size, client, token);
    client.AttachObserver(progress_updater.get());

    if (!client.DownloadFile(params.path, target_path,
                             nsFTP::CRepresentation(nsFTP::CType::Image()),
                             true)) {
        if (PathFileExists(target_path.c_str())) {
            SetFileAttributes(target_path.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFile(target_path.c_str());
            return FtpOperationErrorType::download_cancelled;
        }
    }

    client.DetachObserver(progress_updater.get());
    return std::nullopt;
}

static std::variant<HINTERNET, FtpWebOperationError> open_url(HINTERNET win_inet_handle, const std::wstring& url,
                                                              const FtpOperationParams& params) {
    const auto url_handle = InternetOpenUrl(win_inet_handle, url.c_str(), nullptr, 0,
                                            INTERNET_FLAG_PASSIVE | INTERNET_FLAG_RELOAD |
                                            INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    if (!url_handle)
        return FtpWebOperationError{FtpWebOperationErrorType::url_cannot_be_opened, -1};

    if (!params.anonymous) {
        InternetSetOption(
            url_handle, INTERNET_OPTION_PROXY_USERNAME,
            const_cast<wchar_t *>(params.proxy_username.c_str()),
            static_cast<DWORD>(params.proxy_username.length() + 1));
        InternetSetOption(
            url_handle, INTERNET_OPTION_PROXY_PASSWORD,
            const_cast<wchar_t *>(params.proxy_password.c_str()),
            static_cast<DWORD>(params.proxy_password.length() + 1));
        InternetSetOption(url_handle, INTERNET_OPTION_PROXY_SETTINGS_CHANGED, nullptr, 0);
    }
    HttpSendRequest(url_handle, nullptr, 0, nullptr, 0);

    DWORD code = 0;
    DWORD dummy = 0;
    DWORD size = sizeof(DWORD);

    if (!HttpQueryInfo(url_handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       &code, &size, &dummy))
        return FtpWebOperationError{FtpWebOperationErrorType::querying_status_code_failed, -1};

    if (code != HTTP_STATUS_OK) {
        if (code == HTTP_STATUS_PROXY_AUTH_REQ)
            return FtpWebOperationError{FtpWebOperationErrorType::proxy_authorization_required, static_cast<int>(code)};
        else {
            return FtpWebOperationError{FtpWebOperationErrorType::http_error, static_cast<int>(code)};
        }
    }
    return url_handle;
}

static std::variant<HINTERNET, FtpWebOperationError> ftp_web_proxy_login(const FtpOperationParams& params) {
    std::wstring proxy_final_string = params.proxy_address + L":" + std::to_wstring(params.proxy_port);
    const auto win_inet_handle = InternetOpen(L"DSpellCheck", INTERNET_OPEN_TYPE_PROXY, proxy_final_string.c_str(), L"",
                                              0);
    if (!win_inet_handle)
        return FtpWebOperationError{FtpWebOperationErrorType::http_client_cannot_be_initialized, -1};
    DWORD time_out = 15000;
    for (auto option : {INTERNET_OPTION_CONNECT_TIMEOUT, INTERNET_OPTION_SEND_TIMEOUT, INTERNET_OPTION_RECEIVE_TIMEOUT})
        InternetSetOption(win_inet_handle, option, &time_out, sizeof(DWORD));
    return win_inet_handle;
}

std::variant<FtpWebOperationError, std::vector<std::wstring>> do_download_file_list_ftp_web_proxy(
    FtpOperationParams params) {
    auto result = ftp_web_proxy_login(params);
    if (auto error = std::get_if<FtpWebOperationError>(&result))
        return *error;

    auto handle = std::get<HINTERNET>(result);
    result = open_url(handle, L"ftp://" + params.address + L"/" + params.path, params);
    if (auto error = std::get_if<FtpWebOperationError>(&result))
        return *error;

    auto url_handle = std::get<HINTERNET>(result);

    std::vector<char> file_buffer(INITIAL_BUFFER_SIZE);
    std::size_t cur_index = 0;
    DWORD bytes_read = 0;
    DWORD bytes_read_total = 0;
    DWORD bytes_to_read = 0;
    unsigned int cur_buf_size = INITIAL_BUFFER_SIZE;
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
    while ((size_t)(cur_pos - file_buffer.data()) < bytes_read_total) {
        char* temp_cur_pos = cur_pos;
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
        if (!PathMatchSpecA(temp_buf.data(), "*.zip"))
            continue;

        count++;
        out.push_back(to_wstring(temp_buf.data()));
    }
    return out;
}

std::optional<FtpWebOperationError> do_download_file_web_proxy(FtpOperationParams params,
                                                               const std::wstring& target_path,
                                                               std::shared_ptr<ProgressData> progress_data,
                                                               concurrency::
                                                               cancellation_token token) {
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
    if (!file_handle)
        return FtpWebOperationError{FtpWebOperationErrorType::file_is_not_writeable, -1};

    get_progress()->set_marquee(true);
    while (true) {
        if (token.is_canceled()) {
            _close(file_handle);
            if (PathFileExists(target_path.c_str())) {
                SetFileAttributes(target_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFile(target_path.c_str());
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

        progress_data->set(0, wstring_printf(L"%d / ???   bytes downloaded", bytes_read_total), true);
    }
    _close(file_handle);
    return std::nullopt;
}


void DownloadDicsDlg::update_status(const wchar_t* text, COLORREF status_color) {
    m_status_color = status_color;
    Static_SetText(m_h_status, text);
}

void DownloadDicsDlg::ui_update() {
    get_progress()->update();
}

void DownloadDicsDlg::on_new_file_list(const std::vector<std::wstring>& list) {
    int count = 0;

    for (unsigned int i = 0; i < list.size(); i++) {
        if (!PathMatchSpec(list[i].c_str(), L"*.zip"))
            continue;

        count++;
        auto name = list[i];
        name.erase(name.end() - 4, name.end());
        LanguageName lang(name.c_str());
        m_current_langs.push_back(lang);
    }

    if (count == 0) {
        return update_status(L"Status: Directory doesn't contain any zipped files", COLOR_WARN);
    }

    std::sort(m_current_langs.begin(), m_current_langs.end(), [decode = m_spell_checker_instance->get_decode_names()]
          (const auto& lhs, const auto& rhs)
              {
                  return decode ? less_aliases(lhs, rhs) : less_original(lhs, rhs);
              });
    update_list_box(); // Used only here and on filter change
    // If it is success when we perhaps should add this address to our list.
    if (m_check_if_saving_is_needed) {
        m_spell_checker_instance->add_user_server(*current_address());
    }
    update_status(L"Status: List of available files was successfully loaded", COLOR_OK);
    EnableWindow(m_h_install_selected, true);
}

void DownloadDicsDlg::process_file_list_error(FtpOperationErrorType error) {
    switch (error) {
    case FtpOperationErrorType::none: break;
    case FtpOperationErrorType::login_failed: return update_status(L"Status: Connection cannot be established",
                                                                   COLOR_FAIL);
    case FtpOperationErrorType::download_failed: return
            update_status(L"Status: Can't list directory files", COLOR_FAIL);
    case FtpOperationErrorType::download_cancelled: return update_status(L"Status: Download Cancelled", COLOR_WARN);
    }
}

void DownloadDicsDlg::process_file_list_error(const FtpWebOperationError& error) {
    switch (error.type) {
    case FtpWebOperationErrorType::none: break;
    case FtpWebOperationErrorType::http_client_cannot_be_initialized: return update_status(
            L"Status: HTTP client cannot be initialize", COLOR_FAIL);
    case FtpWebOperationErrorType::url_cannot_be_opened: return update_status(
            L"Status: URL cannot be opneed", COLOR_FAIL);
    case FtpWebOperationErrorType::querying_status_code_failed: return update_status(
            L"Status: Querying status code failed", COLOR_FAIL);
    case FtpWebOperationErrorType::proxy_authorization_required: return update_status(
            L"Status: Proxy authorization required", COLOR_FAIL);
    case FtpWebOperationErrorType::http_error: return update_status(
            wstring_printf(L"Status: HTTP error %d", error.status_code).c_str(), COLOR_FAIL);
    case FtpWebOperationErrorType::html_cannot_be_parsed:
        return update_status(L"Status: HTML cannot be parsed", COLOR_FAIL);
    case FtpWebOperationErrorType::file_is_not_writeable:
        return update_status(L"Status: Target file cannot be written", COLOR_FAIL);
    case FtpWebOperationErrorType::download_cancelled:
        return update_status(L"Status: Target file cannot be written", COLOR_WARN);
    }
}

void DownloadDicsDlg::prepare_file_list_update() {
    EnableWindow(m_h_install_selected, false);
    m_status_color = COLOR_NEUTRAL;
    Static_SetText(m_h_status, L"Status: Loading...");
    ListBox_ResetContent(m_h_file_list);
    m_current_langs.clear();;
}

FtpOperationParams DownloadDicsDlg::spawn_ftp_operation_params(const std::wstring& full_path) {
    FtpOperationParams params;
    std::tie(params.address, params.path) = ftp_split(full_path);
    params.use_proxy = m_spell_checker_instance->get_use_proxy();
    params.proxy_port = m_spell_checker_instance->get_proxy_port();
    params.proxy_address = m_spell_checker_instance->get_proxy_host_name();
    params.anonymous = m_spell_checker_instance->get_proxy_anonymous();
    params.proxy_username = m_spell_checker_instance->get_proxy_user_name();
    params.proxy_password = m_spell_checker_instance->get_proxy_password();
    return params;
}

void DownloadDicsDlg::update_file_list_async_web_proxy(const std::wstring& full_path) {
    // temporary workaround for xsmf_control.h bug
    static_assert(std::is_copy_constructible_v<std::variant<FtpOperationErrorType, std::vector<std::wstring>>>);
    prepare_file_list_update();
    m_ftp_operation_task->do_deferred([params = spawn_ftp_operation_params(full_path)](auto)
                                     {
                                         return do_download_file_list_ftp_web_proxy(params);
                                     }, [this](std::variant<FtpWebOperationError, std::vector<std::wstring>> res)
                                     {
                                         if (auto error = std::get_if<FtpWebOperationError>(&res)) {
                                             return this->process_file_list_error(*error);
                                         }
                                         on_new_file_list(std::get<std::vector<std::wstring>>(res));
                                     });
}

void DownloadDicsDlg::update_file_list_async(const std::wstring& full_path) {
    // temporary workaround for xsmf_control.h bug
    static_assert(std::is_copy_constructible_v<std::variant<FtpOperationErrorType, std::vector<std::wstring>>>);
    prepare_file_list_update();
    m_ftp_operation_task->do_deferred([params = spawn_ftp_operation_params(full_path)](auto)
                                     {
                                         return do_download_file_list_ftp(params);
                                     }, [this](std::variant<FtpOperationErrorType, std::vector<std::wstring>> res)
                                     {
                                         if (auto error = std::get_if<FtpOperationErrorType>(&res)) {
                                             return this->process_file_list_error(*error);
                                         }
                                         on_new_file_list(std::get<std::vector<std::wstring>>(res));
                                     });
}

void DownloadDicsDlg::download_file_async(const std::wstring& full_path, const std::wstring& target_location) {
    m_ftp_operation_task->do_deferred([params = spawn_ftp_operation_params(full_path), target_location,
                                         progressData = get_progress()->get_progress_data()](auto token)
                                     {
                                         return do_download_file(params, target_location, progressData, token);
                                     }, [this](std::optional<FtpOperationErrorType>)
                                     {
                                         on_file_downloaded();
                                     });
}


void DownloadDicsDlg::
download_file_async_web_proxy(const std::wstring& full_path, const std::wstring& target_location) {
    m_ftp_operation_task->do_deferred([params = spawn_ftp_operation_params(full_path), target_location,
                                         progressData = get_progress()->get_progress_data()](auto token)
                                     {
                                         return do_download_file_web_proxy(
                                             params, target_location, progressData, token);
                                     }, [this](std::optional<FtpWebOperationError>)
                                     {
                                         on_file_downloaded();
                                     });
}

void DownloadDicsDlg::do_ftp_operation(FtpOperationType type, const std::wstring& full_path,
                                       const std::wstring& file_name, const std::wstring& location) {
    if (m_spell_checker_instance->get_use_proxy() &&
        m_spell_checker_instance->get_proxy_type() == 0)
        switch (type) {
        case FtpOperationType::fill_file_list: return update_file_list_async_web_proxy(full_path);
        case FtpOperationType::download_file: return download_file_async_web_proxy(full_path + file_name, location);
        }

    switch (type) {
    case FtpOperationType::fill_file_list: return update_file_list_async(full_path);
    case FtpOperationType::download_file: return download_file_async(full_path + file_name, location);
    }
    return;
}

VOID CALLBACK reinit_server(PVOID lp_parameter, BOOLEAN /*TimerOrWaitFired*/
) {
    DownloadDicsDlg* dlg_instance = ((DownloadDicsDlg *)lp_parameter);
    dlg_instance->indicate_that_saving_might_be_needed();
    dlg_instance->on_display_action();
    dlg_instance->remove_timer();
}

void DownloadDicsDlg::remove_timer() {
    DeleteTimerQueueTimer(nullptr, m_timer, nullptr);
    m_timer = nullptr;
}

void DownloadDicsDlg::set_options(bool show_only_known, bool install_system) {
    Button_SetCheck(m_h_show_only_known, show_only_known ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_h_install_system, install_system ? BST_CHECKED : BST_UNCHECKED);
}

void DownloadDicsDlg::update_options(SpellChecker* spellchecker) {
    spellchecker->set_show_only_know(Button_GetCheck(m_h_show_only_known) == BST_CHECKED);
    spellchecker->set_install_system(Button_GetCheck(m_h_install_system) ==
        BST_CHECKED);
}

void DownloadDicsDlg::refresh() { reinit_server(this, false); }

INT_PTR DownloadDicsDlg::run_dlg_proc(UINT message, WPARAM w_param,
                                      LPARAM l_param) {
    switch (message) {
    case WM_INITDIALOG:
        {
            m_h_file_list = ::GetDlgItem(_hSelf, IDC_FILE_LIST);
            m_h_address = ::GetDlgItem(_hSelf, IDC_ADDRESS);
            m_h_status = ::GetDlgItem(_hSelf, IDC_SERVER_STATUS);
            m_h_install_selected = ::GetDlgItem(_hSelf, IDOK);
            m_h_show_only_known = ::GetDlgItem(_hSelf, IDC_SHOWONLYKNOWN);
            m_h_refresh = ::GetDlgItem(_hSelf, IDC_REFRESH);
            m_h_install_system = ::GetDlgItem(_hSelf, IDC_INSTALL_SYSTEM);
            m_refresh_icon = (HICON)LoadImage(_hInst, MAKEINTRESOURCE(IDI_REFRESH),
                                              IMAGE_ICON, 16, 16, 0);
            SendMessage(m_h_refresh, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)m_refresh_icon);
            get_spell_checker()->reset_download_combobox();
            get_spell_checker()->fill_download_dics_dialog();
            m_default_brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        }
        return true;
    case WM_COMMAND:
        {
            switch (LOWORD(w_param)) {
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
                    if (m_timer)
                        ChangeTimerQueueTimer(nullptr, m_timer, 1000, 0);
                    else
                        CreateTimerQueueTimer(&m_timer, nullptr, reinit_server, this, 1000, 0, 0);
                }
                else if (HIWORD(w_param) == CBN_SELCHANGE) {
                    get_spell_checker()->update_from_download_dics_options_no_update();
                    reinit_server(this, false);
                    m_check_if_saving_is_needed = 0;
                }
                break;
            case IDC_REFRESH:
                {
                    refresh();
                }
                break;
            case IDC_INSTALL_SYSTEM:
                if (HIWORD(w_param) == BN_CLICKED) {
                    get_spell_checker()->update_from_download_dics_options_no_update();
                }
                break;
            case IDC_SHOWONLYKNOWN:
                if (HIWORD(w_param) == BN_CLICKED) {
                    get_spell_checker()->update_from_download_dics_options();
                }
                break;
            case IDC_SELECTPROXY:
                if (HIWORD(w_param) == BN_CLICKED) {
                    get_select_proxy()->do_dialog();
                    get_select_proxy()->display();
                }
            }
        }
        break;
    case WM_CTLCOLORSTATIC:
        if (m_h_status == (HWND)l_param) {
            HDC h_dc = (HDC)w_param;
            SetBkColor(h_dc, GetSysColor(COLOR_BTNFACE));
            // SetBkMode(hDC, TRANSPARENT);
            SetTextColor(h_dc, m_status_color);
            return (INT_PTR)m_default_brush;
        }
        break;
    case WM_CLOSE:
        DeleteObject(m_default_brush);
        break;
    };
    return false;
}
