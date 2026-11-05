#pragma once
#include <opencv2/core.hpp>
#include <cstdio>
#include <string>
#include <vector>

class Profiler
{
public:
    void begin() { last_ = cv::getTickCount(); }

    void lap(const std::string& name)
    {
        int64 now = cv::getTickCount();
        double ms = (now - last_) * 1000.0 / cv::getTickFrequency();
        last_ = now;

        for (auto& e : items_)
        {
            if (e.name == name) { e.sum += ms; return; }
        }
        items_.push_back({ name, ms });
    }

    void frameDone()
    {
        if (++frames_ < 100) return;

        std::printf("---- 平均耗时(ms/帧) ----\n");
        double total = 0;
        for (auto& e : items_)
        {
            double avg = e.sum / frames_;
            std::printf("%-16s %6.2f\n", e.name.c_str(), avg);
            total += avg;
            e.sum = 0;
        }
        std::printf("%-16s %6.2f\n\n", "合计", total);
        frames_ = 0;
    }

private:
    struct Item { std::string name; double sum; };
    std::vector<Item> items_;
    int frames_ = 0;
    int64 last_ = 0;
};