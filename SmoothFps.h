#pragma once
#include <algorithm>
#include <chrono>
#include <deque>

class SmoothFps
{
public:
    explicit SmoothFps(size_t window = 60) : window_(std::max<size_t>(window, 1)) {}

    void tick()
    {
        auto now = std::chrono::steady_clock::now();
        if (haslast_)
        {
            double dt = std::chrono::duration<double>(now - last_).count();
            intervals_.push_back(dt);
            sum_ += dt;
            if (intervals_.size() > window_)
            {
                sum_ -= intervals_.front();
                intervals_.pop_front();
            }
        }
        last_ = now;
        haslast_ = true;
    }

    double fps() const
    {
        if (intervals_.empty() || sum_ <= 0.0) return 0.0;
        return static_cast<double>(intervals_.size()) / sum_;
    }

private:
    size_t window_;
    std::deque<double> intervals_;
    double sum_ = 0.0;
    std::chrono::steady_clock::time_point last_;
    bool haslast_ = false;
};
