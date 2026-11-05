#pragma once
#include <opencv2/core.hpp>
#include <deque>
#include <vector>

class Track
{
public:
    explicit Track(size_t maxTrail=40);

    std::vector<cv::Point>findMidPoint(const cv::Mat& frame,
                                       const cv::Scalar& lower,
                                       const cv::Scalar& upper);
    void update(const std::vector<cv::Point>& centers);
    void draw(cv::Mat& img,const std::vector<cv::Point>& centers);
    void clearTrail();

private:    
    size_t maxTrail_;
    std::deque<cv::Point> trail_;
};
