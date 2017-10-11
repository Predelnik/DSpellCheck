#include "TaskWrapper.h"

TaskWrapper::TaskWrapper(HWND target_hwnd): m_target_hwnd(target_hwnd)  {
}

TaskWrapper::~TaskWrapper() {
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


