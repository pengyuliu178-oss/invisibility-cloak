# 隐身斗篷 + 目标追踪(OpenCV / C++)

用摄像头实时把画面中的目标区域替换成事先采集的背景,实现"隐身"效果。支持两种方式找到目标:**紫色布料(HSV 颜色分割)** 和 **AI 人物分割**。同时支持目标中心追踪与运动轨迹。

<!-- ![demo](docs/demo.gif) -->

## 功能

- **HSV 隐身**:颜色分割、形态学去噪、面积筛选,得到目标掩膜
- **AI 隐身**:用 PP-HumanSeg 模型分割人物,不需要紫色布料
- **统一的合成步骤**:两种掩膜共用同一个羽化 + 8 位整数透明度混合
- **追踪模式**:检测最大目标区域,显示中心点坐标和最近 40 帧的渐变轨迹
- **HSV 实时调参**:六个滑动条,按 `S` 保存,下次启动自动读取
- **录制**:把当前画面录成 AVI(MJPG)
- **性能统计**:自带分步计时器,每 100 帧输出一次平均耗时

## 环境要求

- Windows 10/11 x64
- Visual Studio 2022(MSVC 工具集)
- CMake 3.16 及以上
- OpenCV 4.12.0 预编译包

## 模型下载

AI 模式需要 PP-HumanSeg 的 ONNX 模型,来自 [opencv_zoo](https://github.com/opencv/opencv_zoo) 的 `models/human_segmentation_pphumanseg` 目录:

1. 下载 `human_segmentation_pphumanseg_2023mar.onnx`(用页面上的下载按钮,文件应有几 MB;只有几 KB 说明下到的是 Git LFS 指针文件)
2. 在项目根目录新建 `models/` 文件夹,把文件放进去

模型文件不包含在本仓库中,使用前请查看 opencv_zoo 里对应的许可证。

## 构建与运行

```powershell
cmake -S . -B build-cmake -DOpenCV_DIR=你的OpenCV路径/build
cmake --build build-cmake --config Release
```

**请在项目根目录运行**,程序用相对路径读取 `models/` 和 `hsv_setting.yml`:

```powershell
.\build-cmake\Release\invisibility_cloak.exe
```

运行时需要能找到 OpenCV 的 DLL:把 `opencv/build/x64/vc16/bin` 加入 `PATH`,或把 DLL 复制到 exe 同目录。

使用 VS Code 的 CMake Tools 扩展时,把 `-DOpenCV_DIR=...` 写进 `.vscode/settings.json` 的 `cmake.configureArgs` 即可。

## 使用方法

1. 启动后默认是**追踪模式**
2. 人离开画面,按 `B` 采集背景
3. 按 `A` 进入 **AI 隐身**,人物会被替换成背景
4. 想用紫色布料:按 `I` 进入 **HSV 隐身**,先拖动 `HSV Controls` 里的滑动条,使布料被正确识别,按 `S` 保存参数

| 按键 | 功能 |
|---|---|
| `T` | 追踪模式 |
| `I` | HSV 隐身(需先按 `B` 采集背景) |
| `A` | AI 隐身(需先按 `B` 采集背景) |
| `B` | 采集当前画面作为背景 |
| `S` | 保存 HSV 参数到 `hsv_setting.yml` |
| `R` | 开始 / 停止录制,输出 `output.avi` |
| `Esc` | 退出 |

## 项目结构

| 文件 | 作用 |
|---|---|
| `main.cpp` | 主循环、模式切换、按键处理 |
| `Track.h / .cpp` | 目标检测、中心点与轨迹绘制 |
| `Invisible.h / .cpp` | `colorMask`(HSV 掩膜)与 `blendWithMask`(羽化 + 整数混合) |
| `PersonSeg.h / .cpp` | AI 人物分割:加载 ONNX 模型,输入一帧,输出掩膜 |
| `SmoothFps.h` | 滑动窗口平均的帧率计算 |
| `Profiler.h` | 分步耗时统计 |
| `seg_test.cpp` | 分割模型的独立验证程序 |
| `CMakeLists.txt` | 构建配置 |

## 原理概述

```
HSV 路径:  摄像头帧 → BGR 转 HSV → inRange → 开闭运算 → 轮廓面积筛选 ─┐
                                                                        ├→ 掩膜 → 高斯羽化 → 整数 α 混合
AI 路径:   摄像头帧 → 缩放 192×192 → PP-HumanSeg 推理 → 通道相减 → 放大 ─┘
                      混合公式:结果 = 背景 × α + 当前帧 × (1 − α)
```

## 性能分析

在 Release 模式、640×480、CPU 上,用 `Profiler` 统计每帧平均耗时。

**1. 混合步骤优化**:原来的浮点混合每帧要创建多张 `float` 大图,瓶颈是内存带宽。改成 8 位整数运算(`multiply` + `add`)后,数据量缩小为 1/4,并去掉多余的类型转换。

| 版本 | HSV 隐身 `makeInvisible` (ms/帧) | 改动 |
|---|---|---|
| 基线 | 10.53 | 浮点混合(`convertTo` + `mul`) |
| v1 | 2.50 | 8 位整数混合 |

**2. AI 分割耗时**

| 步骤 | 耗时 (ms/帧) |
|---|---|
| 预处理 + 模型推理(192×192) | 13.91 |
| 后处理(通道相减、放大、二值化) | 1.18 |
| 羽化 + 整数混合 | 约 1.5 |
| 合计 | 约 16.6 |

合计由分别测得的数据相加得到。

**帧率结论**:主循环每帧约 33 ms(对应摄像头的 30 fps)。HSV 路径的处理约 3.7 ms,AI 路径约 17 ms,都在预算以内,帧率主要受摄像头出帧速度限制。AI 推理占 AI 路径耗时的大头,是后续优化的重点。

## 已知问题

- AI 掩膜边缘较粗糙,且逐帧略有不同,隐身边缘可能轻微闪烁
- 模型对真人效果较好,对动漫、插画等非真人图像分割不准
- 模型与配置文件使用相对路径,必须在项目根目录运行

## 后续计划

- [x] HSV 隐身与目标追踪
- [x] 分步耗时统计与混合步骤优化
- [x] AI 人物分割(PP-HumanSeg + OpenCV DNN)
- [ ] 掩膜时间平滑,减轻边缘闪烁
- [ ] 背景图片替换(用任意图片代替采集的背景)
- [ ] 尝试更精细的抠图模型
- [ ] 路径统一处理,不依赖运行目录
- [ ] 按需用 Qt 增加交互界面

## 迁移记录

从 qmake / Qt Creator 迁移到 CMake / VS Code 的过程见 [docs/MIGRATION.md](docs/MIGRATION.md)。