#pragma once
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <string>

class PersonSeg
{
public:
    // 构造时加载模型,只做一次
    explicit PersonSeg(const std::string& modelPath);

    // 输入一帧 BGR 图,返回同尺寸的 0/255 单通道掩膜(255 = 人物)
    cv::Mat segment(const cv::Mat& frame);

private:
    cv::dnn::Net net_;
};
