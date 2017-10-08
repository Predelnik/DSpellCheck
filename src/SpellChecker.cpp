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

#include "AbstractSpellerInterface.h"
#include "AspellInterface.h"
#include "HunspellInterface.h"

#include "aspell.h"
#include "DownloadDicsDlg.h"
#include "iconv.h"
#include "CommonFunctions.h"
#include "LanguageName.h"
#include "LangList.h"
#include "MainDef.h"
#include "PluginInterface.h"
#include "Plugin.h"
#include "RemoveDictionariesDialog.h"
#include "SettingsDlg.h"
#include "SpellChecker.h"
#include "SciLexer.h"
#include "Scintilla.h"
#include "SelectProxyDialog.h"
#include "SuggestionsButton.h"
#include "SciUtils.h"
#include "utils/string_utils.h"
#include "utils/utf8.h"
#include "resource.h"

#ifdef UNICODE
#define DEFAULT_DELIMITERS                                                     \
  L",.!?\":;{}()[]\\/"                                                         \
  L"=+-^$*<>|#$@%&~"                                                           \
  L"\u2026\u2116\u2014\u00AB\u00BB\u2013\u2022\u00A9\u203A\u201C\u201D\u00B7"  \
  L"\u00A0\u0060\u2192\u00d7"
#else // All non-ASCII symbols removed
#define DEFAULT_DELIMITERS L",.!?\":;{}()[]\\/=+-^$*<>|#$@%&~"
#endif

HWND GetScintillaWindow(const NppData* NppDataArg) {
    int which = -1;
    SendMessage(NppDataArg->_nppHandle, NPPM_GETCURRENTSCINTILLA, 0,
                (LPARAM)&which);
    if (which == -1)
        return nullptr;
    if (which == 1)
        return NppDataArg->_scintillaSecondHandle;
    return (which == 0)
               ? NppDataArg->_scintillaMainHandle
               : NppDataArg->_scintillaSecondHandle;
}

bool SendMsgToBothEditors(const NppData* NppDataArg, UINT Msg, WPARAM wParam,
                          LPARAM lParam) {
    SendMessage(NppDataArg->_scintillaMainHandle, Msg, wParam, lParam);
    SendMessage(NppDataArg->_scintillaSecondHandle, Msg, wParam, lParam);
    return true;
}

LRESULT SendMsgToActiveEditor(HWND ScintillaWindow, UINT Msg,
                              WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/) {
    return SendMessage(ScintillaWindow, Msg, wParam, lParam);
}

LRESULT SendMsgToNpp(const NppData* NppDataArg, UINT Msg,
                     WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/) {
    return SendMessage(NppDataArg->_nppHandle, Msg, wParam, lParam);
}

// Remember: it's better to use PostMsg wherever possible, to avoid gui update
// on each message send etc etc
// Also it's better to avoid get current scintilla window too much times, since
// it's obviously uses 1 SendMsg call
LRESULT PostMsgToActiveEditor(HWND ScintillaWindow, UINT Msg,
                              WPARAM wParam /*= 0*/, LPARAM lParam /*= 0*/) {
    return PostMessage(ScintillaWindow, Msg, wParam, lParam);
}

void SpellChecker::addUserServer(std::wstring server) {
    ftpTrim(server);
    for (int i = 0; i < static_cast<int>(countof(DefaultServers)); i++) {
        std::wstring defServer = DefaultServers[i];
        ftpTrim(defServer);
        if (server == defServer)
            goto add_user_server_cleanup; // Nothing is done in this case
    }
    for (int i = 0; i < static_cast<int>(countof(ServerNames)); i++) {
        std::wstring addedServer = ServerNames[i];
        ftpTrim(addedServer);
        if (server == addedServer)
            goto add_user_server_cleanup; // Nothing is done in this case
    }
    // Then we're adding finally
    ServerNames[countof(ServerNames) - 1].clear();
    for (int i = countof(ServerNames) - 1; i > 0; i--) {
        ServerNames[i] = ServerNames[i - 1];
    }
    ServerNames[0] = nullptr;
    ServerNames[0] = server;
add_user_server_cleanup:
    ResetDownloadCombobox();
    SaveSettings();
}

SpellChecker::SpellChecker(const wchar_t* IniFilePathArg,
                           SettingsDlg* SettingsDlgInstanceArg,
                           NppData* NppDataInstanceArg,
                           SuggestionsButton* SuggestionsInstanceArg,
                           LangList* LangListInstanceArg) {
    CurrentPosition = 0;
    IniFilePath = IniFilePathArg;
    SettingsDlgInstance = SettingsDlgInstanceArg;
    SuggestionsInstance = SuggestionsInstanceArg;
    NppDataInstance = NppDataInstanceArg;
    LangListInstance = LangListInstanceArg;
    AutoCheckText = 0;
    MultiLangMode = 0;
    CheckThose = 0;
    SBTrans = 0;
    SBSize = 0;
    CurWordList = nullptr;
    SuggestionsMode = 1;
    WUCLength = 0;
    WUCPosition = 0;
    WUCisRight = true;
    CurrentScintilla = GetScintillaWindow(NppDataInstance);
    AspellSpeller = std::make_unique<AspellInterface>(NppDataInstance->_nppHandle);
    HunspellSpeller = std::make_unique<HunspellInterface>(NppDataInstance->_nppHandle);
    CurrentSpeller = AspellSpeller.get();
    PrepareStringForConversion();
    memset(ServerNames, 0, sizeof(ServerNames));
    memset(DefaultServers, 0, sizeof(DefaultServers));
    AddressIsSet = 0;
    DefaultServers[0] = L"ftp://ftp.snt.utwente.nl/pub/software/openoffice/contrib/dictionaries/";
    DefaultServers[1] = L"ftp://sunsite.informatik.rwth-aachen.de/pub/mirror/OpenOffice/contrib/dictionaries/";
    DefaultServers[2] = L"ftp://gd.tuwien.ac.at/office/openoffice/contrib/dictionaries/";
    DecodeNames = false;
    ResetHotSpotCache();
    ProxyAnonymous = true;
    ProxyType = 0;
    ProxyPort = 0;
    SettingsLoaded = false;
    UseProxy = false;
    bool res =
        (SendMsgToNpp(NppDataInstance, NPPM_ALLOCATESUPPORTED, 0, 0) != 0);

    if (res) {
        SetUseAllocatedIds(true);
        int Id;
        SendMsgToNpp(NppDataInstance, NPPM_ALLOCATECMDID, 350, (LPARAM)&Id);
        SetContextMenuIdStart(Id);
        SetLangsMenuIdStart(Id + 103);
    }
}

static const char Yo[] = "\xd0\x81";
static const char Ye[] = "\xd0\x95";
static const char yo[] = "\xd1\x91";
static const char ye[] = "\xd0\xb5";
static const char PunctuationApostrophe[] = "\xe2\x80\x99";

void SpellChecker::PrepareStringForConversion() {
    iconv_t Conv = iconv_open("CHAR", "UTF-8");
    const char* InString[] = {Yo, yo, Ye, ye, PunctuationApostrophe};
    std::string* OutString[] = {&YoANSI, &yoANSI, &YeANSI, &yeANSI, &PunctuationApostropheANSI};

    int i = 0;
    for (auto str : InString) {
        *OutString[i] = toUtf8String(str);
        ++i;
    }
    iconv_close(Conv);
}

SpellChecker::~SpellChecker() {
}

void InsertSuggMenuItem(HMENU Menu, const wchar_t* Text, BYTE Id, int InsertPos,
                        bool Separator) {
    MENUITEMINFO mi;
    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(MENUITEMINFO);
    if (Separator) {
        mi.fType = MFT_SEPARATOR;
    }
    else {
        mi.fType = MFT_STRING;
        mi.fMask = MIIM_ID | MIIM_TYPE;
        if (!GetUseAllocatedIds())
            mi.wID = MAKEWORD(Id, DSPELLCHECK_MENU_ID);
        else
            mi.wID = GetContextMenuIdStart() + Id;

        mi.dwTypeData = const_cast<wchar_t *>(Text);
        mi.cch = static_cast<int>(wcslen(Text)) + 1;
    }
    if (InsertPos == -1)
        InsertMenuItem(Menu, GetMenuItemCount(Menu), true, &mi);
    else
        InsertMenuItem(Menu, InsertPos, true, &mi);
}

void SpellChecker::precalculateMenu() {
    std::vector<SuggestionsMenuItem> suggestionMenuItems;
    if (CheckTextNeeded() && SuggestionsMode == SUGGESTIONS_CONTEXT_MENU) {
        long Pos, Length;
        WUCisRight = GetWordUnderCursorIsRight(Pos, Length, true);
        if (!WUCisRight) {
            WUCPosition = Pos;
            WUCLength = Length;
            suggestionMenuItems = FillSuggestionsMenu(nullptr);
        }
    }
    showCalculatedMenu(std::move(suggestionMenuItems));
}

void SpellChecker::SetSuggType(int SuggType) {
    SuggestionsMode = SuggType;
    HideSuggestionBox();
}

void SpellChecker::SetShowOnlyKnow(bool Value) { ShowOnlyKnown = Value; }

void SpellChecker::SetInstallSystem(bool Value) { InstallSystem = Value; }

bool SpellChecker::GetShowOnlyKnown() { return ShowOnlyKnown; }

bool SpellChecker::GetInstallSystem() { return InstallSystem; }

bool SpellChecker::GetDecodeNames() { return DecodeNames; }

const wchar_t* SpellChecker::GetLangByIndex(int i) {
    return CurrentLangs[i].OrigName.c_str();
}

