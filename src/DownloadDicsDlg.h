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

#pragma once

#include "StaticDialog\StaticDialog.h"
#include <Wininet.h>
#include <optional>
#include "FTPFileStatus.h"
#include <variant>
#include "CommonFunctions.h"
#include "TaskWrapper.h"

struct LanguageName;

void ftpTrim(std::wstring& FtpAddress);

enum FtpOperationType {
    fillFileList = 0,
    downloadFile,
};

enum class FtpWebOperationErrorType {
    none,
    httpClientCannotBeInitialized,
    urlCannotBeOpened,
    queryingStatusCodeFailed,
    proxyAuthorizationRequired,
    httpError,
    htmlCannotBeParsed,
    FileIsNotWriteable,
    DownloadCancelled,
};

struct FtpWebOperationError {
    FtpWebOperationErrorType type;
    int statusCode;
};

enum class FtpOperationErrorType {
    none,
    loginFailed,
    downloadFailed,
    downloadCancelled,
};

class SpellChecker;

struct FtpOperationParams {
    std::wstring address;
    std::wstring path;
    bool useProxy;
    std::wstring proxyAddress;
    int proxyPort;
    bool anonymous;
    std::wstring proxyUsername;
    std::wstring proxyPassword;
};

class DownloadDicsDlg : public StaticDialog {
public:
    ~DownloadDicsDlg();
    DownloadDicsDlg();
    void DoDialog();
    // Maybe hunspell interface should be passed here
    void initDlg(HINSTANCE hInst, HWND Parent,
              SpellChecker* SpellCheckerInstanceArg);
    INT_PTR WINAPI run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
    void UpdateListBox();
    void onNewFileList(const std::vector<std::wstring>& list);
    void prepareFileListUpdate();
    FtpOperationParams spawnFtpOperationParams(const std::wstring& fullPath);
    void updateFileListAsyncWebProxy(const std::wstring& fullPath);
    void updateFileListAsync(const std::wstring& fullPath);
    void downloadFileAsync(const std::wstring& fullPath, const std::wstring& targetLocation);
    void prepareDictionariesDownload();
    void downloadFilesAsync(const std::wstring& fullPath);
    void downloadFileAsyncWebPRoxy(const std::wstring& fullPath, const std::wstring& targetLocation);
    void DoFtpOperation(FtpOperationType Type, const std::wstring& fullPath,
                        const std::wstring& FileName = L"", const std::wstring& Location = L"");
    void startNextDownload();
    void DownloadSelected();
    void FillFileList();
    void RemoveTimer();
    void OnDisplayAction();
    void IndicateThatSavingMightBeNeeded();
    void SetOptions(bool ShowOnlyKnown, bool InstallSystem);
    void UpdateOptions(SpellChecker* spellchecker);
    void SetCancelPressed(bool Value);
    void Refresh();
    LRESULT AskReplacementMessage(const wchar_t* DicName);
    bool prepareDownloading();
    void finalizeDownloading();
    void onFileDownloaded();
    std::optional<std::wstring> currentAddress() const;
    void updateStatus(const wchar_t* text, COLORREF statusColor);
    void uiUpdate();
    void processFileListError(FtpOperationErrorType error);
    void processFileListError(const FtpWebOperationError& error);

private:
    void DoFtpOperationThroughHttpProxy(FtpOperationType Type, std::wstring Address,
                                        const wchar_t* FileName, const wchar_t* Location);

private:
    std::vector<LanguageName> CurrentLangs;
    std::vector<LanguageName> CurrentLangsFiltered;
    HBRUSH DefaultBrush;
    COLORREF StatusColor;
    SpellChecker* SpellCheckerInstance;
    HWND LibCombo;
    HWND HFileList;
    HWND HAddress = nullptr
    ;
    HWND HStatus;
    HWND HInstallSelected;
    HWND HShowOnlyKnown;
    HWND HRefresh;
    HWND HInstallSystem;
    HICON RefreshIcon;
    HANDLE Timer;
    bool CancelPressed;
    int CheckIfSavingIsNeeded;
    std::optional<TaskWrapper> ftpOperationTask;

    // Download State:
    struct DownloadRequest {
        std::wstring targetPath;
        std::wstring fileName;
    };

    int Failure;
    int DownloadedCount;
    int supposedDownloadedCount;
    std::wstring Message;
    std::vector<DownloadRequest> m_toDownload;
    decltype (m_toDownload)::iterator m_cur;
};
