#include "ProgressData.h"

void ProgressData::set(int progress, const std::wstring& status, bool marquee) {
    std::lock_guard<std::mutex> lg (m_mutex);
    m_progress = progress;
    m_status = status;
    m_marquee = marquee;
}

bool ProgressData::getMarquee() const {
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_marquee;
}

int ProgressData::getProgress() const {
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_progress;
}

std::wstring ProgressData::getStatus()const{
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_status;
}