void SpellChecker::ReinitLanguageLists(bool UpdateDialogs) {
    int SpellerId = LibMode;
    bool CurrentLangExists = false;
    const wchar_t* CurrentLang;

    auto SpellerToUse =
    (SpellerId == 1
         ? static_cast<AbstractSpellerInterface *>(HunspellSpeller.get())
         : AspellSpeller.get());

    if (SpellerId == 0) {
        GetDownloadDics()->display(false);
        GetRemoveDics()->display(false);
    }

    if (SpellerId == 1)
        CurrentLang = HunspellLanguage.c_str();
    else
        CurrentLang = AspellLanguage.c_str();

    if (SpellerToUse->IsWorking()) {
        if (UpdateDialogs)
            SettingsDlgInstance->GetSimpleDlg()->DisableLanguageCombo(false);
        auto LangsFromSpeller = SpellerToUse->GetLanguageList();
        CurrentLangs.clear();

        if (LangsFromSpeller.empty()) {
            if (UpdateDialogs)
                SettingsDlgInstance->GetSimpleDlg()->DisableLanguageCombo(true);
            return;
        }
        for (auto& lang : LangsFromSpeller) {
            LanguageName Lang(
                lang.c_str(),
                (SpellerId == 1 && DecodeNames)); // Using them only for Hunspell
            CurrentLangs.push_back(
                Lang);
            if (Lang.OrigName == CurrentLang)
                CurrentLangExists = true;
        }
        if (wcscmp(CurrentLang, L"<MULTIPLE>") == 0)
            CurrentLangExists = true;

        std::sort(CurrentLangs.begin(), CurrentLangs.end(),
                  DecodeNames ? lessAliases : lessOriginal);
        if (!CurrentLangExists && !CurrentLangs.empty()) {
            if (SpellerId == 1)
                SetHunspellLanguage(CurrentLangs.front().OrigName.c_str());
            else
                SetAspellLanguage(CurrentLangs.front().OrigName.c_str());
            RecheckVisibleBothViews();
        }
        if (UpdateDialogs)
            SettingsDlgInstance->GetSimpleDlg()->AddAvailableLanguages(
                CurrentLangs, SpellerId == 1 ? HunspellLanguage.c_str() : AspellLanguage.c_str(),
                SpellerId == 1 ? HunspellMultiLanguages : AspellMultiLanguages,
                SpellerId == 1 ? HunspellSpeller.get() : 0);
    }
    else {
        if (UpdateDialogs)
            SettingsDlgInstance->GetSimpleDlg()->DisableLanguageCombo(true);
    }
}

int SpellChecker::GetLibMode() { return LibMode; }

void SpellChecker::FillDialogs(bool NoDisplayCall) {
    ReinitLanguageLists(true);
    SettingsDlgInstance->GetSimpleDlg()->SetLibMode(LibMode);
    SettingsDlgInstance->GetSimpleDlg()->FillLibInfo(
        AspellSpeller->IsWorking()
            ? 2 - (CurrentLangs.empty() ? 0 : 1)
            : 0,
        AspellPath.c_str(), HunspellPath.c_str(), AdditionalHunspellPath.c_str());
    SettingsDlgInstance->GetSimpleDlg()->FillSugestionsNum(SuggestionsNum);
    SettingsDlgInstance->GetSimpleDlg()->SetFileTypes(CheckThose, FileTypes.c_str());
    SettingsDlgInstance->GetSimpleDlg()->SetCheckComments(checkOnlyCommentsAndString);
    SettingsDlgInstance->GetSimpleDlg()->SetDecodeNames(DecodeNames);
    SettingsDlgInstance->GetSimpleDlg()->SetSuggType(SuggestionsMode);
    SettingsDlgInstance->GetSimpleDlg()->SetOneUserDic(OneUserDic);
    SettingsDlgInstance->GetAdvancedDlg()->FillDelimiters(DelimUtf8.c_str());
    SettingsDlgInstance->GetAdvancedDlg()->SetRecheckDelay(m_recheckDelay);
    SettingsDlgInstance->GetAdvancedDlg()->SetConversionOpts(
        IgnoreYo, ConvertSingleQuotes, RemoveBoundaryApostrophes);
    SettingsDlgInstance->GetAdvancedDlg()->SetUnderlineSettings(UnderlineColor,
                                                                UnderlineStyle);
    SettingsDlgInstance->GetAdvancedDlg()->SetIgnore(
        IgnoreNumbers, IgnoreCStart, IgnoreCHave, IgnoreCAll, Ignore_,
        IgnoreSEApostrophe, IgnoreOneLetter);
    SettingsDlgInstance->GetAdvancedDlg()->SetSuggBoxSettings(SBSize, SBTrans);
    SettingsDlgInstance->GetAdvancedDlg()->SetBufferSize(BufferSize / 1024);
    if (!NoDisplayCall)
        SettingsDlgInstance->display();
}

void SpellChecker::RecheckVisibleBothViews() {
    LRESULT OldLexer = Lexer;
    EncodingType OldEncoding = CurrentEncoding;
    Lexer = SendMsgToActiveEditor(NppDataInstance->_scintillaMainHandle, SCI_GETLEXER);
    CurrentScintilla = NppDataInstance->_scintillaMainHandle;
    RecheckVisible();

    CurrentScintilla = NppDataInstance->_scintillaSecondHandle;
    Lexer = SendMsgToActiveEditor(NppDataInstance->_scintillaSecondHandle, SCI_GETLEXER);
    RecheckVisible();
    Lexer = OldLexer;
    CurrentEncoding = OldEncoding;
    AspellSpeller->SetEncoding(CurrentEncoding);
    HunspellSpeller->SetEncoding(CurrentEncoding);
}

void SpellChecker::applySettings() {
    SettingsDlgInstance->GetSimpleDlg()->ApplySettings(this);
    SettingsDlgInstance->GetAdvancedDlg()->ApplySettings(this);
    FillDialogs(true);
    SaveSettings();
    CheckFileName(); // Cause filters may change
    RefreshUnderlineStyle();
    RecheckVisibleBothViews();
}

void SpellChecker::applyMultiLangSettings() {
    LangListInstance->ApplyChoice(this);
    SaveSettings();
}

void SpellChecker::applyProxySettings() {
    GetSelectProxy()->ApplyChoice(this);
    SaveSettings();
}

void SpellChecker::showSuggestionMenu() {
    FillSuggestionsMenu(SuggestionsInstance->GetPopupMenu());
    SendMessage(SuggestionsInstance->getHSelf(), WM_SHOWANDRECREATEMENU, 0, 0);
}


void SpellChecker::fillDownloadDicsDialog() {
    FillDownloadDics();
    GetDownloadDics()->FillFileList();
}

void SpellChecker::updateSelectProxy() {
    GetSelectProxy()->SetOptions(UseProxy, ProxyHostName.c_str(), ProxyUserName.c_str(),
                                 ProxyPassword.c_str(), ProxyPort, ProxyAnonymous,
                                 ProxyType);
}

void SpellChecker::updateFromRemoveDicsOptions() {
    GetRemoveDics()->UpdateOptions(this);
    SaveSettings();
}

void SpellChecker::updateRemoveDicsOptions() {
    GetRemoveDics()->SetCheckBoxes(RemoveUserDics, RemoveSystem);
}

void SpellChecker::updateFromDownloadDicsOptions() {
    GetDownloadDics()->UpdateOptions(this);
    GetDownloadDics()->UpdateListBox();
    SaveSettings();
}

void SpellChecker::updateFromDownloadDicsOptionsNoUpdate() {
    GetDownloadDics()->UpdateOptions(this);
    SaveSettings();
}

void SpellChecker::libChange() {
    SettingsDlgInstance->GetSimpleDlg()->ApplyLibChange(this);
    SettingsDlgInstance->GetSimpleDlg()->FillLibInfo(
        AspellSpeller->IsWorking()
            ? 2 - (CurrentLangs.empty() ? 1 : 0)
            : 0,
        AspellPath.c_str(), HunspellPath.c_str(), AdditionalHunspellPath.c_str());
    RecheckVisibleBothViews();
    SaveSettings();
}

void SpellChecker::langChange() {
    Lexer = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLEXER);
    RecheckVisible();
}

void SpellChecker::SetRemoveUserDics(bool Value) { RemoveUserDics = Value; }

void SpellChecker::SetRemoveSystem(bool Value) { RemoveSystem = Value; }

bool SpellChecker::GetRemoveUserDics() { return RemoveUserDics; }

bool SpellChecker::GetRemoveSystem() { return RemoveSystem; }

