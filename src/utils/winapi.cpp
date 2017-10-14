#include "winapi.h"
#include "Aclapi.h"

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
    ACL g_null_acl = { 0 };
    InitializeAcl(&g_null_acl, sizeof(g_null_acl), ACL_REVISION);
    SetNamedSecurityInfo(const_cast<wchar_t *>(to), SE_FILE_OBJECT,
                         DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION,
                         nullptr, nullptr, static_cast<PACL>(&g_null_acl), nullptr);
    return ret;
}
