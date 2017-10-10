#include "ProgressData.h"

void ProgressData::set(int progress, const std::wstring& status, bool marquee) {
    std::lock_guard<std::mutex> lg (m_mutex);
    m_progress = progress;
    m_status = status;
    m_marquee = marquee;
}

bool ProgressData::get_marquee() const {
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_marquee;
}

int ProgressData::get_progress() const {
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_progress;
}

std::wstring ProgressData::get_status()const{
    std::lock_guard<std::mutex> lg (m_mutex);
    return m_status;
}
