#include "Track.h"
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <string>

using namespace cv;     // .cpp 里可以用,头文件里不行
using std::vector;

//----------------------------- 追踪 -----------------------------
Track::Track(size_t maxTrail)
    : maxTrail_(std::max<size_t>(maxTrail,1)){}

vector<Point> Track::findMidPoint(const Mat&frame,const Scalar& lower,const Scalar& upper)
{
    Mat hsv, mask;
    cvtColor(frame, hsv, COLOR_BGR2HSV);
    inRange(hsv, lower, upper, mask);

    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
    morphologyEx(mask, mask, MORPH_OPEN, kernel);
    morphologyEx(mask, mask, MORPH_CLOSE, kernel);

    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    int maxIdx = -1;
    double maxArea = 1000.0;               // 同时作为最小面积阈值
    for (int i = 0; i < static_cast<int>(contours.size()); i++)
    {
        double area = contourArea(contours[i]);
        if (area > maxArea)
        {
            maxArea = area;
            maxIdx = i;
        }
    }
    if (maxIdx < 0) return {};

    Rect box = boundingRect(contours[maxIdx]);
    return { Point(box.x + box.width / 2, box.y + box.height / 2) };
}

void Track::update(const vector<Point>& centers)
{
    if(!centers.empty())
    {
        const Point& c = centers[0];
        if (!trail_.empty() && norm(c - trail_.back()) > 100)
        trail_.clear();               // 跳变太大,重置轨迹
        trail_.push_back(centers[0]);
    }
    else if(!trail_.empty())
    {
        // 目标丢失时,轨迹也逐帧缩短,慢慢消失
        trail_.pop_front();
    }
    
    while(trail_.size()>maxTrail_)
    {
        trail_.pop_front();
    }    
}

void Track::draw(Mat& img,const vector<Point>& centers)
{
    //1. 画轨迹:越旧越细、越暗
    for(int i=1;i<trail_.size();i++)
    {
        double ratio=static_cast<double>(i)/trail_.size();       //0~1  越新越大
        int thickness = std::max(1, static_cast<int>(ratio * 6)); 
        Scalar color(0, 255 * ratio, 255 * ratio);               // 越新越亮
        line(img, trail_[i - 1], trail_[i], color, thickness, LINE_AA);           
    }

    //2. 画当前中心点和坐标
    for (const Point& c : centers)
    {
        circle(img, c, 6, Scalar(0, 255, 0), FILLED);
        std::string label = "(" + std::to_string(c.x) + "," + std::to_string(c.y) + ")";
        putText(img, label, Point(c.x + 10, c.y),
                FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 2);
    }    
}

void Track::clearTrail(){trail_.clear();};
