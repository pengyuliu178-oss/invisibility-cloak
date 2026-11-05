#pragma once
#include <opencv2/core.hpp>
#include "Profiler.h"

cv::Mat makeInvisible(const cv::Mat& frame, const cv::Mat& backImg,
                      const cv::Scalar& lower, const cv::Scalar& upper,Profiler& prof);