void SpellChecker::DoPluginMenuInclusion(bool Invalidate) {
    MENUITEMINFO Mif;
    HMENU DSpellCheckMenu = GetDSpellCheckMenu();
    if (!DSpellCheckMenu)
        return;
    HMENU LangsSubMenu = GetLangsSubMenu(DSpellCheckMenu);
    if (LangsSubMenu)
        DestroyMenu(LangsSubMenu);
    const wchar_t* CurLang = (LibMode == 1) ? HunspellLanguage.c_str() : AspellLanguage.c_str();
    HMENU NewMenu = CreatePopupMenu();
    if (!Invalidate) {
        if (!CurrentLangs.empty()) {
            int i = 0;
            for (auto& lang : CurrentLangs) {
                int Checked = (CurLang == lang.OrigName)
                                  ? (MFT_RADIOCHECK | MF_CHECKED)
                                  : MF_UNCHECKED;
                bool Res = AppendMenu(NewMenu, MF_STRING | Checked,
                                      GetUseAllocatedIds()
                                          ? i + GetLangsMenuIdStart()
                                          : MAKEWORD(i, LANGUAGE_MENU_ID),
                                      DecodeNames
                                          ? lang.AliasName.c_str()
                                          : lang.OrigName.c_str());
                if (!Res)
                    return;
                ++i;
            }
            int Checked = (wcscmp(CurLang, L"<MULTIPLE>") == 0)
                              ? (MFT_RADIOCHECK | MF_CHECKED)
                              : MF_UNCHECKED;
            AppendMenu(NewMenu, MF_STRING | Checked,
                       GetUseAllocatedIds()
                           ? MULTIPLE_LANGS + GetLangsMenuIdStart()
                           : MAKEWORD(MULTIPLE_LANGS, LANGUAGE_MENU_ID),
                       L"Multiple Languages");
            AppendMenu(NewMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenu(NewMenu, MF_STRING,
                       GetUseAllocatedIds()
                           ? CUSTOMIZE_MULTIPLE_DICS + GetLangsMenuIdStart()
                           : MAKEWORD(CUSTOMIZE_MULTIPLE_DICS, LANGUAGE_MENU_ID),
                       L"Set Multiple Languages...");
            if (LibMode == 1) // Only Hunspell supported
            {
                AppendMenu(NewMenu, MF_STRING,
                           GetUseAllocatedIds()
                               ? DOWNLOAD_DICS + GetLangsMenuIdStart()
                               : MAKEWORD(DOWNLOAD_DICS, LANGUAGE_MENU_ID),
                           L"Download More Languages...");
                AppendMenu(NewMenu, MF_STRING,
                           GetUseAllocatedIds()
                               ? REMOVE_DICS + GetLangsMenuIdStart()
                               : MAKEWORD(REMOVE_DICS, LANGUAGE_MENU_ID),
                           L"Remove Unneeded Languages...");
            }
        }
        else if (LibMode == 1)
            AppendMenu(NewMenu, MF_STRING,
                       GetUseAllocatedIds()
                           ? DOWNLOAD_DICS + GetLangsMenuIdStart()
                           : MAKEWORD(DOWNLOAD_DICS, LANGUAGE_MENU_ID),
                       L"Download Languages...");
    }

    Mif.fMask = MIIM_SUBMENU | MIIM_STATE;
    Mif.cbSize = sizeof(MENUITEMINFO);
    Mif.hSubMenu = NewMenu;
    Mif.fState = MFS_ENABLED;

    SetMenuItemInfo(DSpellCheckMenu, QUICK_LANG_CHANGE_ITEM, true, &Mif);
}

void SpellChecker::FillDownloadDics() {
    GetDownloadDics()->SetOptions(ShowOnlyKnown, InstallSystem);
}

void SpellChecker::ResetDownloadCombobox() {
    HWND TargetCombobox = GetDlgItem(GetDownloadDics()->getHSelf(), IDC_ADDRESS);
    wchar_t Buf[DEFAULT_BUF_SIZE];
    ComboBox_GetText(TargetCombobox, Buf, DEFAULT_BUF_SIZE);
    if (AddressIsSet) {
        PreserveCurrentAddressIndex();
    }
    ComboBox_ResetContent(TargetCombobox);
    for (int i = 0; i < static_cast<int>(countof(DefaultServers)); i++) {
        ComboBox_AddString(TargetCombobox, DefaultServers[i].c_str ());
    }
    for (int i = 0; i < static_cast<int>(countof(ServerNames)); i++) {
        if (!ServerNames[i].empty())
            ComboBox_AddString(TargetCombobox, ServerNames[i].c_str ());
    }
    if (LastUsedAddress < USER_SERVER_CONST)
        ComboBox_SetCurSel(TargetCombobox, LastUsedAddress);
    else
        ComboBox_SetCurSel(TargetCombobox, LastUsedAddress - USER_SERVER_CONST +
        countof(DefaultServers));
    AddressIsSet = 1;
}

void SpellChecker::PreserveCurrentAddressIndex() {
    auto mb_address = GetDownloadDics()->currentAddress();
    if (!mb_address)
        return;
    auto address = *mb_address;
    ftpTrim(address);
    for (int i = 0; i < static_cast<int>(countof(ServerNames)); i++) {
        std::wstring defServer = DefaultServers[i];
        ftpTrim(defServer);
        if (address == defServer) {
            LastUsedAddress = i;
            return;
        }
    };
    for (int i = 0; i < static_cast<int>(countof(ServerNames)); i++) {
        std::wstring server = ServerNames[i];
        if (address == server) {
            LastUsedAddress = USER_SERVER_CONST + i;
            return;
        }
    }
    LastUsedAddress = 0;
}


void SpellChecker::SetCheckComments(bool Value) { checkOnlyCommentsAndString = Value; }

void SpellChecker::CheckFileName() {
    wchar_t FullPath[MAX_PATH];
    CheckTextEnabled = !CheckThose;
    SendMsgToNpp(NppDataInstance, NPPM_GETFULLCURRENTPATH, MAX_PATH, (LPARAM)FullPath);
    for (auto token : tokenize<wchar_t>(FileTypes, LR"(;)")) {
        if (CheckThose) {
            CheckTextEnabled = CheckTextEnabled || PathMatchSpec(FullPath, std::wstring(token).c_str());
            if (CheckTextEnabled)
                break;
        }
        else {
            CheckTextEnabled &= CheckTextEnabled && (!PathMatchSpec(FullPath, std::wstring(token).c_str()));
            if (!CheckTextEnabled)
                break;
        }
    }

    Lexer = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLEXER);
}

LRESULT SpellChecker::GetStyle(int Pos) {
    return SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETSTYLEAT, Pos);
}

void SpellChecker::setRecheckDelay(int value) {
    if (value < 0)
        value = 0;

    if (value > 20000)
        value = 20000;
    m_recheckDelay = value;
}

int SpellChecker::getRecheckDelay() {
    return m_recheckDelay;
}

bool SpellChecker::CheckTextNeeded() {
    return CheckTextEnabled && AutoCheckText;
}

void SpellChecker::HideSuggestionBox() { SuggestionsInstance->display(false); }

void SpellChecker::FindNextMistake() {
    CurrentPosition =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETCURRENTPOS);
    auto CurLine = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_LINEFROMPOSITION,
                                         CurrentPosition);
    auto LineStartPos = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_POSITIONFROMLINE,
                                              CurLine);
    auto DocLength =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLENGTH);
    auto IteratorPos = LineStartPos;
    Sci_TextRange Range;
    bool FullCheck = false;

    while (true) {
        Range.chrg.cpMin = static_cast<long>(IteratorPos);
        Range.chrg.cpMax = static_cast<long>(IteratorPos + BufferSize);
        int IgnoreOffsetting = 0;
        if (Range.chrg.cpMax > DocLength) {
            IgnoreOffsetting = 1;
            Range.chrg.cpMax = static_cast<long>(DocLength);
        }
        std::vector<char> text(Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1);
        Range.lpstrText = text.data();
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&Range));
        char* IteratingStart =
            Range.lpstrText + Range.chrg.cpMax - Range.chrg.cpMin - 1;
        char* IteratingChar = IteratingStart;
        if (!IgnoreOffsetting) {
            if (CurrentEncoding == ENCODING_UTF8) {
                while (utf8_is_cont(*IteratingChar) && Range.lpstrText < IteratingChar)
                    IteratingChar--;

                while ((!utf8_chr(DelimUtf8Converted.c_str(), IteratingChar)) &&
                    Range.lpstrText < IteratingChar) {
                    IteratingChar = utf8_dec(Range.lpstrText, IteratingChar);
                }
            }
            else {
                while (!strchr(DelimConverted.c_str(), *IteratingChar) &&
                    Range.lpstrText < IteratingChar)
                    IteratingChar--;
            }

            *IteratingChar = '\0';
        }
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_COLOURISE, Range.chrg.cpMin,
                              Range.chrg.cpMax);
        SCNotification scn;
        scn.nmhdr.code = SCN_SCROLLED;
        SendMsgToNpp(NppDataInstance, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scn));
        // To fix bug with hotspots being removed
        bool Result = CheckText(Range.lpstrText, static_cast<long>(IteratorPos), FIND_FIRST);
        if (Result)
            break;

        IteratorPos += (BufferSize + IteratingChar - IteratingStart);

        if (IteratorPos > DocLength) {
            if (!FullCheck) {
                CurrentPosition = 0;
                IteratorPos = 0;
                FullCheck = true;
            }
            else
                break;

            if (FullCheck && IteratorPos > CurrentPosition)
                break; // So nothing was found TODO: Message probably
        }
    }
}

void SpellChecker::FindPrevMistake() {
    CurrentPosition =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETCURRENTPOS);
    auto CurLine = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_LINEFROMPOSITION,
                                         CurrentPosition);
    auto DocLength =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLENGTH);
    auto LineEndPos = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLINEENDPOSITION,
                                            CurLine);

    auto IteratorPos = LineEndPos;
    Sci_TextRange Range;
    bool FullCheck = false;

    while (1) {
        Range.chrg.cpMin = static_cast<long>(IteratorPos - BufferSize);
        Range.chrg.cpMax = static_cast<long>(IteratorPos);
        int IgnoreOffsetting = 0;
        if (Range.chrg.cpMin < 0) {
            Range.chrg.cpMin = 0;
            IgnoreOffsetting = 1;
        }
        std::vector<char> text(Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1);
        Range.lpstrText = text.data();
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&Range));
        char* IteratingStart = Range.lpstrText;
        char* IteratingChar = IteratingStart;
        if (!IgnoreOffsetting) {
            if (CurrentEncoding == ENCODING_UTF8) {
                while (utf8_is_cont(*IteratingChar) && *IteratingChar)
                    IteratingChar++;

                while ((!utf8_chr(DelimUtf8Converted.c_str(), IteratingChar)) &&
                    *IteratingChar) {
                    IteratingChar = utf8_inc(IteratingChar);
                }
            }
            else {
                while (!strchr(DelimConverted.c_str(), *IteratingChar) && IteratingChar)
                    IteratingChar++;
            }
        }
        auto offset = IteratingChar - IteratingStart;
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_COLOURISE, Range.chrg.cpMin + offset,
                              Range.chrg.cpMax);
        SCNotification scn;
        scn.nmhdr.code = SCN_SCROLLED;
        SendMsgToNpp(NppDataInstance, WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scn));

        bool Result = CheckText(Range.lpstrText + offset,
                                static_cast<long>(Range.chrg.cpMin + offset), FIND_LAST);
        if (Result)
            break;

        IteratorPos -= (BufferSize - offset);

        if (IteratorPos < 0) {
            if (!FullCheck) {
                CurrentPosition = DocLength + 1;
                IteratorPos = DocLength;
                FullCheck = true;
            }
            else
                break;

            if (FullCheck && IteratorPos < CurrentPosition - 1)
                break; // So nothing was found TODO: Message probably
        }
    }
}

void SpellChecker::SetDefaultDelimiters() {
    SettingsDlgInstance->GetAdvancedDlg()->SetDelimetersEdit(DEFAULT_DELIMITERS);
}

HWND SpellChecker::GetCurrentScintilla() {
    return GetScintillaWindow(NppDataInstance); // TODO: optimize
}

