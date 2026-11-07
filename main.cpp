//#define NOMINMAX            // 必须在 windows.h 之前
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "Track.h"
#include "SmoothFps.h"
#include "Invisible.h"
#include "Profiler.h"
#include "PersonSeg.h"
using namespace std;
using namespace cv;

//---------------------------- 模式 ----------------------------

enum class Mode { Tracking, InvisibleHSV, InvisibleAI };

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    VideoCapture capture(0);
    if (!capture.isOpened())
    {
        cerr << "摄像头打开失败" << endl;
        return -1;
    }

    Mat frame;
    if (!capture.read(frame) || frame.empty())
    {
        cerr << "当前帧读取失败" << endl;
        return -1;
    }

    //------------- HSV 滑动条 + 读取配置 -------------
    namedWindow("HSV Controls", WINDOW_NORMAL);
    createTrackbar("H min", "HSV Controls", nullptr, 179);
    createTrackbar("H max", "HSV Controls", nullptr, 179);
    createTrackbar("S min", "HSV Controls", nullptr, 255);
    createTrackbar("S max", "HSV Controls", nullptr, 255);
    createTrackbar("V min", "HSV Controls", nullptr, 255);
    createTrackbar("V max", "HSV Controls", nullptr, 255);

    int savedHMin = 125, savedHMax = 155;
    int savedSMin = 80,  savedSMax = 255;
    int savedVMin = 60,  savedVMax = 255;

    FileStorage fsRead("hsv_setting.yml", FileStorage::READ);
    if (fsRead.isOpened())
    {
        if (!fsRead["hMin"].empty() && !fsRead["hMax"].empty() &&
            !fsRead["sMin"].empty() && !fsRead["sMax"].empty() &&
            !fsRead["vMin"].empty() && !fsRead["vMax"].empty())
        {
            fsRead["hMin"] >> savedHMin;
            fsRead["hMax"] >> savedHMax;
            fsRead["sMin"] >> savedSMin;
            fsRead["sMax"] >> savedSMax;
            fsRead["vMin"] >> savedVMin;
            fsRead["vMax"] >> savedVMax;
            cout << "HSV 参数加载成功" << endl;
        }
        else
        {
            cout << "HSV 配置不完整,使用默认值" << endl;
        }
        fsRead.release();
    }
    else
    {
        cout << "未找到HSV配置,使用默认值" << endl;
    }

    setTrackbarPos("H min", "HSV Controls", savedHMin);
    setTrackbarPos("H max", "HSV Controls", savedHMax);
    setTrackbarPos("S min", "HSV Controls", savedSMin);
    setTrackbarPos("S max", "HSV Controls", savedSMax);
    setTrackbarPos("V min", "HSV Controls", savedVMin);
    setTrackbarPos("V max", "HSV Controls", savedVMax);

    //------------- 状态变量 -------------
    Track tracker;
    SmoothFps fpsCounter;
    PersonSeg seg("models/human_segmentation_pphumanseg_2023mar.onnx");
    VideoWriter writer;
    bool recording = false;
    Mat backImg;
    Mode mode = Mode::Tracking;

    Profiler prof;
    Profiler profMain;
    //------------- 主循环 -------------
    while (true)
    {
        profMain.begin();
        // 只在这里读一次摄像头
        capture >> frame;
        if (frame.empty()) break;
        profMain.lap("capture");

        // 读取滑动条,生成 HSV 范围(两种模式共用)
        Scalar lower(getTrackbarPos("H min", "HSV Controls"),
                     getTrackbarPos("S min", "HSV Controls"),
                     getTrackbarPos("V min", "HSV Controls"));
        Scalar upper(getTrackbarPos("H max", "HSV Controls"),
                     getTrackbarPos("S max", "HSV Controls"),
                     getTrackbarPos("V max", "HSV Controls"));

        // frame 保持干净,所有绘制都在 output 上
        Mat output = frame.clone();

    switch (mode)
    {
    case Mode::Tracking:
    {
        auto centers = tracker.findMidPoint(frame, lower, upper);
        tracker.update(centers);
        tracker.draw(output, centers);
        break;
    }
    case Mode::InvisibleHSV:
    case Mode::InvisibleAI:
    {
        if (backImg.empty())
        {
            cv::putText(output, "Press B to capture background", Point(20, 40),
                        FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
            break;
        }

        prof.begin();

        // 掩膜来源是唯一的区别
        Mat mask = (mode == Mode::InvisibleHSV)
                       ? colorMask(frame, lower, upper, prof)
                       : seg.segment(frame);
        if (mode == Mode::InvisibleAI) prof.lap("AI segment");

        output = blendWithMask(frame, backImg, mask, prof);
        prof.frameDone();
        break;
    }
    }
        profMain.lap("process"); 

        // 公共部分:FPS、模式提示、显示、录制
        fpsCounter.tick();
        char buf[32];
        snprintf(buf, sizeof(buf), "FPS : %.1f", fpsCounter.fps());
        cv::putText(output, buf, Point(10, 30),
                FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);

        const char* modeName = "";
        switch (mode)
        {
        case Mode::Tracking:     modeName = "Mode: Tracking";     break;
        case Mode::InvisibleHSV: modeName = "Mode: Invisible HSV"; break;
        case Mode::InvisibleAI:  modeName = "Mode: Invisible AI";  break;
        }
        cv::putText(output, modeName, Point(10, 60),
            FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 255), 2);

        cv::imshow("Result", output);
        profMain.lap("imshow");

        if (recording) writer.write(output);
        profMain.lap("write");

        //------------- 按键 -------------
        int key = waitKey(1);
        profMain.lap("waitKey");

        if (key == 27) break;

        if (key == 't' || key == 'T')
        {
            mode = Mode::Tracking;
            tracker.clearTrail();
        }
        else if (key == 'i' || key == 'I')
        {
            mode = Mode::InvisibleHSV;
        }
        else if (key == 'a' || key == 'A')
        {
            mode = Mode::InvisibleAI;
        }
        else if (key == 'b' || key == 'B')
        {
            backImg = frame.clone();
            cout << "背景采集完成" << endl;
        }
        else if (key == 's' || key == 'S')
        {
            FileStorage fsWrite("hsv_setting.yml", FileStorage::WRITE);
            if (fsWrite.isOpened())
            {
                fsWrite << "hMin" << getTrackbarPos("H min", "HSV Controls");
                fsWrite << "hMax" << getTrackbarPos("H max", "HSV Controls");
                fsWrite << "sMin" << getTrackbarPos("S min", "HSV Controls");
                fsWrite << "sMax" << getTrackbarPos("S max", "HSV Controls");
                fsWrite << "vMin" << getTrackbarPos("V min", "HSV Controls");
                fsWrite << "vMax" << getTrackbarPos("V max", "HSV Controls");
                fsWrite.release();
                cout << "HSV 参数保存成功" << endl;
            }
            else
            {
                cerr << "HSV 参数保存失败" << endl;
            }
        }
        else if (key == 'r' || key == 'R')
        {
            if (!recording)
            {
                int fourcc = VideoWriter::fourcc('M', 'J', 'P', 'G');
                writer.open("output.avi", fourcc, 20.0, frame.size());
                if (writer.isOpened())
                {
                    recording = true;
                    cout << "开始录制,再按R停止" << endl;
                }
                else
                {
                    cerr << "视频文件打开失败" << endl;
                }
            }
            else
            {
                writer.release();
                recording = false;
                cout << "录制已停止" << endl;
            }
        }
        profMain.frameDone();
    }

    writer.release();
    capture.release();
    destroyAllWindows();
    return 0;
}

