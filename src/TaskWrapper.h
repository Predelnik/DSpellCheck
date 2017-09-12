#pragma once

#include "CommonFunctions.h"

struct TaskWrapper {
private:
    using AliveStatusType = std::shared_ptr<concurrency::cancellation_token_source>;

public:
    explicit TaskWrapper(HWND targetHwnd);
    ~TaskWrapper ();
    void resetAliveStatus ();
    void cancel ();
    template <typename ActionType, typename GuiCallbackType>
     void doDeferred (ActionType action, GuiCallbackType guiCallback) {
     resetAliveStatus ();
    concurrency::create_task(
        [
            action = std::move (action),
            guiCallback = std::move(guiCallback),
            as = weaken (m_aliveStatus),
            ctoken = m_aliveStatus->get_token(),
            hwnd = m_targetHwnd
        ]()
    {
        auto ret = action (ctoken);
        if (as.expired() || ctoken.is_canceled())
            return;

        auto cbData = std::make_unique<CallbackData>();
        cbData->callback = [guiCallback = std::move (guiCallback), ret = std::move (ret)](){ guiCallback (ret); };
        cbData->aliveStatus = as;
         PostMessage(hwnd,
            GetCustomGUIMessageId(CustomGUIMessage::GENERIC_CALLBACK), reinterpret_cast<WPARAM> (cbData.release ())
            , 0);
    });
}

private:
    HWND m_targetHwnd;
    AliveStatusType m_aliveStatus;
};
