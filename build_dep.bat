@echo off
setlocal
set "BUILD_CONFIG=%~1"
if not defined BUILD_CONFIG set "BUILD_CONFIG=Debug"

cmake -S "%~dp0third\DreamMediaHubNative" -B "%~dp0build\native\windows" -A x64 -T v143 "-DCMAKE_INSTALL_PREFIX=%~dp0third\build\win"
if errorlevel 1 exit /b 1
cmake --build "%~dp0build\native\windows" --parallel --config "%BUILD_CONFIG%"
if errorlevel 1 exit /b 1
cmake --install "%~dp0build\native\windows" --config "%BUILD_CONFIG%"
if errorlevel 1 exit /b 1