bool SpellChecker::GetWordUnderCursorIsRight(long& Pos, long& Length,
                                             bool UseTextCursor) {
    bool Ret = true;
    POINT p;
    std::ptrdiff_t initCharPos;
    LRESULT SelectionStart = 0;
    LRESULT SelectionEnd = 0;

    if (!UseTextCursor) {
        if (GetCursorPos(&p) == 0)
            return true;

        auto* scintilla = GetScintillaWindow(NppDataInstance);
        if (!scintilla)
            return true;
        ScreenToClient(scintilla, &p);

        initCharPos = SendMsgToActiveEditor(
            GetCurrentScintilla(), SCI_CHARPOSITIONFROMPOINTCLOSE, p.x, p.y);
    }
    else {
        SelectionStart = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETSELECTIONSTART
        );
        SelectionEnd =
            SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETSELECTIONEND);
        initCharPos =
            SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETCURRENTPOS);
    }

    if (initCharPos != -1) {
        auto Line = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_LINEFROMPOSITION,
                                          initCharPos);
        auto LineLength =
            SendMsgToActiveEditor(GetCurrentScintilla(), SCI_LINELENGTH, Line);
        std::vector<char> Buf(LineLength + 1);
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLINE, Line, reinterpret_cast<LPARAM>(Buf.data()));
        Buf[LineLength] = 0;
        auto Offset = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_POSITIONFROMLINE,
                                            Line);
        auto Word = GetWordAt(static_cast<long>(initCharPos), Buf.data(),
                              static_cast<long>(Offset));
        if (Word.empty()) {
            Ret = true;
        }
        else {
            std::string_view sv = Word;
            CutApostrophes(sv);
            Pos = static_cast<long>(sv.data () - Buf.data() + Offset);
            long PosEnd = Pos + static_cast<long> (sv.length ());
            long WordLen = PosEnd - Pos;
            if (SelectionStart != SelectionEnd &&
                (SelectionStart != Pos || SelectionEnd != Pos + WordLen)) {
                return true;
            }
            if (CheckWord(std::string (sv), Pos, Pos + WordLen - 1)) {
                Ret = true;
            }
            else {
                Ret = false;
                Length = WordLen;
            }
        }
    }
    return Ret;
}

std::string_view SpellChecker::GetWordAt(long CharPos, char* Text, long Offset) const {
    char* UsedText;
    char* Iterator = Text + CharPos - Offset;

    if (CurrentEncoding == ENCODING_UTF8) {
        if (utf8_chr(DelimUtf8Converted.c_str(), Iterator))
            Iterator = utf8_dec(Text, Iterator);

        if (Iterator == nullptr)
            return nullptr;

        while ((!utf8_chr(DelimUtf8Converted.c_str(), Iterator)) && Text < Iterator)
            Iterator = (char *)utf8_dec(Text, Iterator);
    }
    else {
        if (strchr(DelimConverted.c_str(), *Iterator))
            Iterator--;
        if (Iterator < Text)
            return nullptr;

        while (!strchr(DelimConverted.c_str(), *Iterator) && Text < Iterator)
            Iterator--;

        if (Iterator < Text)
            return nullptr;
    }

    UsedText = Iterator;

    if (CurrentEncoding == ENCODING_UTF8) {
        if (utf8_chr(DelimUtf8Converted.c_str(), UsedText))
            UsedText = utf8_inc(UsedText); // Then find first token after this zero
    }
    else {
        if (strchr(DelimConverted.c_str(), *UsedText))
            UsedText++;
    }

    // We're just taking the first token (basically repeating the same code as an
    // in CheckVisible

    std::string_view Res;
    if (CurrentEncoding == ENCODING_UTF8)
        {
          auto end = utf8_pbrk(UsedText, DelimUtf8Converted.c_str());
          if (!end) return {};
          Res = {UsedText, static_cast<size_t> (end - UsedText)};
        }
    else
        {
          auto end = strpbrk (UsedText, DelimConverted.c_str());
          if (!end) return {};
          Res = {UsedText, static_cast<size_t> (end - UsedText)};
        }
    if (Res.data () - Text + Offset > CharPos)
        return {};
    else
        return Res;
}

void SpellChecker::SetSuggestionsBoxTransparency() {
    // Set WS_EX_LAYERED on this window
    SetWindowLong(SuggestionsInstance->getHSelf(), GWL_EXSTYLE,
                  GetWindowLong(SuggestionsInstance->getHSelf(), GWL_EXSTYLE) |
                  WS_EX_LAYERED);
    SetLayeredWindowAttributes(SuggestionsInstance->getHSelf(), 0,
                               static_cast<BYTE>((255 * SBTrans) / 100),
                               LWA_ALPHA);
    SuggestionsInstance->display(true);
    SuggestionsInstance->display(false);
}

void SpellChecker::InitSuggestionsBox() {
    if (SuggestionsMode != SUGGESTIONS_BOX)
        return;
    if (!CurrentSpeller->IsWorking())
        return;
    POINT p;
    if (!CheckTextNeeded()) // If there's no red underline let's do nothing
    {
        SuggestionsInstance->display(false);
        return;
    }

    GetCursorPos(&p);
    auto* scintilla = GetScintillaWindow(NppDataInstance);
    if (!scintilla || WindowFromPoint(p) != scintilla) {
        return;
    }

    long Pos, Length;
    if (GetWordUnderCursorIsRight(Pos, Length)) {
        return;
    }
    WUCLength = Length;
    WUCPosition = Pos;
    auto Line = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_LINEFROMPOSITION,
                                      WUCPosition);
    auto TextHeight =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_TEXTHEIGHT, Line);
    auto XPos = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_POINTXFROMPOSITION,
                                      0, WUCPosition);
    auto YPos = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_POINTYFROMPOSITION,
                                      0, WUCPosition);

    p.x = static_cast<LONG>(XPos);
    p.y = static_cast<LONG>(YPos);
    RECT R;
    GetWindowRect(GetCurrentScintilla(), &R);
    scintilla = GetScintillaWindow(NppDataInstance);
    if (!scintilla)
        return;

    ClientToScreen(scintilla, &p);
    if (R.top > p.y + TextHeight - 3 || R.left > p.x ||
        R.bottom < p.y + TextHeight - 3 + SBSize || R.right < p.x + SBSize)
        return;
    MoveWindow(SuggestionsInstance->getHSelf(), p.x,
               p.y + static_cast<int>(TextHeight) - 3, SBSize, SBSize, true);
    SuggestionsInstance->display(true, false);
}

void SpellChecker::ProcessMenuResult(WPARAM MenuId) {
    if ((!GetUseAllocatedIds() && HIBYTE(MenuId) != DSPELLCHECK_MENU_ID &&
            HIBYTE(MenuId) != LANGUAGE_MENU_ID) ||
        (GetUseAllocatedIds() && ((int)MenuId < GetContextMenuIdStart() ||
            (int)MenuId > GetContextMenuIdStart() + 350)))
        return;
    int UsedMenuId;
    if (GetUseAllocatedIds()) {
        UsedMenuId = ((int)MenuId < GetLangsMenuIdStart()
                          ? DSPELLCHECK_MENU_ID
                          : LANGUAGE_MENU_ID);
    }
    else {
        UsedMenuId = HIBYTE(MenuId);
    }

    switch (UsedMenuId) {
    case DSPELLCHECK_MENU_ID:
        {
            WPARAM Result;
            if (!GetUseAllocatedIds())
                Result = LOBYTE(MenuId);
            else
                Result = MenuId - GetContextMenuIdStart();

            if (Result != 0) {
                if (Result == MID_IGNOREALL) {
                    ApplyConversions(SelectedWord);
                    CurrentSpeller->IgnoreAll(SelectedWord.c_str());
                    WUCLength = SelectedWord.length();
                    SendMsgToActiveEditor(GetCurrentScintilla(), SCI_SETSEL, WUCPosition + WUCLength,
                                          WUCPosition + WUCLength);
                    RecheckVisibleBothViews();
                }
                else if (Result == MID_ADDTODICTIONARY) {
                    ApplyConversions(SelectedWord);
                    CurrentSpeller->AddToDictionary(SelectedWord.c_str());
                    WUCLength = SelectedWord.length();
                    SendMsgToActiveEditor(GetCurrentScintilla(), SCI_SETSEL, WUCPosition + WUCLength,
                                          WUCPosition + WUCLength);
                    RecheckVisibleBothViews();
                }
                else if ((unsigned int)Result <= LastSuggestions.size()) {
                    std::string ansiStr;
                    if (CurrentEncoding == ENCODING_ANSI)
                        ansiStr = utf8_to_string(LastSuggestions[Result - 1].c_str());
                    else
                        ansiStr = LastSuggestions[Result - 1];

                    SendMsgToActiveEditor(GetCurrentScintilla(), SCI_REPLACESEL, 0,
                                          reinterpret_cast<LPARAM>(ansiStr.c_str()));
                }
            }
        }
        break;
    case LANGUAGE_MENU_ID:
        {
            WPARAM Result;
            if (!GetUseAllocatedIds())
                Result = LOBYTE(MenuId);
            else
                Result = MenuId - GetLangsMenuIdStart();

            const wchar_t* LangString;
            if (Result == MULTIPLE_LANGS) {
                LangString = L"<MULTIPLE>";
            }
            else if (Result == CUSTOMIZE_MULTIPLE_DICS || Result == DOWNLOAD_DICS ||
                Result == REMOVE_DICS) {
                // All actions are done in GUI thread in that case
                return;
            }
            else
                LangString = CurrentLangs[Result].OrigName.c_str();
            DoPluginMenuInclusion(true);

            if (LibMode == 0)
                SetAspellLanguage(LangString);
            else
                SetHunspellLanguage(LangString);

            ReinitLanguageLists(true);
            UpdateLangsMenu();
            RecheckVisibleBothViews();
            SaveSettings();
            break;
        }
    default:
        break;
    }
}

