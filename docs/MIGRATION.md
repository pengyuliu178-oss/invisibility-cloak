# 摄像头紫色区域背景替换：MSVC x64 迁移副本

原项目：`project01_2026_10_23`。
本目录为独立迁移副本，原项目的 `.pro` 与 `main.cpp` 已经校验，内容未改动。

## 迁移结果

- 保留 qmake `.pro` 工程。
- 使用 Qt 6.12.0 配套 qmake、MSVC 2022 x64、OpenCV 4.12.0。
- 项目本身仍是纯 OpenCV 控制台程序，保留 `CONFIG -= qt`，没有额外引入 Qt 窗口库。
- 紫色 HSV 阈值、形态学开闭运算、背景替换、摄像头读取与 Esc 退出逻辑均保持原样。
- `main.cpp` 仅将未使用参数的 `int main(int argc, char *argv[])` 改为 `int main()`，消除编译警告。
- `.pro` 改为链接 MSVC `.lib`，分别选择 Release 的 `opencv_world4120.lib` 和 Debug 的 `opencv_world4120d.lib`。
- 链接完成后自动复制对应 OpenCV DLL 和视频插件到程序旁边，不需要修改全局 PATH。
- 旧的 `.pro.user` 没有复制，以免继续引用 MinGW 32 位套件。

## 打开与运行

双击本目录的 `打开项目.cmd`，使用新版 Qt Creator 的独立设置打开迁移项目。
开发套件选择 `Qt 6.12.0 MSVC 2022 x64`。
按 Ctrl+B 构建，按 Ctrl+R 运行。预览窗口中按 Esc 退出。

也可以在本目录的终端中构建：

```powershell
.\build.cmd
.\build.cmd debug
```

Release 程序：`build\release\bin\project01_qt6_msvc64.exe`。
Debug 程序：`build\debug\bin\project01_qt6_msvc64.exe`。

## 图片与摄像头依赖

保留了原源码中的背景图片路径：

`D:/workspace/picture/video_screenshot_22.10.2026.png`

已确认这张图片存在，运行测试读取成功。图片没有搬动；如果要换背景或换电脑，需要修改 `main.cpp` 中的 `imread` 路径。
程序默认使用编号 0 的摄像头，运行时需确保它没有被其他应用独占。

## 验证记录

验证日期：2026-10-27。

- Release、Debug 编译通过，最终版本没有项目源码编译警告。
- 两个 exe 的 PE 机器类型均为 `0x8664`，确认是 x64。
- Release 实际运行输出 `OpenCV version: 4.12.0`，创建 `finalImg` 合成预览窗口，并在发送 Esc 后以状态码 0 正常退出。
- 已在新版 Qt Creator 中完成套件配置，通过 Ctrl+B 构建、Ctrl+R 启动 Debug 版本，并确认合成预览窗口正常。
- 地址检测版本也通过同一摄像头运行检查，没有报告地址错误。预编译 OpenCV DLL 本身未加入地址检测；MSVC 不提供 UndefinedBehaviorSanitizer，本次未运行该项。
- 原项目 `.pro` 与 `main.cpp` 的 SHA256 与迁移前一致。

Debug 构建可正常生成；Qt Creator 的 CDB 断点调试器未在本次迁移中安装。Ctrl+R 为普通运行。

Debug 日志中可能出现可选 GTK 或并行计算插件的 `INFO ... FAILED` 探测信息。本机已实际使用内置 WIN32 窗口后端成功运行，不需要为这些候选插件安装额外组件。
