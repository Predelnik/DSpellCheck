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

#pragma once

#include "CommonFunctions.h"
#include "MainDefs.h"
#include <ppltasks.h>
#include "utils/move_only.h"

struct CallbackData {
  std::weak_ptr<void> alive_status;
  std::function<void ()> callback;
};

struct TaskWrapper {
private:
    using AliveStatusType = std::shared_ptr<concurrency::cancellation_token_source>;
    using Self = TaskWrapper;

public:
    explicit TaskWrapper(HWND target_hwnd);
    TaskWrapper(Self &&) = default;
    Self &operator=(Self &&) = default;
    ~TaskWrapper ();
    void reset_alive_status ();
    void cancel ();
    template <typename ActionType, typename GuiCallbackType>
     void do_deferred (ActionType action, GuiCallbackType gui_callback) {
     reset_alive_status ();
    concurrency::create_task(
        [
            action = std::move (action),
            guiCallback = std::move(gui_callback),
            as = weaken (m_alive_status),
            ctoken = m_alive_status->get_token(),
            hwnd = m_target_hwnd
        ]()
    {
        auto ret = action (ctoken);
        if (as.expired() || ctoken.is_canceled())
            return;

        auto cb_data = std::make_unique<CallbackData>();
        cb_data->callback = [guiCallback = std::move (guiCallback), ret = std::move (ret)]()mutable{ guiCallback (std::move (ret)); };
        cb_data->alive_status = as;
         PostMessage(hwnd,
            get_custom_gui_message_id(CustomGuiMessage::generic_callback), reinterpret_cast<WPARAM> (cb_data.release ())
            , 0);
    });
}

private:
    HWND m_target_hwnd;
    AliveStatusType m_alive_status;
    move_only<bool> m_valid;
};
