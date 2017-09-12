#include "TaskWrapper.h"

TaskWrapper::TaskWrapper(HWND targetHwnd): m_targetHwnd(targetHwnd)  {
}

TaskWrapper::~TaskWrapper() {
    cancel ();
}

void TaskWrapper::resetAliveStatus() {
    m_aliveStatus = {new concurrency::cancellation_token_source (), [](
        concurrency::cancellation_token_source *source){ source->cancel(); delete source; }};
}

void TaskWrapper::cancel() {
    if (m_aliveStatus)
        {
            m_aliveStatus->cancel();
            m_aliveStatus.reset ();
        }
}


