#define NOMINMAX
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <windows.h>
#include "Profiler.h"
using namespace std;
using namespace cv;

static Mat blendWithMask(const Mat& frame, const Mat& back, const Mat& mask)
{
    Mat soft;
    GaussianBlur(mask, soft, Size(9, 9), 0);

    Mat alpha3, inv, partBack, partFront, result;
    cvtColor(soft, alpha3, COLOR_GRAY2BGR);
    bitwise_not(alpha3, inv);
    multiply(back,  alpha3, partBack,  1.0 / 255.0);
    multiply(frame, inv,    partFront, 1.0 / 255.0);
    add(partBack, partFront, result);
    return result;
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    
    dnn::Net net = dnn::readNetFromONNX("models/human_segmentation_pphumanseg_2023mar.onnx");
    net.setPreferableBackend(dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(dnn::DNN_TARGET_CPU);

    VideoCapture capture(0);
    if(!capture.isOpened())
    {
        cerr<<"摄像头读取失败"<<endl;
        return -1;
    }

    Mat frame,backImg;
    Profiler prof;

    if(!capture.read(frame)||frame.empty())
    {
        cerr<<"当前帧读取失败"<<endl;
        return -1;
    }



    while(true)
    {
        capture>>frame;
        if(frame.empty())break;  

        if (frame.empty()) { std::cerr << "图片读取失败" << std::endl; return -1; }
        prof.begin();
        
        /*  // 缩小到最长边 960,保持宽高比
        double scale = 960.0 / std::max(frame.cols, frame.rows);
        if (scale < 1.0)
        resize(frame, frame, Size(), scale, scale, INTER_AREA);*/

        // 预处理:缩放到 192×192,BGR→RGB,(x - 127.5) / 127.5
        Mat blob = dnn::blobFromImage(frame, 1.0 / 127.5, Size(192, 192),
                                      Scalar(127.5, 127.5, 127.5),
                                    /*swapRB=*/true, /*crop=*/false);
        net.setInput(blob);
        Mat out = net.forward();                       // 1 × 2 × 192 × 192
        prof.lap("infer");

        // 后处理:人物通道减背景通道,放大回原图,大于 0 即为人物
        int H = out.size[2], W = out.size[3];
        Mat bg(H, W, CV_32F, out.ptr<float>(0, 0));
        Mat person(H, W, CV_32F, out.ptr<float>(0, 1));
        Mat diff = person - bg;
        resize(diff, diff, frame.size(), 0, 0, INTER_LINEAR);
        Mat mask = diff > 0;                           // CV_8U,值为 0 或 255
        prof.lap("post");

        // 叠加显示:人物区域染成半透明绿色
        Mat vis = frame.clone();
        Mat green(frame.size(), frame.type(), Scalar(0, 255, 0));
        Mat tinted;
        addWeighted(frame, 0.5, green, 0.5, 0, tinted);
        tinted.copyTo(vis, mask);
        imshow("overlay", vis);

        if (!backImg.empty())
            imshow("invisible", blendWithMask(frame, backImg, mask));
        prof.lap("show");

        prof.frameDone();

        int key = waitKey(1);
        if (key == 27) break;
        if (key == 'b' || key == 'B')
        {
            backImg = frame.clone();
            cout << "背景采集完成" << endl;
        }
    }

    return 0;
}

