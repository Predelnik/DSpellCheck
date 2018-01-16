#pragma once

#include "CommonFunctions.h"
#include "MainDef.h"

struct CallbackData {
  std::weak_ptr<void> alive_status;
  std::function<void ()> callback;
};

struct TaskWrapper {
private:
    using AliveStatusType = std::shared_ptr<concurrency::cancellation_token_source>;

public:
    explicit TaskWrapper(HWND target_hwnd);
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
        cb_data->callback = [guiCallback = std::move (guiCallback), ret = std::move (ret)](){ guiCallback (ret); };
        cb_data->alive_status = as;
         PostMessage(hwnd,
            get_custom_gui_message_id(CustomGuiMessage::generic_callback), reinterpret_cast<WPARAM> (cb_data.release ())
            , 0);
    });
}

private:
    HWND m_target_hwnd;
    AliveStatusType m_alive_status;
};
