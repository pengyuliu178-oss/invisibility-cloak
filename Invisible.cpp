#include "Invisible.h"
#include "Profiler.h"
#include <opencv2/imgproc.hpp>
using namespace cv;
using std::vector;

/*
 * 功能:隐身效果。把画面中"目标颜色"的区域替换成事先拍好的背景。
 * 参数:
 *   frame   当前摄像头帧(BGR,只读)
 *   backImg 事先采集的背景图(尺寸、类型必须和 frame 一致)
 *   lower   HSV 下限 (H,S,V)
 *   upper   HSV 上限 (H,S,V)
 *   prof    计时器
 * 返回:合成后的画面(8 位 BGR)
 */

/*----------得到掩膜----------*/



cv::Mat colorMask(const cv::Mat& frame, const cv::Scalar& lower,
                  const cv::Scalar& upper, Profiler& prof)
{
    prof.begin();

    // ================= 第一步:生成掩膜 =================
    Mat hsv, mask;
    cvtColor(frame, hsv, COLOR_BGR2HSV);                                prof.lap("1 cvtColor");
    inRange(hsv, lower, upper, mask);                                   prof.lap("1 inRange");

    // ================= 第二步:形态学去噪 =================
    Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
    morphologyEx(mask, mask, MORPH_OPEN, kernel);    // 开运算:去小白点
    morphologyEx(mask, mask, MORPH_CLOSE, kernel);   // 闭运算:填小黑洞    
                                                                        prof.lap("1 morphology");

    // ================= 第三步:按面积筛选 =================
    vector<vector<Point>> contours;
    findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);   prof.lap("1 findContours");
    Mat cleanMask = Mat::zeros(mask.size(), mask.type());
    for (int i = 0; i < static_cast<int>(contours.size()); i++)
    {
        if (contourArea(contours[i]) >= 1000.0)
            drawContours(cleanMask, contours, i, Scalar(255), FILLED);
    }

    // ================= 第四步:羽化 =================
    // 高斯模糊让掩膜边缘出现 0~255 的灰色过渡,合成时边缘更自然
    Mat softMask;
    GaussianBlur(cleanMask, softMask, Size(9, 9), 0);                   prof.lap("5 GaussianBlur");
    
    return cleanMask;

    /*-----------------------------------以下替换成blendWithMask函数实现-------------------------------
    // ================= 第五步:透明度融合(8 位整数版) =================
    // 思路:全程保持 8 位,不再转 float。
    // 公式仍是:结果 = 背景 × α + 当前帧 × (1 − α),
    // 只是 α 不再是 0.0~1.0,而是直接用 0~255,
    // 相乘后每项再乘 1/255 缩回 0~255 范围。

    // cvtColor(..., COLOR_GRAY2BGR):单通道灰度 → 三通道(三个通道内容相同)。
    // 作用等同于原来的 merge,但对 8 位图更快。alpha3 取值 0~255。
    Mat alpha3;
    cvtColor(softMask, alpha3, COLOR_GRAY2BGR);

    // bitwise_not(输入, 输出):按位取反。对 8 位图等价于 255 − 值,
    // 即得到 (1 − α) 对应的权重 inv。
    Mat inv;
    bitwise_not(alpha3, inv);

    // multiply(src1, src2, dst, scale):逐元素相乘,再乘 scale,结果自动饱和回 8 位。
    // scale = 1/255:把 "值 × 值" 的 0~65025 缩回 0~255。
    Mat partBack, partFront, result;
    multiply(backImg, alpha3, partBack,  1.0 / 255.0);   // 背景 × α
    multiply(frame,   inv,    partFront, 1.0 / 255.0);   // 当前帧 × (1 − α)

    // add(a, b, dst):逐元素相加,8 位图自动饱和(超过 255 取 255)。
    add(partBack, partFront, result);                                   prof.lap("6 intBlend");
    return result;

    /* ======================= 旧版:浮点混合(已停用) =======================
    Mat alpha;
    softMask.convertTo(alpha, CV_32F, 1.0 / 255.0);   // 0~255 → 0.0~1.0
    Mat alpha3;
    merge(vector<Mat>(3, alpha), alpha3);             // 单通道 → 三通道   prof.lap("6 alpha");

    Mat frameF, backF;
    frame.convertTo(frameF, CV_32F);                  // 8 位 → 32 位浮点
    backImg.convertTo(backF, CV_32F);                                     prof.lap("7 toFloat");

    // 逐元素相乘:背景 × α + 当前帧 × (1 − α)
    Mat finalF = backF.mul(alpha3) + frameF.mul(Scalar::all(1.0) - alpha3);
                                                                          prof.lap("8 blend");
    Mat result;
    finalF.convertTo(result, CV_8U);                  // 转回 8 位(自动饱和) prof.lap("9 toU8");
    return result;
    ======================================================================= */
    
    //-------------------------------------------------------------------------*/
}

Mat blendWithMask(const Mat& frame, const Mat& backImg,
                  const Mat& mask, Profiler& prof)
{
    // 羽化:高斯模糊让边缘出现灰色过渡
    Mat softMask;
    GaussianBlur(mask, softMask, Size(9, 9), 0);        prof.lap("GaussianBlur");

    // 8 位整数混合:结果 = 背景 × α + 当前帧 × (1 − α)
    Mat alpha3, inv, partBack, partFront, result;
    cvtColor(softMask, alpha3, COLOR_GRAY2BGR);
    bitwise_not(alpha3, inv);
    multiply(backImg, alpha3, partBack,  1.0 / 255.0);
    multiply(frame,   inv,    partFront, 1.0 / 255.0);
    add(partBack, partFront, result);                   prof.lap("intBlend");
    return result;
}
