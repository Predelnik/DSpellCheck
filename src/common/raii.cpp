// This file is part of DSpellCheck Plug-in for Notepad++
// Copyright (C)2019 Sergey Semushin <Predelnik@gmail.com>
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

#include <OleCtl.h>

ToolbarIconsWrapper::ToolbarIconsWrapper() : m_icons{std::make_unique<toolbarIconsWithDarkMode>()} {
  m_icons->hToolbarBmp = nullptr;
  m_icons->hToolbarIcon = nullptr;
  m_icons->hToolbarIconDarkMode = nullptr;
}

ToolbarIconsWrapper::~ToolbarIconsWrapper() {
  if (m_icons->hToolbarBmp != nullptr)
    DeleteObject(m_icons->hToolbarBmp);

  if (m_icons->hToolbarIcon != nullptr)
    DeleteObject(m_icons->hToolbarIcon);

  if (m_icons->hToolbarIconDarkMode != nullptr)
    DeleteObject(m_icons->hToolbarIconDarkMode);
}

ToolbarIconsWrapper::ToolbarIconsWrapper(HINSTANCE h_inst, LPCWSTR normal_name, LPCWSTR dark_mode_name, LPCWSTR bmp_name)
  : ToolbarIconsWrapper() {
  m_icons->hToolbarBmp = ::LoadBitmap(h_inst, bmp_name);
  m_icons->hToolbarIcon = ::LoadIcon(h_inst, normal_name);
  m_icons->hToolbarIconDarkMode = ::LoadIcon(h_inst, dark_mode_name);
}

const toolbarIconsWithDarkMode *ToolbarIconsWrapper::get() {
  return m_icons.get();
}
