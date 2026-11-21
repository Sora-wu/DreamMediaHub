# DreamMediaHub

一个轻量级、前后端分离的音视频功能合集。Qt Quick 前端包含播放器和录制器，独立仓库 `DreamMediaHubNative` 后端基于 FFmpeg，使用 C++23。

## 目录与源码获取

```text
DreamMediaHub/
  player/                     # Qt Quick 播放器
  recorder/                   # Qt Quick 录制器
  third/
    src/
      DreamMediaHubNative/    # native Git 子模块，内含 FFmpeg 子模块
    build/<平台>/            # native 安装产物，不提交到仓库
```

```bash
git clone --recurse-submodules git@github.com:Sora-wu/DreamMediaHub.git
# 已有仓库在更新后执行
git submodule update --init --recursive
```

Native 源码和 FFmpeg 开发包位于 `third/src/DreamMediaHubNative`。根工程默认从当前工作树构建 Native 库，播放器和录制器直接链接对应的构建目标。需要使用单独安装的 `Dream` CMake 包时，先用根目录的 `build_dep.sh` 或 `build_dep.bat` 安装 Native，再在配置前端时传入 `-DDREAM_USE_PREINSTALLED_NATIVE=ON`。

## Windows 编译与运行

需要 CMake 3.21+、MSVC x64（v143 工具集）、Windows SDK，以及 Qt 6.8+ 的 MSVC x64 版本。本机已验证 Qt `D:\Qt\6.11.1\msvc2022_64`、Visual Studio 2026 + v143。

FFmpeg 使用 4.x 的共享库开发包，头文件、导入库与 DLL 必须来自同一套 x64 构建，目录如下：

```text
third/src/DreamMediaHubNative/third/build/win/
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

在项目根目录执行 PowerShell 命令：

```powershell
cmake -S . -B build/windows-msvc -G "Visual Studio 18 2026" -A x64 -T v143 -DCMAKE_PREFIX_PATH="D:/Qt/6.11.1/msvc2022_64"
cmake --build build/windows-msvc --config Debug --parallel
```

如果使用 Visual Studio 2022，将生成器改为 `"Visual Studio 17 2022"`。已存在的构建目录不能切换生成器，请为另一生成器选择新的目录。

```powershell
# 播放器必须提供媒体文件路径
.\build\windows-msvc\player\Debug\playerApp.exe "D:\media\sample.mp4"

# 录制器界面；开始录制需要可用的摄像头和麦克风
.\build\windows-msvc\recorder\Debug\recorderApp.exe

# 切换构建配置
cmake --build build/windows-msvc --config Release --parallel
```

输出沿用 CMake 默认目录。上述 Visual Studio 构建的两个程序分别位于 `build/windows-msvc/player/Debug` 和 `build/windows-msvc/recorder/Debug`；Release 配置使用对应的 `Release` 子目录。着色器嵌入 Qt 资源。

构建会将对应的 Native DLL 复制到每个程序的输出目录。默认源码模式也会复制子模块中的 FFmpeg DLL，预装模式需将所选安装包中的 FFmpeg DLL 部署到程序目录。以下命令用 `windeployqt` 部署 Debug 版本的 Qt 运行依赖。

```powershell
$qtBin = "D:/Qt/6.11.1/msvc2022_64/bin"
$playerBin = "build/windows-msvc/player/Debug"
$recorderBin = "build/windows-msvc/recorder/Debug"
& "$qtBin/windeployqt.exe" --debug --qmldir player/resource "$playerBin/playerApp.exe"
& "$qtBin/windeployqt.exe" --debug --qmldir recorder/resource "$recorderBin/recorderApp.exe"
```

Release 版本把输出目录改为 `Release`，部署参数改为 `--release`。分发时复制整个配置输出目录。

Qt Creator 中选择 MSVC x64 Kit，Qt 路径使用上述安装目录，打开根目录 `CMakeLists.txt`；播放器的运行参数中填写媒体文件路径。

`build_dep.bat Debug` / `build_dep.bat Release` 负责后端构建和安装，脚本支持从其他工作目录调用，并在失败时立即退出。

## macOS 编译（Apple Silicon）

先通过 Homebrew 安装构建工具、Qt 和 FFmpeg 编码依赖。项目使用子模块中的 FFmpeg 4.x，不需要安装 Homebrew 的 FFmpeg。

```bash
brew install cmake qt pkg-config fdk-aac x264 x265
./third/src/DreamMediaHubNative/build_dep.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 8
```

Homebrew 的 `fdk-aac` 已被标记为弃用；本机使用可安装的 2.0.3 版本验证通过。

FFmpeg 安装到 `third/src/DreamMediaHubNative/third/build/macos`。两个程序为 `build/player/playerApp` 和 `build/recorder/recorderApp`，播放器需要传入媒体文件路径。构建脚本会自动识别 Homebrew 的 pkg-config 路径。

## Linux 编译

先安装支持 C++23 的编译器、CMake、Qt 6.8+ 和 FFmpeg 所需的开发依赖。FFmpeg 可使用 `third/src/DreamMediaHubNative/build_dep.sh` 构建到该子模块内的 `third/build/linux`，也可以将兼容的 FFmpeg 开发包放到该目录。

```bash
./third/src/DreamMediaHubNative/build_dep.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/Qt/gcc_64
cmake --build build --parallel
./build/player/playerApp /path/to/video.mp4
./build/recorder/recorderApp
```

根 CMake 向全部子目录定义 `PLATFORM_WINDOWS` 或 `PLATFORM_LINUX`。后端的依赖路径、链接方式和安装规则保留项目原始实现。

本次拆分仅调整目录、构建入口和文档，不修改业务代码逻辑。Linux 下已验证 FFmpeg、native 库和两个前端程序的 Debug 构建。
