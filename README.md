# DreamMediaHub
一个轻量级、前后端分离的3D渲染引擎，后端`DreamMediaNative`基于FFmpeg实现；基于C++23。
## 📋 编译
### 🐧 Linux
安装依赖：
```bash
cd code/DreamMediaNative
./build_dep.sh
````

### 🪟 Windows
```bat
cd code/DreamMediaNative
...(FFmpeg自行在Windows上编译)
```

## 🚀 编译项目
⚠️确保你的编译器支持C++23
```bash
cmake -B build
cmake --build build -j