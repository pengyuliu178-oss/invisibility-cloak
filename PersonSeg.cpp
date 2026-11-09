#include "PersonSeg.h"
#include <opencv2/imgproc.hpp>

using namespace cv;

PersonSeg::PersonSeg(const std::string& modelPath)
{
    net_ = dnn::readNetFromONNX(modelPath);
    net_.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
    net_.setPreferableTarget(dnn::DNN_TARGET_CPU);
}

Mat PersonSeg::segment(const Mat& frame)
{
    // 预处理:缩放到 192×192,BGR→RGB,(x - 127.5) / 127.5
    Mat blob = dnn::blobFromImage(frame, 1.0 / 127.5, Size(192, 192),
                                  Scalar(127.5, 127.5, 127.5),
                                  /*swapRB=*/true, /*crop=*/false);
    net_.setInput(blob);
    Mat out = net_.forward();                    // 1 × 2 × 192 × 192

    // 后处理:人物通道减背景通道,放大回原图尺寸,大于 0 即为人物
    int H = out.size[2], W = out.size[3];
    Mat bg(H, W, CV_32F, out.ptr<float>(0, 0));
    Mat person(H, W, CV_32F, out.ptr<float>(0, 1));
    Mat diff = person - bg;
    if (prevDiff_.empty()) prevDiff_ = diff.clone();
    else addWeighted(prevDiff_, smooth_, diff, 1.0 - smooth_, 0, prevDiff_);

    Mat big;
    resize(prevDiff_, big, frame.size(), 0, 0, INTER_LINEAR);
    return big > 0;                             // CV_8U,0 或 255
}
