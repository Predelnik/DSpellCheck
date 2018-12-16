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

#include "TaskWrapper.h"

TaskWrapper::TaskWrapper(HWND target_hwnd): m_target_hwnd(target_hwnd), m_valid(true)  {
}

TaskWrapper::~TaskWrapper() {
  if (m_valid.get())
    cancel ();
}

void TaskWrapper::reset_alive_status() {
    m_alive_status = {new concurrency::cancellation_token_source (), [](
        concurrency::cancellation_token_source *source){ source->cancel(); delete source; }};
}

void TaskWrapper::cancel() {
    if (m_alive_status)
        {
            m_alive_status->cancel();
            m_alive_status.reset ();
        }
}