std::vector<SuggestionsMenuItem> SpellChecker::FillSuggestionsMenu(HMENU Menu) {
    if (!CurrentSpeller->IsWorking())
        return {}; // Word is already off-screen

    int Pos = WUCPosition;
    Sci_TextRange Range;
    Range.chrg.cpMin = WUCPosition;
    Range.chrg.cpMax = WUCPosition + static_cast<long>(WUCLength);
    std::vector<char> text(WUCLength + 1);
    Range.lpstrText = text.data();
    PostMsgToActiveEditor(GetCurrentScintilla(), SCI_SETSEL, Pos,
                          Pos + WUCLength);
    if (SuggestionsMode == SUGGESTIONS_BOX) {
        // PostMsgToEditor (GetCurrentScintilla (), NppDataInstance, SCI_SETSEL,
        // Pos, Pos + WUCLength);
    }
    std::vector<SuggestionsMenuItem> SuggestionMenuItems;
    SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&Range));

    SelectedWord = Range.lpstrText;
    ApplyConversions(SelectedWord);

    LastSuggestions = CurrentSpeller->GetSuggestions(SelectedWord.c_str());

    for (size_t i = 0; i < LastSuggestions.size(); i++) {
        if (i >= static_cast<unsigned int>(SuggestionsNum))
            break;

        auto item = utf8_to_wstring(LastSuggestions[i].c_str());
        if (SuggestionsMode == SUGGESTIONS_BOX)
            InsertSuggMenuItem(Menu, item.c_str(), static_cast<BYTE>(i + 1), -1);
        else
            SuggestionMenuItems.emplace_back(item.c_str(), static_cast<BYTE>(i + 1));
    }

    if (!LastSuggestions.empty()) {
        if (SuggestionsMode == SUGGESTIONS_BOX)
            InsertSuggMenuItem(Menu, L"", 0, 103, true);
        else
            SuggestionMenuItems.emplace_back(L"", 0, true);
    }

    std::string BufUtf8;
    if (CurrentEncoding == ENCODING_UTF8)
        BufUtf8 = Range.lpstrText;
    else
        BufUtf8 = toUtf8String(Range.lpstrText);
    ApplyConversions(BufUtf8);
    auto item = utf8_to_wstring(BufUtf8.c_str());
    auto menuString = wstring_printf(L"Ignore \"%s\" for Current Session", item.c_str());
    if (SuggestionsMode == SUGGESTIONS_BOX)
        InsertSuggMenuItem(Menu, menuString.c_str(), MID_IGNOREALL, -1);
    else
        SuggestionMenuItems.emplace_back(menuString.c_str(), MID_IGNOREALL);
    menuString = wstring_printf(L"Add \"%s\" to Dictionary", item.c_str());;
    if (SuggestionsMode == SUGGESTIONS_BOX)
        InsertSuggMenuItem(Menu, menuString.c_str(), MID_ADDTODICTIONARY, -1);
    else
        SuggestionMenuItems.emplace_back(menuString.c_str(), MID_ADDTODICTIONARY);

    if (SuggestionsMode == SUGGESTIONS_CONTEXT_MENU)
        SuggestionMenuItems.emplace_back(L"", 0, true);

    return SuggestionMenuItems;
}

void SpellChecker::UpdateAutocheckStatus(int SaveSetting) {
    if (SaveSetting)
        SaveSettings();

    SendMsgToNpp(NppDataInstance, NPPM_SETMENUITEMCHECK, get_funcItem()[0]._cmdID,
                 AutoCheckText);
}

void SpellChecker::SetCheckThose(int CheckThoseArg) {
    CheckThose = CheckThoseArg;
}

void SpellChecker::SetFileTypes(const wchar_t* FileTypesArg) {
    FileTypes = FileTypesArg;
}

void SpellChecker::SetHunspellMultipleLanguages(const char* MultiLanguagesArg) {
    HunspellMultiLanguages = to_wstring(MultiLanguagesArg);
}

void SpellChecker::SetAspellMultipleLanguages(const char* MultiLanguagesArg) {
    AspellMultiLanguages = to_wstring(MultiLanguagesArg);
}

void SpellChecker::RefreshUnderlineStyle() {
    SendMsgToBothEditors(NppDataInstance, SCI_INDICSETSTYLE, SCE_ERROR_UNDERLINE,
                         UnderlineStyle);
    SendMsgToBothEditors(NppDataInstance, SCI_INDICSETFORE, SCE_ERROR_UNDERLINE,
                         UnderlineColor);
}

void SpellChecker::SetUnderlineColor(int Value) { UnderlineColor = Value; }

void SpellChecker::SetUnderlineStyle(int Value) { UnderlineStyle = Value; }

void SpellChecker::SetProxyUserName(const wchar_t* Str) {
    ProxyUserName = Str;
}

void SpellChecker::SetProxyHostName(const wchar_t* Str) {
    ProxyHostName = Str;
}

void SpellChecker::SetProxyPassword(const wchar_t* Str) {
    ProxyPassword = Str;
}

void SpellChecker::SetProxyPort(int Value) { ProxyPort = Value; }

void SpellChecker::SetUseProxy(bool Value) { UseProxy = Value; }

void SpellChecker::SetProxyAnonymous(bool Value) { ProxyAnonymous = Value; }

void SpellChecker::SetProxyType(int Value) { ProxyType = Value; }

const wchar_t* SpellChecker::GetProxyUserName() const { return ProxyUserName.c_str(); }

const wchar_t* SpellChecker::GetProxyHostName() const { return ProxyHostName.c_str(); }

const wchar_t* SpellChecker::GetProxyPassword() const { return ProxyPassword.c_str(); }

int SpellChecker::GetProxyPort() { return ProxyPort; }

bool SpellChecker::GetUseProxy() { return UseProxy; }

bool SpellChecker::GetProxyAnonymous() { return ProxyAnonymous; }

int SpellChecker::GetProxyType() { return ProxyType; }

void SpellChecker::SetIgnore(bool IgnoreNumbersArg, bool IgnoreCStartArg,
                             bool IgnoreCHaveArg, bool IgnoreCAllArg,
                             bool Ignore_Arg, bool IgnoreSEApostropheArg,
                             bool IgnoreOneLetterArg) {
    IgnoreNumbers = IgnoreNumbersArg;
    IgnoreCStart = IgnoreCStartArg;
    IgnoreCHave = IgnoreCHaveArg;
    IgnoreCAll = IgnoreCAllArg;
    Ignore_ = Ignore_Arg;
    IgnoreSEApostrophe = IgnoreSEApostropheArg;
    IgnoreOneLetter = IgnoreOneLetterArg;
}

std::wstring SpellChecker::GetDefaultHunspellPath() {
    return IniFilePath.substr(0, IniFilePath.rfind(L'\\')) + L"\\Hunspell";
}

void SpellChecker::SaveSettings() {
    FILE* Fp;
    _wfopen_s(&Fp, IniFilePath.c_str(), L"w"); // Cleaning settings file (or creating it)
    fclose(Fp);
    if (!SettingsLoaded)
        return;
    SaveToIni(L"Autocheck", AutoCheckText, 1);
    SaveToIni(L"Hunspell_Multiple_Languages", HunspellMultiLanguages.c_str(), L"");
    SaveToIni(L"Aspell_Multiple_Languages", AspellMultiLanguages.c_str(), L"");
    SaveToIni(L"Hunspell_Language", HunspellLanguage.c_str(), L"en_GB");
    SaveToIni(L"Aspell_Language", AspellLanguage.c_str(), L"en");
    SaveToIni(L"Remove_User_Dics_On_Dic_Remove", RemoveUserDics, 0);
    SaveToIni(L"Remove_Dics_For_All_Users", RemoveSystem, 0);
    SaveToIni(L"Show_Only_Known", ShowOnlyKnown, true);
    SaveToIni(L"Install_Dictionaries_For_All_Users", InstallSystem, false);
    SaveToIni(L"Recheck_Delay", m_recheckDelay, 500);
    wchar_t Buf[DEFAULT_BUF_SIZE];
    for (int i = 0; i < static_cast<int>(countof(ServerNames)); i++) {
        if (ServerNames[i].empty())
            continue;
        swprintf(Buf, L"Server_Address[%d]", i);
        SaveToIni(Buf, ServerNames[i].c_str(), L"");
    }
    SaveToIni(L"Suggestions_Control", SuggestionsMode, 1);
    SaveToIni(L"Ignore_Yo", IgnoreYo, 0);
    SaveToIni(L"Convert_Single_Quotes_To_Apostrophe", ConvertSingleQuotes, 1);
    SaveToIni(L"Remove_Ending_And_Beginning_Apostrophe",
              RemoveBoundaryApostrophes, 1);
    SaveToIni(L"Check_Only_Comments_And_Strings", checkOnlyCommentsAndString, 1);
    SaveToIni(L"Check_Those_\\_Not_Those", CheckThose, 1);
    SaveToIni(L"File_Types", FileTypes.c_str(), L"*.*");
    SaveToIni(L"Ignore_Having_Number", IgnoreNumbers, 1);
    SaveToIni(L"Ignore_Start_Capital", IgnoreCStart, 0);
    SaveToIni(L"Ignore_Have_Capital", IgnoreCHave, 1);
    SaveToIni(L"Ignore_All_Capital", IgnoreCAll, 1);
    SaveToIni(L"Ignore_With_", Ignore_, 1);
    SaveToIni(L"Ignore_That_Start_or_End_with_'", IgnoreSEApostrophe, 0);
    SaveToIni(L"Ignore_One_Letter", IgnoreOneLetter, 0);
    SaveToIni(L"Underline_Color", UnderlineColor, 0x0000ff);
    SaveToIni(L"Underline_Style", UnderlineStyle, INDIC_SQUIGGLE);
    auto path = GetDefaultAspellPath();
    SaveToIni(L"Aspell_Path", AspellPath.c_str(), path.c_str());
    path = GetDefaultHunspellPath();
    SaveToIni(L"User_Hunspell_Path", HunspellPath.c_str(), path.c_str());
    SaveToIni(L"System_Hunspell_Path", AdditionalHunspellPath.c_str(),
              L".\\plugins\\config\\Hunspell");
    SaveToIni(L"Suggestions_Number", SuggestionsNum, 5);
    SaveToIniUtf8(L"Delimiters", DelimUtf8.c_str(), toUtf8String(DEFAULT_DELIMITERS).c_str(), true);
    SaveToIni(L"Find_Next_Buffer_Size", BufferSize / 1024, 4);
    SaveToIni(L"Suggestions_Button_Size", SBSize, 15);
    SaveToIni(L"Suggestions_Button_Opacity", SBTrans, 70);
    SaveToIni(L"Library", LibMode, 1);
    PreserveCurrentAddressIndex();
    SaveToIni(L"Last_Used_Address_Index", LastUsedAddress, 0);
    SaveToIni(L"Decode_Language_Names", DecodeNames, true);
    SaveToIni(L"United_User_Dictionary(Hunspell)", OneUserDic, false);

    SaveToIni(L"Use_Proxy", UseProxy, false);
    SaveToIni(L"Proxy_User_Name", ProxyUserName.c_str(), L"anonymous");
    SaveToIni(L"Proxy_Host_Name", ProxyHostName.c_str(), L"");
    SaveToIni(L"Proxy_Password", ProxyPassword.c_str(), L"");
    SaveToIni(L"Proxy_Port", ProxyPort, 808);
    SaveToIni(L"Proxy_Is_Anonymous", ProxyAnonymous, true);
    SaveToIni(L"Proxy_Type", ProxyType, 0);
}

