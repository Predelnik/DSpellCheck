#pragma once

struct ProgressData {
public:
    void set (int progress, const std::wstring &status, bool marquee = false);
    bool getMarquee() const;
    int getProgress() const;
    std::wstring getStatus () const;

private:
    int m_progress = 0;
    std::wstring m_status;
    mutable std::mutex m_mutex;
    bool m_marquee = false;
};
