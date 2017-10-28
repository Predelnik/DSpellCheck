#include "winapi.h"
#include "Aclapi.h"
#include <cassert>

std::wstring get_edit_text(HWND edit) {
    auto length = Edit_GetTextLength (edit);
    std::vector<wchar_t> buf(length + 1);
    Edit_GetText(edit, buf.data (), static_cast<int> (buf.size ()));
    return buf.data();
}

bool move_file_and_reset_security_descriptor(const wchar_t* from, const wchar_t* to) {
    // on why it is needed https://blogs.msdn.microsoft.com/oldnewthing/20060824-16/?p=29973/
    auto ret = MoveFile(from, to);
    // here goes exact code from https://stackoverflow.com/a/20009217/1269661
    ACL g_null_acl = {0};
    InitializeAcl(&g_null_acl, sizeof(g_null_acl), ACL_REVISION);
    SetNamedSecurityInfo(const_cast<wchar_t *>(to), SE_FILE_OBJECT,
                         DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
                         nullptr, nullptr, static_cast<PACL>(&g_null_acl), nullptr);
    return ret;
}

static std::wstring get_class_name(HWND hwnd) {
    static const int max_class_name = 256;
    std::vector<wchar_t> buf(max_class_name);
    GetClassName(hwnd, buf.data(), max_class_name);
    return {buf.data()};
}


namespace WinApi
{
    WinBase::~WinBase() {
    }

    void WinBase::set_enabled(bool enabled) {
        EnableWindow(m_hwnd, enabled ? TRUE : FALSE);
    }

    void WinBase::init(HWND hwnd) {
        m_hwnd = hwnd;
        check_hwnd();
        init_impl ();
    }

    void ComboBox::check_hwnd() {
        assert(get_class_name(m_hwnd) == L"ComboBox");
    }

    std::optional<int> ComboBox::find_by_data(int data) {
        for (int i = 0; i < ComboBox_GetCount(m_hwnd); ++i)
            if (ComboBox_GetItemData (m_hwnd, i) == data)
                return i;
        return std::nullopt;
    }
}