void SpellChecker::SetDecodeNames(bool Value) { DecodeNames = Value; }

void SpellChecker::SetOneUserDic(bool Value) {
    OneUserDic = Value;
    HunspellSpeller->SetUseOneDic(Value);
}

bool SpellChecker::GetOneUserDic() { return OneUserDic; }

void SpellChecker::SetLibMode(int i) {
    LibMode = i;
    if (i == 0) {
        AspellReinitSettings();
        CurrentSpeller = AspellSpeller.get();
    }
    else {
        CurrentSpeller = HunspellSpeller.get();
        HunspellReinitSettings(false);
    }
}

void SpellChecker::LoadSettings() {
    SettingsLoaded = true;
    auto Path = GetDefaultAspellPath();
    AspellPath = LoadFromIni(L"Aspell_Path", Path.c_str());
    Path = GetDefaultHunspellPath();
    HunspellPath = LoadFromIni(L"User_Hunspell_Path", Path.c_str());

    AdditionalHunspellPath = LoadFromIni(L"System_Hunspell_Path",
                                         L".\\plugins\\config\\Hunspell");

    LoadFromIni(SuggestionsMode, L"Suggestions_Control", 1);
    LoadFromIni(AutoCheckText, L"Autocheck", true);
    UpdateAutocheckStatus(0);
    AspellMultiLanguages = LoadFromIni(L"Aspell_Multiple_Languages", L"");
    HunspellMultiLanguages = LoadFromIni(L"Hunspell_Multiple_Languages", L"");
    SetAspellLanguage(LoadFromIni(L"Aspell_Language", L"en").c_str());
    SetHunspellLanguage(LoadFromIni(L"Hunspell_Language", L"en_GB").c_str());

    SetDelimiters(LoadFromIniUtf8(L"Delimiters", toUtf8String(DEFAULT_DELIMITERS).c_str()).c_str());
    LoadFromIni(SuggestionsNum, L"Suggestions_Number", 5);
    LoadFromIni(IgnoreYo, L"Ignore_Yo", 0);
    LoadFromIni(ConvertSingleQuotes, L"Convert_Single_Quotes_To_Apostrophe", 1);
    LoadFromIni(RemoveBoundaryApostrophes,
                L"Remove_Ending_And_Beginning_Apostrophe", 1);
    LoadFromIni(CheckThose, L"Check_Those_\\_Not_Those", 1);
    FileTypes = LoadFromIni(L"File_Types", L"*.*");
    LoadFromIni(checkOnlyCommentsAndString, L"Check_Only_Comments_And_Strings", 1);
    LoadFromIni(UnderlineColor, L"Underline_Color", 0x0000ff);
    LoadFromIni(UnderlineStyle, L"Underline_Style", INDIC_SQUIGGLE);
    LoadFromIni(IgnoreNumbers, L"Ignore_Having_Number", 1);
    LoadFromIni(IgnoreCStart, L"Ignore_Start_Capital", 0);
    LoadFromIni(IgnoreCHave, L"Ignore_Have_Capital", 1);
    LoadFromIni(IgnoreCAll, L"Ignore_All_Capital", 1);
    LoadFromIni(IgnoreOneLetter, L"Ignore_One_Letter", 0);
    LoadFromIni(Ignore_, L"Ignore_With_", 1);
    int Value;
    LoadFromIni(Value, L"United_User_Dictionary(Hunspell)", false);
    SetOneUserDic(Value);
    LoadFromIni(IgnoreSEApostrophe, L"Ignore_That_Start_or_End_with_'", 0);

    HunspellSpeller->SetDirectory(HunspellPath.c_str());
    HunspellSpeller->SetAdditionalDirectory(AdditionalHunspellPath.c_str());
    AspellSpeller->Init(AspellPath.c_str());
    int x;
    LoadFromIni(x, L"Library", 1);
    SetLibMode(x);
    int Size, Trans;
    LoadFromIni(Size, L"Suggestions_Button_Size", 15);
    LoadFromIni(Trans, L"Suggestions_Button_Opacity", 70);
    SetSuggBoxSettings(Size, Trans, 0);
    LoadFromIni(Size, L"Find_Next_Buffer_Size", 4);
    SetBufferSize(Size);
    RefreshUnderlineStyle();
    LoadFromIni(ShowOnlyKnown, L"Show_Only_Known", true);
    LoadFromIni(InstallSystem, L"Install_Dictionaries_For_All_Users", false);
    LoadFromIni(m_recheckDelay, L"Recheck_Delay", 500);
    wchar_t Buf[DEFAULT_BUF_SIZE];
    for (int i = 0; i < static_cast<int>(countof(ServerNames)); i++) {
        swprintf(Buf, L"Server_Address[%d]", i);
        ServerNames[i] = LoadFromIni(Buf, L"");
    }
    LoadFromIni(LastUsedAddress, L"Last_Used_Address_Index", 0);
    LoadFromIni(RemoveUserDics, L"Remove_User_Dics_On_Dic_Remove", 0);
    LoadFromIni(RemoveSystem, L"Remove_Dics_For_All_Users", 0);
    LoadFromIni(DecodeNames, L"Decode_Language_Names", true);

    LoadFromIni(UseProxy, L"Use_Proxy", false);
    LoadFromIni(ProxyUserName.c_str(), L"Proxy_User_Name", L"anonymous");
    LoadFromIni(ProxyHostName.c_str(), L"Proxy_Host_Name", L"");
    LoadFromIni(ProxyPassword.c_str(), L"Proxy_Password", L"");
    LoadFromIni(ProxyPort, L"Proxy_Port", 808);
    LoadFromIni(ProxyAnonymous, L"Proxy_Is_Anonymous", true);
    LoadFromIni(ProxyType, L"Proxy_Type", 0);
}

void SpellChecker::CreateWordUnderline(HWND ScintillaWindow, long start,
                                       long end) {
    PostMsgToActiveEditor(ScintillaWindow, SCI_SETINDICATORCURRENT,
                          SCE_ERROR_UNDERLINE);
    PostMsgToActiveEditor(ScintillaWindow, SCI_INDICATORFILLRANGE, start,
                          (end - start + 1));
}

void SpellChecker::RemoveUnderline(HWND ScintillaWindow, long start, long end) {
    if (end < start)
        return;
    PostMsgToActiveEditor(ScintillaWindow, SCI_SETINDICATORCURRENT,
                          SCE_ERROR_UNDERLINE);
    PostMsgToActiveEditor(ScintillaWindow, SCI_INDICATORCLEARRANGE, start,
                          (end - start + 1));
}

void SpellChecker::GetVisibleLimits(long& Start, long& Finish) {
    auto top = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETFIRSTVISIBLELINE
    );
    auto bottom = top + SendMsgToActiveEditor(GetCurrentScintilla(), SCI_LINESONSCREEN
    );
    top = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_DOCLINEFROMVISIBLE,
                                top);

    bottom = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_DOCLINEFROMVISIBLE,
                                   bottom);
    auto LineCount =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLINECOUNT);
    Start = static_cast<long>(SendMsgToActiveEditor(GetCurrentScintilla(), SCI_POSITIONFROMLINE,
                                                    top));
    // Not using end of line position cause utf-8 symbols could be more than one
    // char
    // So we use next line start as the end of our visible text
    if (bottom + 1 < LineCount) {
        Finish = static_cast<long>(SendMsgToActiveEditor(
            GetCurrentScintilla(), SCI_POSITIONFROMLINE, bottom + 1));
    }
    else {
        Finish = static_cast<long>(
            SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETTEXTLENGTH));
    }
    return;
}

std::vector<char> SpellChecker::GetVisibleText(long* offset, bool NotIntersectionOnly) {
    Sci_TextRange range;
    GetVisibleLimits(range.chrg.cpMin, range.chrg.cpMax);

    if (range.chrg.cpMax < 0 || range.chrg.cpMin > range.chrg.cpMax)
        return {};

    PreviousA = range.chrg.cpMin;
    PreviousB = range.chrg.cpMax;

    if (NotIntersectionOnly) {
        if (range.chrg.cpMin < PreviousA && range.chrg.cpMax >= PreviousA)
            range.chrg.cpMax = PreviousA - 1;
        else if (range.chrg.cpMax > PreviousB && range.chrg.cpMin <= PreviousB)
            range.chrg.cpMin = PreviousB + 1;
    }

    std::vector<char> buf(range.chrg.cpMax - range.chrg.cpMin + 1);
    range.lpstrText = buf.data();
    SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETTEXTRANGE, NULL, reinterpret_cast<LPARAM>(&range));
    *offset = range.chrg.cpMin;
    buf[range.chrg.cpMax - range.chrg.cpMin] = 0;
    return buf;
}

void SpellChecker::ClearAllUnderlines() {
    auto length =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLENGTH);
    if (length > 0) {
        PostMsgToActiveEditor(GetCurrentScintilla(), SCI_SETINDICATORCURRENT,
                              SCE_ERROR_UNDERLINE);
        PostMsgToActiveEditor(GetCurrentScintilla(), SCI_INDICATORCLEARRANGE, 0,
                              length);
    }
}

void SpellChecker::SetAspellPath(const wchar_t* Path) {
    AspellPath = Path;
    AspellReinitSettings();
}

