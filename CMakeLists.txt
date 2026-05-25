cmake_minimum_required(VERSION 3.21)
project(DreamMediaHub LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(PLATFORM_NAME "linux")
    list(APPEND CMAKE_CXX_FLAGS "-m64")
    add_compile_definitions(PLATFORM_LINUX)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(PLATFORM_NAME "win")
    file(GLOB_RECURSE COPY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/code/third/build/${PLATFORM_NAME}/*.dll)
    add_compile_definitions(PLATFORM_WINDOWS)
endif()

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/code/third/build/${PLATFORM_NAME})

add_subdirectory(code/player)