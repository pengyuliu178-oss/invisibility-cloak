#pragma once
#include <opencv2/core.hpp>
#include "Profiler.h"
cv::Mat blendWithMask(const cv::Mat& frame, const cv::Mat& backImg,
                      const cv::Mat& mask, Profiler& prof);


cv::Mat colorMask(const cv::Mat& frame, const cv::Scalar& lower,
                  const cv::Scalar& upper, Profiler& prof);        


