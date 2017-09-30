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

#include "Controls/CheckedList/CheckedList.h"
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

void DownloadDicsDlg::DoDialog() {
    if (!isCreated()) {
        create(IDD_DOWNLOADDICS);
    }
    else {
        goToCenter();
        display();
    }
    SetFocus(HFileList);
}

void DownloadDicsDlg::FillFileList() {
    GetDownloadDics()->DoFtpOperation(fillFileList, currentAddress()->c_str());
}

void DownloadDicsDlg::OnDisplayAction() {
    FillFileList();
}

DownloadDicsDlg::DownloadDicsDlg() {
    Timer = nullptr;
    RefreshIcon = nullptr;
    CheckIfSavingIsNeeded = 0;
    HFileList = nullptr;
}

void DownloadDicsDlg::initDlg(HINSTANCE hInst, HWND Parent,
                              SpellChecker* SpellCheckerInstanceArg) {
    ftpOperationTask = TaskWrapper{Parent};
    SpellCheckerInstance = SpellCheckerInstanceArg;
    return Window::init(hInst, Parent);
}

DownloadDicsDlg::~DownloadDicsDlg() {
    if (Timer)
        DeleteTimerQueueTimer(nullptr, Timer, nullptr);
    if (RefreshIcon)
        DestroyIcon(RefreshIcon);
}

void DownloadDicsDlg::IndicateThatSavingMightBeNeeded() {
    CheckIfSavingIsNeeded = 1;
}

LRESULT DownloadDicsDlg::AskReplacementMessage(const wchar_t* DicName) {
    std::wstring name;
    std::tie(name, std::ignore) = applyAlias(DicName);
    return MessageBox(_hParent, wstring_printf(
                          L"Looks like %s dictionary is already present. Do you want to replace it?",
                          name.c_str()
                      ).c_str(), L"Dictionary already exists", MB_YESNO);
}

static std::wstring getTempPath() {
    std::vector<wchar_t> tempPathBuf(MAX_PATH);
    GetTempPath(MAX_PATH, tempPathBuf.data());
    return tempPathBuf.data();
}

