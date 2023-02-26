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

HRESULT SaveIcon(HICON hIcon, const wchar_t *path) {
  // Create the IPicture intrface
  PICTDESC desc = {sizeof(PICTDESC)};
  desc.picType = PICTYPE_ICON;
  desc.icon.hicon = hIcon;
  IPicture *pPicture = 0;
  HRESULT hr = OleCreatePictureIndirect(&desc, IID_IPicture, FALSE, (void **)&pPicture);
  if (FAILED(hr))
    return hr;

  // Create a stream and save the image
  IStream *pStream = 0;
  CreateStreamOnHGlobal(0, TRUE, &pStream);
  LONG cbSize = 0;
  hr = pPicture->SaveAsFile(pStream, TRUE, &cbSize);

  // Write the stream content to the file
  if (!FAILED(hr)) {
    HGLOBAL hBuf = 0;
    GetHGlobalFromStream(pStream, &hBuf);
    void *buffer = GlobalLock(hBuf);
    HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (!hFile)
      hr = HRESULT_FROM_WIN32(GetLastError());
    else {
      DWORD written = 0;
      WriteFile(hFile, buffer, cbSize, &written, 0);
      CloseHandle(hFile);
    }
    GlobalUnlock(buffer);
  }
  // Cleanup
  pStream->Release();
  pPicture->Release();
  return hr;
}

ToolbarIconsWrapper::ToolbarIconsWrapper(HINSTANCE h_inst, LPCWSTR normal_name, LPCWSTR dark_mode_name, LPCWSTR bmp_name)
  : ToolbarIconsWrapper() {
  m_icons->hToolbarBmp = ::LoadBitmap(h_inst, bmp_name);
  m_icons->hToolbarIcon = ::LoadIcon(h_inst, normal_name);
  m_icons->hToolbarIconDarkMode = ::LoadIcon(h_inst, dark_mode_name);
  SaveIcon(m_icons->hToolbarIconDarkMode, L"c:\\tmp\\1.ico");
}

const toolbarIconsWithDarkMode *ToolbarIconsWrapper::get() {
  return m_icons.get();
}
