// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2018 Sergey Semushin <Predelnik@gmail.com>
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

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
