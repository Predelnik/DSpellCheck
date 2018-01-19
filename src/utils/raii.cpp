#include "raii.h"
#include "Notepad_plus_msgs.h"

ToolbarIconsWrapper::ToolbarIconsWrapper() : m_icons{std::make_unique<ToolbarIcons>()} {
    m_icons->hToolbarBmp = nullptr;
    m_icons->hToolbarIcon = nullptr;
}

ToolbarIconsWrapper::~ToolbarIconsWrapper() {
    if (m_icons->hToolbarBmp != nullptr)
        DeleteObject(m_icons->hToolbarBmp);

    if (m_icons->hToolbarIcon != nullptr)
        DeleteObject(m_icons->hToolbarBmp);
}

ToolbarIconsWrapper::
ToolbarIconsWrapper(HINSTANCE h_inst, LPCWSTR name, UINT type, int cx, int cy, UINT fu_load) : ToolbarIconsWrapper() {
    m_icons->hToolbarIcon = static_cast<HICON>(::LoadImage(h_inst, name, type, cx, cy, fu_load));
    ICONINFO iconinfo;
    GetIconInfo(m_icons->hToolbarIcon, &iconinfo);
    m_icons->hToolbarBmp = iconinfo.hbmColor;
}

const ToolbarIcons* ToolbarIconsWrapper::get() {
    return m_icons.get();
}