void SpellChecker::SetHunspellPath(const wchar_t* Path) {
    HunspellPath = Path;
    HunspellReinitSettings(1);
}

void SpellChecker::SetHunspellAdditionalPath(const wchar_t* Path) {
    if (!*Path)
        return;
    AdditionalHunspellPath = Path;
    HunspellReinitSettings(1);
}

void SpellChecker::SaveToIni(const wchar_t* Name, const wchar_t* Value,
                             const wchar_t* DefaultValue, bool InQuotes) {
    if (!Name || !Value)
        return;

    if (DefaultValue && wcscmp(Value, DefaultValue) == 0)
        return;

    if (InQuotes) {
        WritePrivateProfileString(L"SpellCheck", Name, wstring_printf(LR"("%s")", Value).c_str(), IniFilePath.c_str());
    }
    else {
        WritePrivateProfileString(L"SpellCheck", Name, Value, IniFilePath.c_str());
    }
}

void SpellChecker::SaveToIni(const wchar_t* Name, int Value, int DefaultValue) {
    if (!Name)
        return;

    if (Value == DefaultValue)
        return;

    wchar_t Buf[DEFAULT_BUF_SIZE];
    _itow_s(Value, Buf, 10);
    SaveToIni(Name, Buf, nullptr);
}

void SpellChecker::SaveToIniUtf8(const wchar_t* Name, const char* Value,
                                 const char* DefaultValue, bool InQuotes) {
    if (!Name || !Value)
        return;

    if (DefaultValue && strcmp(Value, DefaultValue) == 0)
        return;

    SaveToIni(Name, utf8_to_wstring(Value).c_str(), nullptr, InQuotes);
}

std::wstring SpellChecker::LoadFromIni(const wchar_t* Name,
                                       const wchar_t* defaultValue, bool InQuotes) {
    assert (Name && defaultValue);

    auto value = readIniValue(L"Spellcheck", Name, defaultValue, IniFilePath.c_str());

    if (InQuotes) {
        // Proof check for quotes
        if (value.front() != '\"' || value.back() != '\"' || value.length() < 2) {
            return defaultValue;
        }

        return value.substr(1, value.length() - 2);
    }
    return value;
}

void SpellChecker::LoadFromIni(int& Value, const wchar_t* Name,
                               int defaultValue) {
    if (!Name)
        return;

    auto buf = LoadFromIni(Name, std::to_wstring(defaultValue).c_str());
    Value = _wtoi(buf.c_str());
}

void SpellChecker::LoadFromIni(bool& Value, const wchar_t* Name,
                               bool DefaultValue) {
    if (!Name)
        return;

    auto buf = LoadFromIni(Name, std::to_wstring(DefaultValue).c_str());
    Value = _wtoi(buf.c_str()) != 0;
}

std::string SpellChecker::LoadFromIniUtf8(const wchar_t* Name,
                                          const char* defaultValue, bool InQuotes) {
    if (!Name || !defaultValue)
        return defaultValue;
    return toUtf8String(LoadFromIni(Name, utf8_to_wstring(defaultValue).c_str(), InQuotes).c_str());
}

// Here parameter is in ANSI (may as well be utf-8 cause only English I guess)
void SpellChecker::SetAspellLanguage(const wchar_t* Str) {
    AspellLanguage = Str;

    if (wcscmp(Str, L"<MULTIPLE>") == 0) {
        SetMultipleLanguages(AspellMultiLanguages.c_str(), AspellSpeller.get());
        AspellSpeller->SetMode(1);
    }
    else {
        AspellSpeller->SetLanguage(Str);
        CurrentSpeller->SetMode(0);
    }
}

void SpellChecker::SetHunspellLanguage(const wchar_t* Str) {
    HunspellLanguage = Str;

    if (wcscmp(Str, L"<MULTIPLE>") == 0) {
        SetMultipleLanguages(HunspellMultiLanguages.c_str(), HunspellSpeller.get());
        HunspellSpeller->SetMode(1);
    }
    else {
        HunspellSpeller->SetLanguage(HunspellLanguage.c_str());
        HunspellSpeller->SetMode(0);
    }
}

const char* SpellChecker::GetDelimiters() { return DelimUtf8.c_str(); }

void SpellChecker::SetSuggestionsNum(int Num) { SuggestionsNum = Num; }

// Here parameter is in UTF-8
void SpellChecker::SetDelimiters(const char* Str) {
    DelimUtf8 = Str;
    DelimUtf8Converted = toUtf8String((parseString(utf8_to_wstring(DelimUtf8.c_str()).c_str()) + L" \n\r\t\v").c_str());
    DelimConverted = utf8_to_string(DelimUtf8Converted.c_str());
}

void SpellChecker::SetMultipleLanguages(std::wstring_view MultiString,
                                        AbstractSpellerInterface* Speller) {
    std::vector<std::wstring> MultiLangList;
    for (auto token : tokenize<wchar_t>(MultiString, LR"(\|)"))
        MultiLangList.push_back(std::wstring{token});

    Speller->SetMultipleLanguages(MultiLangList);
}

bool SpellChecker::HunspellReinitSettings(bool ResetDirectory) {
    if (ResetDirectory) {
        HunspellSpeller->SetDirectory(HunspellPath.c_str());
        HunspellSpeller->SetAdditionalDirectory(AdditionalHunspellPath.c_str());
    }
    if (wcscmp(HunspellLanguage.c_str(), L"<MULTIPLE>") != 0)
        HunspellSpeller->SetLanguage(HunspellLanguage.c_str());
    else
        SetMultipleLanguages(HunspellMultiLanguages.c_str(), HunspellSpeller.get());
    return true;
}

bool SpellChecker::AspellReinitSettings() {
    AspellSpeller->Init(AspellPath.c_str());

    if (AspellLanguage == L"<MULTIPLE>") {
        AspellSpeller->SetLanguage(AspellLanguage.c_str());
    }
    else
        SetMultipleLanguages(AspellMultiLanguages.c_str(), AspellSpeller.get());
    return true;
}

void SpellChecker::SetBufferSize(int Size) {
    if (Size < 1)
        Size = 1;
    if (Size > 10 * 1024)
        Size = 10 * 1024;
    BufferSize = Size * 1024;
}

void SpellChecker::SetSuggBoxSettings(int Size, int Transparency, int SaveIni) {
    if (SBSize != Size) {
        SBSize = Size;
        if (SaveIni)
            HideSuggestionBox();
    }

    if (Transparency != SBTrans) {
        SBTrans = Transparency;
        // Don't sure why but this helps to fix a bug with notepad++ window resizing
        // TODO: Fix it normal way
        SetLayeredWindowAttributes(SuggestionsInstance->getHSelf(), 0,
                                   static_cast<BYTE>((255 * SBTrans) / 100),
                                   LWA_ALPHA);
    }
}

void SpellChecker::ApplyConversions(
    std::string& Word) // In Utf-8, Maybe shortened during conversion
{
    const char* ConvertFrom[3];
    const char* ConvertTo[3];
    int Apply[3] = {IgnoreYo, IgnoreYo, ConvertSingleQuotes};

    if (CurrentEncoding == ENCODING_ANSI) {
        ConvertFrom[0] = YoANSI.c_str();
        ConvertFrom[1] = yoANSI.c_str();
        ConvertFrom[2] = PunctuationApostropheANSI.c_str();
        ConvertTo[0] = YeANSI.c_str();
        ConvertTo[1] = yeANSI.c_str();
        ConvertTo[2] = "\'";
    }
    else {
        ConvertFrom[0] = Yo;
        ConvertFrom[1] = yo;
        ConvertFrom[2] = PunctuationApostrophe;
        ConvertTo[0] = Ye;
        ConvertTo[1] = ye;
        ConvertTo[2] = "\'";
    }

    static_assert (countof (ConvertFrom) == countof (ConvertTo));
    for (int i = 0; i < static_cast<int> (countof (ConvertFrom)); ++i) {
        if (!Apply[i])
            continue;
        replaceAll(Word, ConvertFrom[i], ConvertTo[i]);
    }
}

void SpellChecker::ResetHotSpotCache() {
    memset(HotSpotCache, -1, sizeof(HotSpotCache));
}

bool SpellChecker::CheckWord(std::string Word, long Start, long /*End*/) {
    bool res;
    if (!CurrentSpeller->IsWorking() || Word.empty())
        return true;
    // Well Numbers have same codes for ANSI and Unicode I guess, so
    // If word contains number then it's probably just a number or some crazy name
    auto Style = GetStyle(Start);
    if (checkOnlyCommentsAndString && !SciUtils::isCommentOrString(Lexer, Style))
        return true;

    if (HotSpotCache[Style] == -1) {
        HotSpotCache[Style] = SendMsgToActiveEditor(GetCurrentScintilla(), SCI_STYLEGETHOTSPOT,
                                                    Style);
    }

    if (HotSpotCache[Style] == 1)
        return true;

    ApplyConversions(Word);

    auto SymbolsNum =
        (CurrentEncoding == ENCODING_UTF8) ? utf8_length(Word.c_str()) : Word.length();
    if (SymbolsNum == 0) {
        return true;
    }

    if (IgnoreOneLetter && SymbolsNum == 1) {
        return true;
    }

    if (IgnoreNumbers &&
        (CurrentEncoding == ENCODING_UTF8
             ? utf8_pbrk(Word.c_str(), "0123456789")
             : strpbrk(Word.c_str(), "0123456789")) != nullptr) // Same for UTF-8 and not
    {
        return true;
    }

    if (IgnoreCStart || IgnoreCHave || IgnoreCAll) {
        std::wstring Ts;
        if (CurrentEncoding == ENCODING_UTF8)
            Ts = utf8_to_wstring(Word.c_str());
        else
            Ts = to_wstring(Word.c_str());
        if (IgnoreCStart && IsCharUpper(Ts[0])) {
            return true;
        }
        if (IgnoreCHave || IgnoreCAll) {
            bool AllUpper = IsCharUpper(Ts[0]);
            for (auto c : Ts) {
                if (IsCharUpper(c)) {
                    if (IgnoreCHave) {
                        return true;
                    }
                }
                else
                    AllUpper = false;
            }

            if (AllUpper && IgnoreCAll) {
                return true;
            }
        }
    }

    if (Ignore_ && strchr(Word.c_str(), '_') != nullptr) // I guess the same for UTF-8 and ANSI
    {
        return true;
    }

    auto Len = Word.length();

    if (IgnoreSEApostrophe) {
        if (Word[0] == '\'' || Word[Len - 1] == '\'') {
            return true;
        }
    }

    res = CurrentSpeller->CheckWord(Word.c_str());
    return res;
}

