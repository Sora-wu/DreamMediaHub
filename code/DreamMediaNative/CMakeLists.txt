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
    file(GLOB COPY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/third/build/${PLATFORM_NAME}/bin/*)
    add_compile_definitions(PLATFORM_WINDOWS)
endif()

set(FFMPEG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third/build/${PLATFORM_NAME})
set(FFMPEG_LIBS avcodec avdevice avfilter avformat avutil postproc swresample swscale)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/* ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

add_library(DreamPlayer ${SRC})
target_include_directories(DreamPlayer PRIVATE ${FFMPEG_ROOT}/include include)
target_link_directories(DreamPlayer PRIVATE ${FFMPEG_ROOT}/lib)
target_link_libraries(DreamPlayer PRIVATE ${FFMPEG_LIBS})