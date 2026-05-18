cmake_minimum_required(VERSION 3.21)
project(DreamMediaNative LANGUAGES C CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(PLATFORM_NAME "linux")
    list(APPEND CMAKE_CXX_FLAGS "-m64")
    add_compile_definitions(PLATFORM_LINUX)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(PLATFORM_NAME "win")
    file(GLOB_RECURSE COPY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/third/build/${PLATFORM_NAME}/*.dll)
    add_compile_definitions(PLATFORM_WINDOWS)
endif()

set(FFMPEG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third/build/${PLATFORM_NAME})
set(FFMPEG_LIBS avcodec avdevice avfilter avformat avutil postproc swresample swscale)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(Dream_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Dream)

set(BUILD_SHARED_LIBS ON)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DreamConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/DreamConfig.cmake
    INSTALL_DESTINATION ${Dream_INSTALL_CMAKEDIR}
    NO_SET_AND_CHECK_MACRO
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/DreamConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/DreamConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/DreamConfigVersion.cmake
    DESTINATION ${Dream_INSTALL_CMAKEDIR}
)

add_subdirectory(DreamPlayer)