# DreamMediaHub

一个轻量级、前后端分离的音视频功能合集。Qt Quick 前端包含播放器和录制器，`DreamMediaNative` 后端基于 FFmpeg，使用 C++23。

## Windows 编译与运行

需要 CMake 3.21+、MSVC x64（v143 工具集）、Windows SDK，以及 Qt 6.8+ 的 MSVC x64 版本。本机已验证 Qt `D:\Qt\6.11.1\msvc2022_64`、Visual Studio 2026 + v143。

FFmpeg 使用 4.x 的共享库开发包，头文件、导入库与 DLL 必须来自同一套 x64 构建，目录如下：

```text
code/DreamMediaNative/third/build/win/
  include/libavcodec/avcodec.h
  include/libavformat/avformat.h
  ...
  lib/avcodec.lib
  lib/avformat.lib
  ...
  bin/avcodec-58.dll
  bin/avformat-58.dll
  ...（包含 FFmpeg 依赖的其他 DLL）
```

沿用项目原来的构建流程：先构建、安装后端到 `code/third/build/win`，前端再通过 `find_package(Dream)` 使用已安装的库。FFmpeg 路径和库名沿用原有 CMake 配置。

在项目根目录执行 PowerShell 命令：

```powershell
.\build_dep.bat Debug
cmake -S . -B build/windows-msvc -G "Visual Studio 18 2026" -A x64 -T v143 -DCMAKE_PREFIX_PATH="D:/Qt/6.11.1/msvc2022_64"
cmake --build build/windows-msvc --config Debug --parallel
```

如果使用 Visual Studio 2022，将生成器改为 `"Visual Studio 17 2022"`。已存在的构建目录不能切换生成器，请为另一生成器选择新的目录。

```powershell
# 播放器必须提供媒体文件路径
.\build\windows-msvc\code\player\Debug\playerApp.exe "D:\media\sample.mp4"

# 录制器界面；开始录制需要可用的摄像头和麦克风
.\build\windows-msvc\code\recorder\Debug\recorderApp.exe

# 切换构建配置时，先安装相同配置的后端，再重新配置前端
.\build_dep.bat Release
cmake -S . -B build/windows-msvc
cmake --build build/windows-msvc --config Release --parallel
```

输出沿用 CMake 默认目录。上述 Visual Studio 构建的两个程序分别位于 `build/windows-msvc/code/player/Debug` 和 `build/windows-msvc/code/recorder/Debug`；Release 配置使用对应的 `Release` 子目录。着色器嵌入 Qt 资源。

Qt 和 FFmpeg 的运行依赖由你手动部署，CMake 不再自动调用部署工具。以下是 Debug 版本的部署命令：

```powershell
$qtBin = "D:/Qt/6.11.1/msvc2022_64/bin"
$playerBin = "build/windows-msvc/code/player/Debug"
$recorderBin = "build/windows-msvc/code/recorder/Debug"
Copy-Item "code/third/build/win/bin/*.dll" $playerBin
Copy-Item "code/third/build/win/bin/*.dll" $recorderBin
& "$qtBin/windeployqt.exe" --debug --qmldir code/player/resource "$playerBin/playerApp.exe"
& "$qtBin/windeployqt.exe" --debug --qmldir code/recorder/resource "$recorderBin/recorderApp.exe"
```

Release 版本把输出目录改为 `Release`，部署参数改为 `--release`。分发时复制整个配置输出目录。

Qt Creator 中选择 MSVC x64 Kit，Qt 路径使用上述安装目录，打开根目录 `CMakeLists.txt`；播放器的运行参数中填写媒体文件路径。

`build_dep.bat Debug` / `build_dep.bat Release` 负责后端构建和安装，脚本支持从其他工作目录调用，并在失败时立即退出。

## Linux 编译

先安装支持 C++23 的编译器、CMake、Qt 6.8+ 和 FFmpeg 开发包。FFmpeg 也可使用 `code/DreamMediaNative/build_dep.sh` 构建到 `third/build/linux`。

```bash
./build_dep.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/Qt/gcc_64
cmake --build build --parallel
./build/code/player/playerApp /path/to/video.mp4
./build/code/recorder/recorderApp
```

根 CMake 向全部子目录定义 `PLATFORM_WINDOWS` 或 `PLATFORM_LINUX`。后端的依赖路径、链接方式和安装规则保留项目原始实现。

本次改动仅保留构建、MSVC 警告与 Qt 资源兼容性调整，额外的运行逻辑修复已回滚。已知问题及排查线索见 [运行逻辑问题记录](docs/runtime-bugs.md)，这些问题尚未修复。当前机器未配置 Linux 发行版，Linux 构建尚未实测。
