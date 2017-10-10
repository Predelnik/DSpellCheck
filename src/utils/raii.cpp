#include "raii.h"
#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"

ToolbarIconsWrapper::ToolbarIconsWrapper() : m_icons{std::make_unique<toolbarIcons>()} {
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
ToolbarIconsWrapper(HINSTANCE hInst, LPCWSTR name, UINT type, int cx, int cy, UINT fuLoad) : ToolbarIconsWrapper() {
    m_icons->hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)hInst, name, type, cx, cy, fuLoad);
}

const toolbarIcons* ToolbarIconsWrapper::get() {
    return m_icons.get();
}