bool DownloadDicsDlg::prepareDownloading() {
    DownloadedCount = 0;
    supposedDownloadedCount = 0;
    Failure = 0;
    Message = L"Dictionaries copied:\n";
    m_toDownload.clear();

    // If path isn't exist we're gonna try to create it else it's finish
    if (!CheckForDirectoryExistence(getTempPath().c_str(), false, _hParent)) {
        MessageBox(
            _hParent, L"Path defined as temporary dir doesn't exist and couldn't "
            L"be created, probably one of subdirectories have limited "
            L"access, please make temporary path valid.",
            L"Temporary Path is Broken", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }
    CancelPressed = false;
    ProgressDlg* p = getProgress();
    p->getProgressData()->set(0, L"");
    getProgress()->update();
    p->SetTopMessage(L"");
    if (!CheckForDirectoryExistence(
        SpellCheckerInstance->GetInstallSystem()
            ? SpellCheckerInstance->GetHunspellAdditionalPath()
            : SpellCheckerInstance->GetHunspellPath(),
        0, _hParent)) // If path doesn't exist we're gonna try to create it
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
    for (int i = 0; i < ListBox_GetCount(HFileList); i++) {
        if (CheckedListBox_GetCheckState(HFileList, i) == BST_CHECKED) {
            DownloadRequest req;
            req.fileName = CurrentLangsFiltered[i].OrigName + L".zip"s;
            req.targetPath = getTempPath() + req.fileName;
            m_toDownload.push_back(req);
            ++supposedDownloadedCount;
        }
    }
    m_cur = m_toDownload.begin();
    return true;
}

void DownloadDicsDlg::finalizeDownloading() {
    getProgress()->display(false);
    if (Failure == 1) {
        MessageBox(
            _hParent, L"Access denied to dictionaries directory, either try to run "
            L"Notepad++ as administrator or select some different, "
            L"accessible dictionary path",
            L"Dictionaries Haven't Been Downloaded", MB_OK | MB_ICONEXCLAMATION);
    }
    else if (DownloadedCount) {
        MessageBox(_hParent, Message.c_str(), L"Dictionaries Downloaded",
                   MB_OK | MB_ICONINFORMATION);
    }
    else if (supposedDownloadedCount) // Otherwise - silent
    {
        MessageBox(
            _hParent, L"Sadly, no dictionaries were copied, please try again",
            L"Dictionaries Haven't Been Downloaded", MB_OK | MB_ICONEXCLAMATION);
    }
    for (int i = 0; i < ListBox_GetCount(HFileList); i++)
        CheckedListBox_SetCheckState(HFileList, i, BST_UNCHECKED);
    SpellCheckerInstance->HunspellReinitSettings(
        true); // Calling the update for Hunspell dictionary list
    SpellCheckerInstance->ReinitLanguageLists(true);
    SpellCheckerInstance->DoPluginMenuInclusion();
    SpellCheckerInstance->RecheckVisibleBothViews();
}

static const auto bufSizeForCopy = 10240;

void DownloadDicsDlg::onFileDownloaded() {
    bool IsAffFile = false;
    bool IsDicFile = false;
    wchar_t ProgMessage[DEFAULT_BUF_SIZE];
    std::map<std::string, int> FilesFound; // 0x01 - .aff found, 0x02 - .dic found
    auto LocalPathANSI = to_string (m_cur->targetPath.c_str());
    unzFile fp = unzOpen(LocalPathANSI.c_str ());
    unz_file_info fInfo;
    if (unzGoToFirstFile(fp) != UNZ_OK)
        goto clean_and_continue;
    do {
        std::string DicFileNameANSI;
        {
            unzGetCurrentFileInfo(fp, &fInfo, nullptr, 0, nullptr, 0, nullptr, 0);
            std::vector<char> buf(fInfo.size_filename + 1);
            unzGetCurrentFileInfo(fp, &fInfo, buf.data(), static_cast<uLong>(buf.size()), nullptr, 0, nullptr, 0);
            DicFileNameANSI = buf.data();
        }

        if (DicFileNameANSI.length() < 4)
            continue;
        const auto ext = std::string_view(DicFileNameANSI).substr(DicFileNameANSI.length() - 4);
        IsAffFile = ext == ".aff";
        IsDicFile = ext == ".dic";
        if (IsAffFile || IsDicFile) {
            auto filename = DicFileNameANSI.substr(0, DicFileNameANSI.length() - 4);
            FilesFound[filename] |= (IsAffFile ? 0x01 : 0x02);
            unzOpenCurrentFile(fp);
            auto dicFileLocalPath = getTempPath();
            auto dicFileName = to_wstring(filename.c_str());
            dicFileLocalPath += dicFileName;
            if (IsAffFile)
                dicFileLocalPath += L".aff";
            else
                dicFileLocalPath += L".dic";

            SetFileAttributes(dicFileLocalPath.c_str(), FILE_ATTRIBUTE_NORMAL);
            auto LocalDicFileHandle =
                _wopen(dicFileLocalPath.c_str(), _O_CREAT | _O_BINARY | _O_WRONLY);
            if (LocalDicFileHandle == -1)
                continue;

            swprintf(ProgMessage, L"Extracting %s...", dicFileName.c_str());
            getProgress()->SetTopMessage(ProgMessage);
            DWORD BytesTotal = 0;
            int BytesCopied;
            std::vector<char> FileCopyBuf(bufSizeForCopy);
            while ((BytesCopied = unzReadCurrentFile(fp, FileCopyBuf.data(),
                                                     static_cast<unsigned int>(FileCopyBuf.size()))) != 0) {
                _write(LocalDicFileHandle, FileCopyBuf.data(), BytesCopied);
                BytesTotal += BytesCopied;
                swprintf(ProgMessage, L"%d / %d bytes extracted (%d %%)",
                         BytesTotal, fInfo.uncompressed_size,
                         BytesTotal * 100 / fInfo.uncompressed_size);
                getProgress()->getProgressData()->set(BytesTotal * 100 / fInfo.uncompressed_size, ProgMessage);
            }
            unzCloseCurrentFile(fp);
            _close(LocalDicFileHandle);
        }
    }
    while (unzGoToNextFile(fp) == UNZ_OK);
    // Now we're gonna check what's exactly we extracted with using FilesFound
    // map
    for (auto& p : FilesFound) {
        auto& fileNameANSI = p.first; // TODO: change to structured binding when Resharper supports them
        auto mask = p.second;
        auto fileName = to_wstring(fileNameANSI.c_str());
        if (mask != 0x03) // Some of .aff/.dic is missing
        {
            auto DicFileLocalPath = getTempPath();
            switch (mask) {
            case 1:
                DicFileLocalPath += L".aff";
                break;
            case 2:
                DicFileLocalPath += L".dic";
                break;
            }
            SetFileAttributes(DicFileLocalPath.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFile(DicFileLocalPath.c_str());
        }
        else {
            auto DicFileLocalPath = getTempPath();
            (DicFileLocalPath += fileName) += L".aff";
            std::wstring HunspellDicPath =
                SpellCheckerInstance->GetInstallSystem()
                    ? SpellCheckerInstance->GetHunspellAdditionalPath()
                    : SpellCheckerInstance->GetHunspellPath();
            HunspellDicPath += L"\\";
            HunspellDicPath += fileName;
            HunspellDicPath += L".aff";
            bool Confirmation = true;
            bool ReplaceQuestionWasAsked = false;
            if (PathFileExists(HunspellDicPath.c_str())) {
                auto Answer = AskReplacementMessage(fileName.c_str());
                ReplaceQuestionWasAsked = true;
                if (Answer == IDNO) {
                    Confirmation = false;
                    SetFileAttributes(DicFileLocalPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(DicFileLocalPath.c_str());
                }
                else {
                    SetFileAttributes(HunspellDicPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(HunspellDicPath.c_str());
                }
            }

            if (Confirmation && !MoveFile(DicFileLocalPath.c_str(), HunspellDicPath.c_str())) {
                SetFileAttributes(DicFileLocalPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFile(DicFileLocalPath.c_str());
                Failure = 1;
            }
            DicFileLocalPath = DicFileLocalPath.substr(0, DicFileLocalPath.length() - 4) + L".dic";
            HunspellDicPath = HunspellDicPath.substr(0, HunspellDicPath.length() - 4) + L".dic";
            if (!Confirmation) {
                SetFileAttributes(DicFileLocalPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFile(DicFileLocalPath.c_str());
            }
            else if (PathFileExists(HunspellDicPath.c_str())) {
                int Res = 0;
                if (ReplaceQuestionWasAsked)
                    Res = !Confirmation;
                else {
                    Res = (AskReplacementMessage(fileName.c_str()) == IDNO);
                }
                if (Res) {
                    SetFileAttributes(DicFileLocalPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(DicFileLocalPath.c_str());
                    Confirmation = false;
                }
                else {
                    SetFileAttributes(HunspellDicPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFile(HunspellDicPath.c_str());
                }
            }

            if (Confirmation && !MoveFile(DicFileLocalPath.c_str(), HunspellDicPath.c_str())) {
                SetFileAttributes(DicFileLocalPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFile(DicFileLocalPath.c_str());
                Failure = 1;
            }
            std::wstring ConvertedDicName;
            std::tie(ConvertedDicName, std::ignore) = applyAlias(fileName);
            if (Failure)
                goto clean_and_continue;

            if (Confirmation) {
                Message += ConvertedDicName + L"\n";
                DownloadedCount++;
            }
            if (!Confirmation)
                supposedDownloadedCount--;
        }
    }
clean_and_continue:
    FilesFound.clear();
    unzClose(fp);
    SetFileAttributes(m_cur->targetPath.c_str(), FILE_ATTRIBUTE_NORMAL);
    DeleteFile(m_cur->targetPath.c_str()); // Removing downloaded .zip file
    if (Failure)
        return finalizeDownloading();

    ++m_cur;
    startNextDownload();
}

void DownloadDicsDlg::startNextDownload() {
    if (m_cur == m_toDownload.end() || CancelPressed)
        return finalizeDownloading();

    getProgress()->SetTopMessage(wstring_printf(L"Downloading %s...", m_cur->fileName.c_str()).c_str());
    if (PathFileExists(m_cur->targetPath.c_str())) {
        SetFileAttributes(m_cur->targetPath.c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFile(m_cur->targetPath.c_str());
    }
    DoFtpOperation(downloadFile, *currentAddress(), m_cur->fileName, m_cur->targetPath);
}

void DownloadDicsDlg::DownloadSelected() {
    if (!prepareDownloading())
        return;

    startNextDownload();
}

void ftpTrim(std::wstring& ftpAddress) {
    trim(ftpAddress);
    for (auto& c : ftpAddress) {
        if (c == L'\\')
            c = L'/';
    }
    std::wstring ftpPrefix = L"ftp://";
    if (ftpAddress.find(ftpPrefix) == 0)
        ftpAddress.erase(ftpAddress.begin(), ftpAddress.begin() + ftpPrefix.length());

    std::transform(ftpAddress.begin(), std::find(ftpAddress.begin(), ftpAddress.end(), L'/'), ftpAddress.begin(),
                   &towlower); // In dir names upper/lower case could matter
}

std::pair<std::wstring, std::wstring> ftpSplit(std::wstring fullPath) {
    ftpTrim(fullPath);
    auto it = std::find(fullPath.begin(), fullPath.end(), L'/');
    return {{fullPath.begin(), it}, {it != fullPath.end() ? next(it) : it, fullPath.end()}};
}

void DownloadDicsDlg::UpdateListBox() {
    if (!HFileList || CurrentLangs.empty())
        return;

    CurrentLangsFiltered.clear();
    for (auto& lang : CurrentLangs) {
        const LanguageName Lang(lang);
        if (SpellCheckerInstance->GetShowOnlyKnown() &&
            !Lang.AliasApplied)
            continue;
        CurrentLangsFiltered.push_back(Lang);
    }
    ListBox_ResetContent(HFileList);
    for (auto& lang : CurrentLangsFiltered) {
        ListBox_AddString(HFileList, SpellCheckerInstance->GetDecodeNames()
            ? lang.AliasName.c_str ()
            : lang.OrigName.c_str ());
    }
}

// Copy of CFile with ability to kick progress bar
class Observer : public nsFTP::CFTPClient::CNotification {
    std::shared_ptr<ProgressData> m_progressData;
    std::wstring m_targetPath;
    long m_bytesReceived = 0;
    long m_targetSize;
    concurrency::cancellation_token m_token;
    nsFTP::CFTPClient& m_client;

public:
    Observer(std::shared_ptr<ProgressData> progressData, const std::wstring& targetPath,
             long targetSize, nsFTP::CFTPClient& client,
             concurrency::cancellation_token token)
        : m_progressData(progressData), m_targetPath(targetPath), m_targetSize(targetSize), m_token(token),
          m_client(client) {
    }

    virtual void OnBytesReceived(const nsFTP::TByteVector& /*vBuffer*/,
                                 long lReceivedBytes) override {
        if (m_token.is_canceled()) {
            DeleteFile(m_targetPath.c_str());
            m_client.Logout();
            return;
        }
        m_bytesReceived += lReceivedBytes;
        m_progressData->set(m_bytesReceived * 100 / m_targetSize,
                            wstring_printf(L"%d / %d bytes downloaded (%d %%)", m_bytesReceived,
                                           m_targetSize, m_bytesReceived * 100 / m_targetSize));
    }
};

void DownloadDicsDlg::SetCancelPressed(bool Value) {
    CancelPressed = Value;
    ftpOperationTask->cancel();
    m_cur = m_toDownload.end();
    finalizeDownloading();
}

#define INITIAL_BUFFER_SIZE 50 * 1024
#define INITIAL_SMALL_BUFFER_SIZE 10 * 1024

std::optional<std::wstring> DownloadDicsDlg::currentAddress() const {
    if (!HAddress)
        return std::nullopt;
    auto sel = ComboBox_GetCurSel (HAddress);
    if (sel < 0) {
        auto textLen = ComboBox_GetTextLength (HAddress);
        std::vector<wchar_t> buf(textLen + 1);
        ComboBox_GetText (HAddress, buf.data (), textLen + 1);
        return buf.data();
    }
    auto textLen = ComboBox_GetLBTextLen (HAddress, sel);
    std::vector<wchar_t> buf(textLen + 1);
    ComboBox_GetLBText (HAddress, sel, buf.data ());
    return buf.data();
}


static bool ftpLogin(nsFTP::CFTPClient& client, const FtpOperationParams& params) {
    std::unique_ptr<nsFTP::CLogonInfo> logonInfo;
    if (!params.useProxy)
        logonInfo = std::make_unique<nsFTP::CLogonInfo>(params.address);
    else
        logonInfo = std::make_unique<nsFTP::CLogonInfo>(
            params.address, USHORT(21), L"anonymous", L"", L"",
            params.proxyAddress, L"", L"",
            static_cast<USHORT>(params.proxyPort),
            nsFTP::CFirewallType::UserWithNoLogon());
    return client.Login(*logonInfo);
}


std::variant<FtpOperationErrorType, std::vector<std::wstring>> doDownloadFileListFTP(FtpOperationParams params) {
    nsFTP::CFTPClient client(nsSocket::CreateDefaultBlockingSocketInstance(), 3);
    if (!ftpLogin(client, params))
        return FtpOperationErrorType::loginFailed;

    nsFTP::TFTPFileStatusShPtrVec list;
    if (!client.List(params.path, list, true))
        return FtpOperationErrorType::downloadFailed;

    std::vector<std::wstring> ret(list.size());
    std::transform(list.begin(), list.end(), ret.begin(),
                   [](const nsFTP::TFTPFileStatusShPtr& status) { return status->Name(); });
    return ret;
}

std::optional<FtpOperationErrorType> doDownloadFile(FtpOperationParams params, const std::wstring& targetPath,
                                                    std::shared_ptr<ProgressData> progressData, concurrency::
                                                    cancellation_token token) {
    nsFTP::CFTPClient client(nsSocket::CreateDefaultBlockingSocketInstance(), 3);
    if (!ftpLogin(client, params))
        return FtpOperationErrorType::loginFailed;

    long FileSize = 0;
    if (client.FileSize(params.path, FileSize) != nsFTP::FTP_OK)
        return FtpOperationErrorType::downloadFailed;

    auto progressUpdater = std::make_unique<Observer>(std::move(progressData), targetPath, FileSize, client, token);
    client.AttachObserver(progressUpdater.get());

    if (!client.DownloadFile(params.path, targetPath,
                             nsFTP::CRepresentation(nsFTP::CType::Image()),
                             true)) {
        if (PathFileExists(targetPath.c_str())) {
            SetFileAttributes(targetPath.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFile(targetPath.c_str());
            return FtpOperationErrorType::downloadCancelled;
        }
    }

    client.DetachObserver(progressUpdater.get());
    return std::nullopt;
}

static std::variant<HINTERNET, FtpWebOperationError> openUrl(HINTERNET WinInetHandle, const std::wstring& url,
                                                             const FtpOperationParams& params) {
    const auto urlHandle = InternetOpenUrl(WinInetHandle, url.c_str(), nullptr, 0,
                                           INTERNET_FLAG_PASSIVE | INTERNET_FLAG_RELOAD |
                                           INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    if (!urlHandle)
        return FtpWebOperationError{FtpWebOperationErrorType::urlCannotBeOpened, -1};

    if (!params.anonymous) {
        InternetSetOption(
            urlHandle, INTERNET_OPTION_PROXY_USERNAME,
            const_cast<wchar_t *>(params.proxyUsername.c_str()),
            static_cast<DWORD>(params.proxyUsername.length() + 1));
        InternetSetOption(
            urlHandle, INTERNET_OPTION_PROXY_PASSWORD,
            const_cast<wchar_t *>(params.proxyPassword.c_str()),
            static_cast<DWORD>(params.proxyPassword.length() + 1));
        InternetSetOption(urlHandle, INTERNET_OPTION_PROXY_SETTINGS_CHANGED, nullptr, 0);
    }
    HttpSendRequest(urlHandle, nullptr, 0, nullptr, 0);

    DWORD code = 0;
    DWORD Dummy = 0;
    DWORD Size = sizeof(DWORD);

    if (!HttpQueryInfo(urlHandle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       &code, &Size, &Dummy))
        return FtpWebOperationError{FtpWebOperationErrorType::queryingStatusCodeFailed, -1};

    if (code != HTTP_STATUS_OK) {
        if (code == HTTP_STATUS_PROXY_AUTH_REQ)
            return FtpWebOperationError{FtpWebOperationErrorType::proxyAuthorizationRequired, static_cast<int>(code)};
        else {
            return FtpWebOperationError{FtpWebOperationErrorType::httpError, static_cast<int>(code)};
        }
    }
    return urlHandle;
}

static std::variant<HINTERNET, FtpWebOperationError> ftpWebProxyLogin(const FtpOperationParams& params) {
    std::wstring proxyFinalString = params.proxyAddress + L":" + std::to_wstring(params.proxyPort);
    const auto WinInetHandle = InternetOpen(L"DSpellCheck", INTERNET_OPEN_TYPE_PROXY, proxyFinalString.c_str(), L"", 0);
    if (!WinInetHandle)
        return FtpWebOperationError{FtpWebOperationErrorType::httpClientCannotBeInitialized, -1};
    DWORD TimeOut = 15000;
    for (auto option : {INTERNET_OPTION_CONNECT_TIMEOUT, INTERNET_OPTION_SEND_TIMEOUT, INTERNET_OPTION_RECEIVE_TIMEOUT})
        InternetSetOption(WinInetHandle, option, &TimeOut, sizeof(DWORD));
    return WinInetHandle;
}

std::variant<FtpWebOperationError, std::vector<std::wstring>> doDownloadFileListFTPWebProxy(FtpOperationParams params) {
    auto result = ftpWebProxyLogin(params);
    if (auto error = std::get_if<FtpWebOperationError>(&result))
        return *error;

    auto handle = std::get<HINTERNET>(result);
    result = openUrl(handle, L"ftp://" + params.address + L"/" + params.path, params);
    if (auto error = std::get_if<FtpWebOperationError>(&result))
        return *error;

    auto urlHandle = std::get<HINTERNET>(result);

    std::vector<char> FileBuffer(INITIAL_BUFFER_SIZE);
    std::size_t curIndex = 0;
    DWORD BytesRead = 0;
    DWORD BytesReadTotal = 0;
    DWORD BytesToRead = 0;
    unsigned int CurBufSize = INITIAL_BUFFER_SIZE;
    while (1) {
        InternetQueryDataAvailable(urlHandle, &BytesToRead, 0, 0);
        if (BytesToRead == 0)
            break;
        if (BytesReadTotal + BytesToRead > CurBufSize) {
            FileBuffer.resize(FileBuffer.size() * 2);
        }

        InternetReadFile(urlHandle, FileBuffer.data() + curIndex, BytesToRead, &BytesRead);
        if (BytesRead == 0)
            break;
        BytesReadTotal += BytesRead;
        curIndex += BytesRead;
    }
    curIndex = 0;
    int count = 0;
    // Bad Parsing. Really, really bad. I'm sorry :(
    auto CurPos = FileBuffer.data();
    std::vector<std::wstring> out;
    while ((size_t)(CurPos - FileBuffer.data()) < BytesReadTotal) {
        char* TempCurPos = CurPos;
        CurPos = strstr(CurPos, "</A>");
        if (CurPos == nullptr)
            CurPos = strstr(TempCurPos, "</a>");

        if (CurPos == nullptr)
            break;
        TempCurPos = CurPos;
        while (*TempCurPos != '>' && TempCurPos > FileBuffer.data())
            TempCurPos--;

        if (TempCurPos == nullptr)
            return FtpWebOperationError{FtpWebOperationErrorType::htmlCannotBeParsed, -1};
        TempCurPos++;
        CurPos--;
        if (CurPos <= TempCurPos) {
            CurPos += 2;
            continue;
        }
        std::vector<char> TempBuf(CurPos - TempCurPos + 1 + 1);
        strncpy(TempBuf.data(), TempCurPos, CurPos - TempCurPos + 1);
        TempBuf[CurPos - TempCurPos + 1] = '\0';
        CurPos += 2;
        if (!PathMatchSpecA(TempBuf.data(), "*.zip"))
            continue;

        count++;
        out.push_back(to_wstring (TempBuf.data()));
    }
    return out;
}

std::optional<FtpWebOperationError> doDownloadFileWebProxy(FtpOperationParams params, const std::wstring& targetPath,
                                                           std::shared_ptr<ProgressData> progressData, concurrency::
                                                           cancellation_token token) {
    auto result = ftpWebProxyLogin(params);
    if (auto error = std::get_if<FtpWebOperationError>(&result))
        return *error;

    auto handle = std::get<HINTERNET>(result);
    result = openUrl(handle, L"ftp://" + params.address + L"/" + params.path, params);
    if (auto error = std::get_if<FtpWebOperationError>(&result))
        return *error;

    auto urlHandle = std::get<HINTERNET>(result);

    std::vector<char> FileBuffer;
    DWORD BytesToRead = 0;
    DWORD BytesRead;
    DWORD BytesReadTotal = 0;
    int FileHandle = _wopen(targetPath.c_str(), _O_CREAT | _O_BINARY | _O_WRONLY);
    if (!FileHandle)
        return FtpWebOperationError{FtpWebOperationErrorType::FileIsNotWriteable, -1};

    getProgress()->SetMarquee(true);
    while (1) {
        if (token.is_canceled()) {
            _close(FileHandle);
            if (PathFileExists(targetPath.c_str())) {
                SetFileAttributes(targetPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFile(targetPath.c_str());
            }
            return FtpWebOperationError{FtpWebOperationErrorType::DownloadCancelled, -1};
        }
        InternetQueryDataAvailable(urlHandle, &BytesToRead, 0, 0);
        if (BytesToRead == 0)
            break;
        if (BytesToRead > FileBuffer.size()) {
            FileBuffer.resize(BytesToRead);
        }

        InternetReadFile(urlHandle, FileBuffer.data(), BytesToRead, &BytesRead);
        if (BytesRead == 0)
            break;

        write(FileHandle, FileBuffer.data(), BytesRead);
        BytesReadTotal += BytesRead;

        progressData->set(0, wstring_printf(L"%d / ???   bytes downloaded", BytesReadTotal), true);
    }
    _close(FileHandle);
    return std::nullopt;
}


void DownloadDicsDlg::updateStatus(const wchar_t* text, COLORREF statusColor) {
    StatusColor = statusColor;
    Static_SetText(HStatus, text);
}

void DownloadDicsDlg::uiUpdate() {
    getProgress()->update();
}

void DownloadDicsDlg::onNewFileList(const std::vector<std::wstring>& list) {
    int count = 0;

    for (unsigned int i = 0; i < list.size(); i++) {
        if (!PathMatchSpec(list[i].c_str(), L"*.zip"))
            continue;

        count++;
        auto name = list[i];
        name.erase(name.end() - 4, name.end());
        LanguageName Lang(name.c_str());
        CurrentLangs.push_back(Lang);
    }

    if (count == 0) {
        return updateStatus(L"Status: Directory doesn't contain any zipped files", COLOR_WARN);
    }

    std::sort(CurrentLangs.begin(), CurrentLangs.end(), [decode = SpellCheckerInstance->GetDecodeNames()]
          (const auto& lhs, const auto& rhs)
          {
              return decode ? lessAliases(lhs, rhs) : lessOriginal(lhs, rhs);
          });
        UpdateListBox(); // Used only here and on filter change
    // If it is success when we perhaps should add this address to our list.
    if (CheckIfSavingIsNeeded) {
        SpellCheckerInstance->addUserServer(*currentAddress());
    }
    updateStatus(L"Status: List of available files was successfully loaded", COLOR_OK);
    EnableWindow(HInstallSelected, true);
}

void DownloadDicsDlg::processFileListError(FtpOperationErrorType error) {
    switch (error) {
    case FtpOperationErrorType::none: break;
    case FtpOperationErrorType::loginFailed: return updateStatus(L"Status: Connection cannot be established",
                                                                 COLOR_FAIL);
    case FtpOperationErrorType::downloadFailed: return updateStatus(L"Status: Can't list directory files", COLOR_FAIL);
    case FtpOperationErrorType::downloadCancelled: return updateStatus(L"Status: Download Cancelled", COLOR_WARN);
    }
}

void DownloadDicsDlg::processFileListError(const FtpWebOperationError& error) {
    switch (error.type) {
    case FtpWebOperationErrorType::none: break;
    case FtpWebOperationErrorType::httpClientCannotBeInitialized: return updateStatus(
            L"Status: HTTP client cannot be initialize", COLOR_FAIL);
    case FtpWebOperationErrorType::urlCannotBeOpened: return updateStatus(L"Status: URL cannot be opneed", COLOR_FAIL);
    case FtpWebOperationErrorType::queryingStatusCodeFailed: return updateStatus(
            L"Status: Querying status code failed", COLOR_FAIL);
    case FtpWebOperationErrorType::proxyAuthorizationRequired: return updateStatus(
            L"Status: Proxy authorization required", COLOR_FAIL);
    case FtpWebOperationErrorType::httpError: return updateStatus(
            wstring_printf(L"Status: HTTP error %d", error.statusCode).c_str(), COLOR_FAIL);
    case FtpWebOperationErrorType::htmlCannotBeParsed:
        return updateStatus(L"Status: HTML cannot be parsed", COLOR_FAIL);
    case FtpWebOperationErrorType::FileIsNotWriteable:
        return updateStatus(L"Status: Target file cannot be written", COLOR_FAIL);
    case FtpWebOperationErrorType::DownloadCancelled:
        return updateStatus(L"Status: Target file cannot be written", COLOR_WARN);
    }
}

void DownloadDicsDlg::prepareFileListUpdate() {
    EnableWindow(HInstallSelected, false);
    StatusColor = COLOR_NEUTRAL;
    Static_SetText(HStatus, L"Status: Loading...");
    ListBox_ResetContent(HFileList);
    CurrentLangs.clear ();;
}

FtpOperationParams DownloadDicsDlg::spawnFtpOperationParams(const std::wstring& fullPath) {
    FtpOperationParams params;
    std::tie(params.address, params.path) = ftpSplit(fullPath);
    params.useProxy = SpellCheckerInstance->GetUseProxy();
    params.proxyPort = SpellCheckerInstance->GetProxyPort();
    params.proxyAddress = SpellCheckerInstance->GetProxyHostName();
    params.anonymous = SpellCheckerInstance->GetProxyAnonymous();
    params.proxyUsername = SpellCheckerInstance->GetProxyUserName();
    params.proxyPassword = SpellCheckerInstance->GetProxyPassword();
    return params;
}

void DownloadDicsDlg::updateFileListAsyncWebProxy(const std::wstring& fullPath) {
    // temporary workaround for xsmf_control.h bug
    static_assert(std::is_copy_constructible_v<std::variant<FtpOperationErrorType, std::vector<std::wstring>>>);
    prepareFileListUpdate();
    ftpOperationTask->doDeferred([params = spawnFtpOperationParams(fullPath)](auto)
                             {
                                 return doDownloadFileListFTPWebProxy(params);
                             }, [this](std::variant<FtpWebOperationError, std::vector<std::wstring>> res)
                             {
                                 if (auto error = std::get_if<FtpWebOperationError>(&res)) {
                                     return this->processFileListError(*error);
                                 }
                                 onNewFileList(std::get<std::vector<std::wstring>>(res));
                             });
}

void DownloadDicsDlg::updateFileListAsync(const std::wstring& fullPath) {
    // temporary workaround for xsmf_control.h bug
    static_assert(std::is_copy_constructible_v<std::variant<FtpOperationErrorType, std::vector<std::wstring>>>);
    prepareFileListUpdate();
    ftpOperationTask->doDeferred([params = spawnFtpOperationParams(fullPath)](auto)
                             {
                                 return doDownloadFileListFTP(params);
                             }, [this](std::variant<FtpOperationErrorType, std::vector<std::wstring>> res)
                             {
                                 if (auto error = std::get_if<FtpOperationErrorType>(&res)) {
                                     return this->processFileListError(*error);
                                 }
                                 onNewFileList(std::get<std::vector<std::wstring>>(res));
                             });
}

void DownloadDicsDlg::downloadFileAsync(const std::wstring& fullPath, const std::wstring& targetLocation) {
    ftpOperationTask->doDeferred([params = spawnFtpOperationParams(fullPath), targetLocation,
                                     progressData = getProgress()->getProgressData()](auto token)
                             {
                                 return doDownloadFile(params, targetLocation, progressData, token);
                             }, [this](std::optional<FtpOperationErrorType>)
                             {
                                 onFileDownloaded();
                             });
}


void DownloadDicsDlg::downloadFileAsyncWebPRoxy(const std::wstring& fullPath, const std::wstring& targetLocation) {
    ftpOperationTask->doDeferred([params = spawnFtpOperationParams(fullPath), targetLocation,
                                     progressData = getProgress()->getProgressData()](auto token)
                             {
                                 return doDownloadFileWebProxy(params, targetLocation, progressData, token);
                             }, [this](std::optional<FtpWebOperationError>)
                             {
                                 onFileDownloaded();
                             });
}

void DownloadDicsDlg::DoFtpOperation(FtpOperationType Type, const std::wstring& fullPath,
                                     const std::wstring& fileName, const std::wstring& Location) {
    if (SpellCheckerInstance->GetUseProxy() &&
        SpellCheckerInstance->GetProxyType() == 0)
        switch (Type) {
        case fillFileList: return updateFileListAsyncWebProxy(fullPath);
        case downloadFile: return downloadFileAsyncWebPRoxy(fullPath + fileName, Location);
        }

    switch (Type) {
    case fillFileList: return updateFileListAsync(fullPath);
    case downloadFile: return downloadFileAsync(fullPath + fileName, Location);
    }
    return;
}

VOID CALLBACK ReinitServer(PVOID lpParameter, BOOLEAN /*TimerOrWaitFired*/
) {
    DownloadDicsDlg* DlgInstance = ((DownloadDicsDlg *)lpParameter);
    DlgInstance->IndicateThatSavingMightBeNeeded();
    DlgInstance->OnDisplayAction();
    DlgInstance->RemoveTimer();
}

void DownloadDicsDlg::RemoveTimer() {
    DeleteTimerQueueTimer(nullptr, Timer, nullptr);
    Timer = nullptr;
}

void DownloadDicsDlg::SetOptions(bool ShowOnlyKnown, bool InstallSystem) {
    Button_SetCheck(HShowOnlyKnown, ShowOnlyKnown ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(HInstallSystem, InstallSystem ? BST_CHECKED : BST_UNCHECKED);
}

void DownloadDicsDlg::UpdateOptions(SpellChecker* spellchecker) {
    spellchecker->SetShowOnlyKnow(Button_GetCheck(HShowOnlyKnown) == BST_CHECKED);
    spellchecker->SetInstallSystem(Button_GetCheck(HInstallSystem) ==
        BST_CHECKED);
}

void DownloadDicsDlg::Refresh() { ReinitServer(this, false); }

INT_PTR DownloadDicsDlg::run_dlgProc(UINT message, WPARAM wParam,
                                     LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        {
            HFileList = ::GetDlgItem(_hSelf, IDC_FILE_LIST);
            HAddress = ::GetDlgItem(_hSelf, IDC_ADDRESS);
            HStatus = ::GetDlgItem(_hSelf, IDC_SERVER_STATUS);
            HInstallSelected = ::GetDlgItem(_hSelf, IDOK);
            HShowOnlyKnown = ::GetDlgItem(_hSelf, IDC_SHOWONLYKNOWN);
            HRefresh = ::GetDlgItem(_hSelf, IDC_REFRESH);
            HInstallSystem = ::GetDlgItem(_hSelf, IDC_INSTALL_SYSTEM);
            RefreshIcon = (HICON)LoadImage(_hInst, MAKEINTRESOURCE(IDI_REFRESH),
                                           IMAGE_ICON, 16, 16, 0);
            SendMessage(HRefresh, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)RefreshIcon);
            getSpellChecker()->ResetDownloadCombobox();
            getSpellChecker()->fillDownloadDicsDialog();
            DefaultBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        }
        return true;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
            case IDOK:
                if (HIWORD(wParam) == BN_CLICKED) {
                    DownloadSelected();
                    getProgress()->DoDialog();
                    display(false);
                }
                break;
            case IDCANCEL:
                if (HIWORD(wParam) == BN_CLICKED) {
                    for (int i = 0; i < ListBox_GetCount(HFileList); i++)
                        CheckedListBox_SetCheckState(HFileList, i, BST_UNCHECKED);
                    display(false);
                }

                break;
            case IDC_ADDRESS:
                if (HIWORD(wParam) == CBN_EDITCHANGE) {
                    if (Timer)
                        ChangeTimerQueueTimer(nullptr, Timer, 1000, 0);
                    else
                        CreateTimerQueueTimer(&Timer, nullptr, ReinitServer, this, 1000, 0, 0);
                }
                else if (HIWORD(wParam) == CBN_SELCHANGE) {
                    getSpellChecker()->updateFromDownloadDicsOptionsNoUpdate();
                    ReinitServer(this, false);
                    CheckIfSavingIsNeeded = 0;
                }
                break;
            case IDC_REFRESH:
                {
                    Refresh();
                }
                break;
            case IDC_INSTALL_SYSTEM:
                if (HIWORD(wParam) == BN_CLICKED) {
                    getSpellChecker()->updateFromDownloadDicsOptionsNoUpdate();
                }
                break;
            case IDC_SHOWONLYKNOWN:
                if (HIWORD(wParam) == BN_CLICKED) {
                    getSpellChecker()->updateFromDownloadDicsOptions();
                }
                break;
            case IDC_SELECTPROXY:
                if (HIWORD(wParam) == BN_CLICKED) {
                    GetSelectProxy()->DoDialog();
                    GetSelectProxy()->display();
                }
            }
        }
        break;
    case WM_CTLCOLORSTATIC:
        if (HStatus == (HWND)lParam) {
            HDC hDC = (HDC)wParam;
            SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
            // SetBkMode(hDC, TRANSPARENT);
            SetTextColor(hDC, StatusColor);
            return (INT_PTR)DefaultBrush;
        }
        break;
    case WM_CLOSE:
        DeleteObject(DefaultBrush);
        break;
    };
    return false;
}
