#include "raii.h"
#include "Notepad_plus_msgs.h"

ToolbarIconsWrapper::ToolbarIconsWrapper() : m_icons{std::make_unique<ToolbarIcons>()} {
    m_icons->hToolbarBmp = nullptr;
    m_icons->hToolbarIcon = nullptr;
}

ToolbarIconsWrapper::~ToolbarIconsWrapper() {
    if (m_icons->hToolbarBmp)
        DeleteObject(m_icons->hToolbarBmp);

    if (m_icons->hToolbarIcon)
        DeleteObject(m_icons->hToolbarBmp);
}

ToolbarIconsWrapper::
ToolbarIconsWrapper(HINSTANCE h_inst, LPCWSTR name, UINT type, int cx, int cy, UINT fu_load) : ToolbarIconsWrapper() {
    m_icons->hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)h_inst, name, type, cx, cy, fu_load);
}

const ToolbarIcons* ToolbarIconsWrapper::get() {
    return m_icons.get();
}
