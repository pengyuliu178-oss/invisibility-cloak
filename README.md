# 紫色布隐身斗篷 + 目标追踪(OpenCV / C++)

用摄像头实时识别紫色区域,替换成事先采集的背景,实现"隐身"效果。同时支持追踪目标中心并显示运动轨迹。

<!--![demo](docs/demo.gif)-->

## 功能

- **隐身模式**:HSV 颜色分割、形态学去噪、面积筛选、掩膜羽化,再用 8 位整数做透明度混合
- **追踪模式**:检测最大的目标区域,标出中心点和坐标,绘制最近 40 帧的渐变轨迹
- **HSV 实时调参**:六个滑动条,按 `S` 保存,下次启动自动读取
- **录制**:把当前画面录成 AVI(MJPG)
- **性能统计**:自带分步计时器,每 100 帧输出一次平均耗时

## 环境要求

- Windows 10/11 x64
- Visual Studio 2022(MSVC 工具集)
- CMake 3.16 及以上
- OpenCV 4.12.0 预编译包

## 构建与运行

```powershell
cmake -S . -B build-cmake -DOpenCV_DIR=D:/work/opencv-4.12.0/opencv/build
cmake --build build-cmake --config Release
.\build-cmake\Release\invisibility_cloak.exe
```

运行时需要能找到 OpenCV 的 DLL:把 `opencv/build/x64/vc16/bin` 加入 `PATH`,或把 DLL 复制到 exe 同目录。

在 VS Code 中使用 CMake Tools 扩展时,把 `-DOpenCV_DIR=...` 写进 `.vscode/settings.json` 的 `cmake.configureArgs` 即可。

## 使用方法

1. 启动后默认是**追踪模式**,拖动 `HSV Controls` 窗口的滑动条,使目标被正确识别
2. 按 `S` 保存 HSV 参数
3. 人离开画面,按 `B` 采集背景
4. 按 `I` 切换到隐身模式,按 `T` 切回追踪模式

| 按键 | 功能 |
|---|---|
| `T` | 追踪模式 |
| `I` | 隐身模式(需先按 `B` 采集背景) |
| `B` | 采集当前画面作为背景 |
| `S` | 保存 HSV 参数到 `hsv_setting.yml` |
| `R` | 开始 / 停止录制,输出 `output.avi` |
| `Esc` | 退出 |

## 项目结构

| 文件 | 作用 |
|---|---|
| `main.cpp` | 主循环、模式切换、按键处理 |
| `Track.h / .cpp` | 目标检测、中心点与轨迹绘制 |
| `Invisible.h / .cpp` | 隐身效果的掩膜生成与图像混合 |
| `SmoothFps.h` | 滑动窗口平均的帧率计算 |
| `Profiler.h` | 分步耗时统计 |
| `CMakeLists.txt` | 构建配置 |

## 原理概述

```
摄像头帧 → BGR 转 HSV → inRange 提取颜色 → 开闭运算去噪
        → 轮廓面积筛选 → 高斯模糊羽化 → 整数 α 混合(背景 × α + 当前帧 × (1−α))
```

## 性能分析

在 Release 模式、640×480 下,用 `Profiler` 统计每帧平均耗时。

**混合步骤优化**:原来的浮点混合每帧要创建多张 `float` 大图,瓶颈是内存带宽。改成 8 位整数运算(`multiply` + `add`)后,数据量缩小为 1/4,并去掉多余的类型转换。

| 版本 | `makeInvisible` (ms/帧) | 改动 |
|---|---|---|
| 基线 | 10.53 | 浮点混合(`convertTo` + `mul`) |
| v1 | 2.50 | 8 位整数混合 |

优化后各步骤耗时(ms/帧):

| cvtColor | inRange | morphology | findContours | GaussianBlur | intBlend |
|---|---|---|---|---|---|
| 0.30 | 0.34 | 0.26 | 0.15 | 0.17 | 1.28 |

**帧率结论**:主循环每帧约 33 ms,其中处理只占约 3.7 ms,其余是等待摄像头出帧。帧率被摄像头的 30 fps 限制,继续优化算法不会提高 FPS,但能降低 CPU 占用,为后续模型推理留出时间预算。

## 后续计划

- 用 AI 人物分割(OpenCV DNN / ONNX Runtime)替代 HSV 掩膜,不再依赖紫色布料
- 实现背景图片替换
- 按需用 Qt 增加交互界面

## 迁移记录

从 qmake / Qt Creator 迁移到 CMake / VS Code 的过程见 [docs/MIGRATION.md](docs/MIGRATION.md)。