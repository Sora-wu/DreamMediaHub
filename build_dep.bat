@echo off
chdir /D "code"
if exist "third\build\win" rmdir /s /q "third\build\win"

chdir /D "DreamMediaNative"
if exist "build" rmdir /s /q "build"

cmake -B build -DCMAKE_INSTALL_PREFIX=..\third\build\win
cmake --build build -j --config Debug
cmake --install build --config Debug

pause