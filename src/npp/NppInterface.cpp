#include "NppInterface.h"

#include "Notepad_plus_msgs.h"
#include <assert.h>
#include "PluginInterface.h"
#include "menuCmdID.h"
#include "MainDef.h"

NppInterface::NppInterface(const NppData* nppData) : m_npp_data{*nppData}
{
}

LRESULT NppInterface::send_msg_to_npp(UINT Msg, WPARAM wParam, LPARAM lParam) const
{
    return SendMessage(m_npp_data.npp_handle, Msg, wParam, lParam);
}

LRESULT NppInterface::send_msg_to_scintilla(ViewType view, UINT Msg, WPARAM wParam, LPARAM lParam) const
{
    auto handle = m_npp_data.scintilla_main_handle;
    switch (view)
    {
    case ViewType::primary:
        handle = m_npp_data.scintilla_main_handle;
        break;
    case ViewType::secondary:
        handle = m_npp_data.scintilla_second_handle;
        break;
    default: break;
    }
    return SendMessage(handle, Msg, wParam, lParam);
}

std::wstring NppInterface::get_dir_msg(UINT msg) const
{
    std::vector<wchar_t> buf(MAX_PATH);
    send_msg_to_npp(msg, MAX_PATH, reinterpret_cast<LPARAM>(buf.data()));
    return {buf.data()};
}

int NppInterface::to_index(ViewType target)
{
    switch (target)
    {
    case ViewType::primary: return 0;
    case ViewType::secondary: return 1;
    case ViewType::COUNT: break;
    }
    assert(false);
    return 0;
}

NppInterface::ViewType NppInterface::active_view() const
{
    int cur_scintilla = 0;
    send_msg_to_npp(NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&cur_scintilla));
    if (cur_scintilla == 0)
        return ViewType::primary;
    else if (cur_scintilla == 1)
        return ViewType::secondary;

    assert (false);
    return ViewType::primary;
}

bool NppInterface::open_document(std::wstring filename)
{
    return send_msg_to_npp(NPPM_DOOPEN, 0, reinterpret_cast<LPARAM>(filename.data())) == TRUE;
}

bool NppInterface::is_opened(const std::wstring& filename) const
{
    auto filenames = get_open_filenames(ViewTarget::both);
    return std::find(filenames.begin(), filenames.end(), filename) != filenames.end();
}

NppInterface::Codepage NppInterface::get_encoding(ViewType view) const
{
    auto CodepageId = static_cast<int>(send_msg_to_scintilla(view, SCI_GETCODEPAGE, 0, 0));
    if (CodepageId == SC_CP_UTF8)
        return Codepage::utf8;
    else
        return Codepage::ansi;
}

void NppInterface::activate_document(int index, ViewType view)
{
    send_msg_to_npp(NPPM_ACTIVATEDOC, to_index(view), index);
}

void NppInterface::activate_document(const std::wstring& filepath, ViewType view)
{
    auto fnames = get_open_filenames(ViewTarget::both);
    auto it = std::find(fnames.begin(), fnames.end(), filepath);
    if (it == fnames.end()) // exception prbbly
        return;

    activate_document(static_cast<int> (it - fnames.begin()), view);
}

std::wstring NppInterface::active_document_path() const
{
    return get_dir_msg(NPPM_GETFULLCURRENTPATH);
}

void NppInterface::switch_to_file(const std::wstring& path)
{
    send_msg_to_npp(NPPM_SWITCHTOFILE, 0, reinterpret_cast<LPARAM>(path.data()));
}

std::wstring NppInterface::active_file_directory() const
{
    return get_dir_msg(NPPM_GETCURRENTDIRECTORY);
}

void NppInterface::do_command(int id)
{
    send_msg_to_npp(WM_COMMAND, id);
}

void NppInterface::move_active_document_to_other_view()
{
    do_command(IDM_VIEW_GOTO_ANOTHER_VIEW);
}

void NppInterface::add_toolbar_icon(int cmdId, const toolbarIcons* toolBarIconsPtr)
{
    send_msg_to_npp(NPPM_ADDTOOLBARICON, static_cast<WPARAM>(cmdId), reinterpret_cast<LPARAM>(toolBarIconsPtr));
}

std::vector<char> NppInterface::selected_text(ViewType view) const
{
    int sel_buf_size = static_cast<int> (send_msg_to_scintilla(view, SCI_GETSELTEXT, 0, 0));
    if (sel_buf_size > 1) // Because it includes terminating '\0'
    {
        std::vector<char> buf(sel_buf_size);
        send_msg_to_scintilla(view, SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(buf.data()));
        return buf;
    }
    return {};
}

std::vector<char> NppInterface::get_current_line(ViewType view) const
{
    int cur_line = get_current_line_number(view);
    int buf_size = static_cast<int> (send_msg_to_scintilla(view, SCI_LINELENGTH, cur_line) + 1);
    std::vector<char> buf(buf_size);
    int ret = static_cast<int> (send_msg_to_scintilla(view, SCI_GETLINE, cur_line, reinterpret_cast<LPARAM>(buf.data())));
    static_cast<void>(ret);
    assert(ret == buf_size - 1);
    return buf;
}

int NppInterface::get_current_pos(ViewType view) const
{
    return static_cast<int> (send_msg_to_scintilla(view, SCI_GETCURRENTPOS, 0, 0));
}

int NppInterface::get_current_line_number(ViewType view) const
{
    return line_from_position(view, get_current_pos(view));
}

int NppInterface::line_from_position(ViewType view, int position) const
{
    return static_cast<int> (send_msg_to_scintilla(view, SCI_LINEFROMPOSITION, position));
}

std::wstring NppInterface::plugin_config_dir() const
{
    return get_dir_msg(NPPM_GETPLUGINSCONFIGDIR);
}

int NppInterface::position_from_line(NppInterface::ViewType view, int line) const
{
    return static_cast<int> (send_msg_to_scintilla(view, SCI_POSITIONFROMLINE, line));
}

HWND NppInterface::app_handle()
{
    return m_npp_data.npp_handle;
}

std::vector<std::wstring> NppInterface::get_open_filenames(ViewTarget viewTarget) const
{
    int enum_val = -1;

    switch (viewTarget)
    {
    case ViewTarget::primary:
        enum_val = PRIMARY_VIEW;
        break;
    case ViewTarget::secondary:
        enum_val = SECOND_VIEW;
        break;
    case ViewTarget::both:
        enum_val = ALL_OPEN_FILES;
        break;
    }

    ASSERT_RETURN(enum_val >= 0, {});
    int count = static_cast<int> (send_msg_to_npp(NPPM_GETNBOPENFILES, 0, enum_val));

    auto paths = new wchar_t *[count];
    for (int i = 0; i < count; ++i)
        paths[i] = new wchar_t[MAX_PATH];

    int msg = -1;
    switch (viewTarget)
    {
    case ViewTarget::primary:
        msg = NPPM_GETOPENFILENAMESPRIMARY;
        break;
    case ViewTarget::secondary:
        msg = NPPM_GETOPENFILENAMESSECOND;
        break;
    case ViewTarget::both:
        msg = NPPM_GETOPENFILENAMES;
        break;
    }

    ASSERT_RETURN(msg >= 0, {});
    {
        auto ret = send_msg_to_npp(msg, reinterpret_cast<WPARAM>(paths), count);
        ASSERT_RETURN(ret == count, {});
    }

    std::vector<std::wstring> res;
    for (int i = 0; i < count; ++i)
    {
        res.push_back(paths[i]);
        delete[] paths[i];
    }
    delete[] paths;
    return res;
}