void SpellChecker::CutApostrophes(std::string_view& word) {
    if (RemoveBoundaryApostrophes && word.size() > 1) {
        while (!word.empty() && word.front() == '\'')
            word = word.substr(1);

        while (!word.empty() && word.back() == '\'')
            word = word.substr(0, word.length() - 1);
    }
}

int SpellChecker::CheckTextDefaultAnswer(CheckTextMode Mode) {
    switch (Mode) {
    case SpellChecker::UNDERLINE_ERRORS:
    case SpellChecker::FIND_FIRST:
    case SpellChecker::FIND_LAST:
        return false;
    case SpellChecker::GET_FIRST:
        return -1;
    }
    return -1;
}

int SpellChecker::CheckText(char* TextToCheck, long Offset,
                            CheckTextMode Mode) {
    if (!TextToCheck || !*TextToCheck) {
        return CheckTextDefaultAnswer(Mode);
    }

    HWND ScintillaWindow = GetCurrentScintilla();
    SendMsgToActiveEditor(ScintillaWindow, SCI_GETINDICATORCURRENT);
    bool stop = false;
    long ResultingWordEnd = -1, ResultingWordStart = -1;
    auto TextLen = strlen(TextToCheck);
    std::vector<long> UnderlineBuffer;
    long WordStart = 0;
    long WordEnd = 0;

    std::vector<std::string_view> tokens;
    if (CurrentEncoding == ENCODING_UTF8)
        tokens = tokenize_utf8(TextToCheck, DelimUtf8Converted);
    else
        tokens = tokenize<char>(TextToCheck, DelimConverted);

    for (auto token : tokens) {
        CutApostrophes(token);
        WordStart = Offset + static_cast<long>(token.data() - TextToCheck);
        WordEnd = Offset + static_cast<long>(token.data() - TextToCheck + token.length());
        if (WordEnd < WordStart)
            continue;

        if (!CheckWord(std::string(token), WordStart, WordEnd)) {
            switch (Mode) {
            case UNDERLINE_ERRORS:
                UnderlineBuffer.push_back(WordStart);
                UnderlineBuffer.push_back(WordEnd);
                break;
            case FIND_FIRST:
                if (WordEnd > CurrentPosition) {
                    SendMsgToActiveEditor(GetCurrentScintilla(), SCI_SETSEL, WordStart,
                                          WordEnd);
                    stop = true;
                }
                break;
            case FIND_LAST:
                {
                    if (WordEnd >= CurrentPosition) {
                        stop = true;
                        break;
                    }
                    ResultingWordStart = WordStart;
                    ResultingWordEnd = WordEnd;
                }
                break;
            case GET_FIRST:
                return WordStart;
            }
            if (stop)
                break;
        }
        else {
        }

    }

    if (Mode == UNDERLINE_ERRORS) {
        long PrevPos = Offset;
        for (long i = 0; i < (long)UnderlineBuffer.size() - 1; i += 2) {
            RemoveUnderline(ScintillaWindow, PrevPos, UnderlineBuffer[i] - 1);
            CreateWordUnderline(ScintillaWindow, UnderlineBuffer[i],
                                UnderlineBuffer[i + 1] - 1);
            PrevPos = UnderlineBuffer[i + 1];
        }
        RemoveUnderline(ScintillaWindow, PrevPos,
                        Offset + static_cast<long>(TextLen) - 1);
    }

    // PostMsgToEditor (ScintillaWindow, NppDataInstance, SCI_SETINDICATORCURRENT,
    // oldid);

    switch (Mode) {
    case UNDERLINE_ERRORS:
        return true;
    case FIND_FIRST:
        return stop;
    case GET_FIRST:
        return -1;
    case FIND_LAST:
        if (ResultingWordStart == -1)
            return false;
        else {
            SendMsgToActiveEditor(GetCurrentScintilla(), SCI_SETSEL, ResultingWordStart,
                                  ResultingWordEnd);
            return true;
        }
    };
    return false;
}

void SpellChecker::ClearVisibleUnderlines() {
    auto length =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLENGTH);
    if (length > 0) {
        PostMsgToActiveEditor(GetCurrentScintilla(), SCI_SETINDICATORCURRENT,
                              SCE_ERROR_UNDERLINE);
        PostMsgToActiveEditor(GetCurrentScintilla(), SCI_INDICATORCLEARRANGE, 0,
                              length);
    }
}

void SpellChecker::CheckVisible(bool NotIntersectionOnly) {
    VisibleText = GetVisibleText(&VisibleTextOffset, NotIntersectionOnly);
    CheckText(VisibleText.data(), VisibleTextOffset, UNDERLINE_ERRORS);
}

void SpellChecker::setEncodingById(int EncId) {
    /*
    int CCH;
    char szCodePage[10];
    char *FinalString;
    */
    switch (EncId) {
    case SC_CP_UTF8:
        CurrentEncoding = ENCODING_UTF8;
        // SetEncoding ("utf-8");
        break;
    default:
        {
            CurrentEncoding = ENCODING_ANSI;
        }
    }
    HunspellSpeller->SetEncoding(CurrentEncoding);
    AspellSpeller->SetEncoding(CurrentEncoding);
}

void SpellChecker::SwitchAutoCheck() {
    if (!SettingsLoaded)
        return;
    AutoCheckText = !AutoCheckText;
    UpdateAutocheckStatus();
    RecheckVisibleBothViews();
}

void SpellChecker::RecheckModified() {
    if (!CurrentSpeller->IsWorking()) {
        ClearAllUnderlines();
        return;
    }

    auto FirstModifiedLine = SendMsgToActiveEditor(
        GetCurrentScintilla(), SCI_LINEFROMPOSITION, ModifiedStart);
    auto LastModifiedLine = SendMsgToActiveEditor(
        GetCurrentScintilla(), SCI_LINEFROMPOSITION, ModifiedEnd);
    auto LineCount =
        SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLINECOUNT);
    auto FirstPossiblyModifiedPos = SendMsgToActiveEditor(
        GetCurrentScintilla(), SCI_POSITIONFROMLINE, FirstModifiedLine);

    LRESULT LastPossiblyModifiedPos;
    if (LastModifiedLine + 1 < LineCount) {
        LastPossiblyModifiedPos = SendMsgToActiveEditor(
            GetCurrentScintilla(), SCI_POSITIONFROMLINE, LastModifiedLine + 1);
    }
    else {
        LastPossiblyModifiedPos =
            SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLENGTH);
    }

    Sci_TextRange Range;
    Range.chrg.cpMin = static_cast<long>(FirstPossiblyModifiedPos);
    Range.chrg.cpMax = static_cast<long>(LastPossiblyModifiedPos);
    std::vector<char> buf(Range.chrg.cpMax - Range.chrg.cpMin + 1 + 1);
    Range.lpstrText = buf.data();
    SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETTEXTRANGE, 0, (LPARAM)&Range);

    CheckText(Range.lpstrText, static_cast<long>(FirstPossiblyModifiedPos),
              UNDERLINE_ERRORS);
}

void SpellChecker::SetConversionOptions(bool ConvertYo,
                                        bool ConvertSingleQuotesArg,
                                        bool RemoveBoundaryApostrophesArg) {
    IgnoreYo = ConvertYo;
    ConvertSingleQuotes = ConvertSingleQuotesArg;
    RemoveBoundaryApostrophes = RemoveBoundaryApostrophesArg;
}

void SpellChecker::RecheckVisible(bool NotIntersectionOnly) {
    int CodepageId;
    if (!CurrentSpeller->IsWorking()) {
        ClearAllUnderlines();
        return;
    }

    CodepageId = (int)SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETCODEPAGE,
                                            0, 0);
    setEncodingById(CodepageId); // For now it just changes should we convert it
    // to utf-8 or no
    if (CheckTextNeeded())
        CheckVisible(NotIntersectionOnly);
    else
        ClearAllUnderlines();
}

void SpellChecker::ErrorMsgBox(const wchar_t* message) {
    wchar_t buf[DEFAULT_BUF_SIZE];
    swprintf_s(buf, L"DSpellCheck Error: %ws", message);
    MessageBox(NppDataInstance->_nppHandle, message, L"Error Happened!",
               MB_OK | MB_ICONSTOP);
}

void SpellChecker::copyMisspellingsToClipboard() {
    auto lengthDoc =
        (SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETLENGTH) + 1);

    std::vector<char> buf(lengthDoc);
    SendMsgToActiveEditor(GetCurrentScintilla(), SCI_GETTEXT, lengthDoc, reinterpret_cast<LPARAM>(buf.data()));

    int res = 0;
    std::string str; // Yay for first use of std::stirng
    do {
        res = CheckText(buf.data() + res, res, GET_FIRST);
        if (res != -1) {
            str += std::string(buf.data() + res);
            str += "\n";
        }
        else
            break;

        while (*(buf.data() + res) != 0)
            res++;

        while (*(buf.data() + res) == 0)
            res++;

        if (res >= lengthDoc)
            break;
    }
    while (true);

    std::wstring wchar_str;

    switch (CurrentEncoding) {
    case ENCODING_UTF8:
        wchar_str = utf8_to_wstring(str.c_str());
        break;
    case ENCODING_ANSI:
        wchar_str = to_wstring(str.c_str());
        break;
    }

    const size_t len = (wchar_str.length() + 1) * 2;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), wchar_str.c_str(), len);
    GlobalUnlock(hMem);
    OpenClipboard(nullptr);
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
}

SuggestionsMenuItem::SuggestionsMenuItem(const wchar_t* TextArg, int IdArg,
                                         bool SeparatorArg /*= false*/) {
    Text = TextArg;
    Id = static_cast<BYTE>(IdArg);
    Separator = SeparatorArg;
}
