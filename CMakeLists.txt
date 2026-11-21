cmake_minimum_required(VERSION 3.21)
project(DreamMediaHub LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(PLATFORM_NAME "linux")
    add_compile_definitions(PLATFORM_LINUX)
elseif (APPLE)
    set(PLATFORM_NAME "macos")
    add_compile_definitions(PLATFORM_MACOS)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(PLATFORM_NAME "win")
    add_compile_definitions(PLATFORM_WINDOWS)
    set(QT_SKIP_AUTO_QML_PLUGIN_INCLUSION ON)
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

include(GNUInstallDirs)
option(DREAM_USE_PREINSTALLED_NATIVE "Use an installed Dream package for the Qt apps" OFF)

if(DREAM_USE_PREINSTALLED_NATIVE)
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/third/build/${PLATFORM_NAME}")
    find_package(Dream CONFIG REQUIRED COMPONENTS Player Recorder)
else()
    add_subdirectory(third/src/DreamMediaHubNative)
    if(WIN32)
        file(GLOB_RECURSE dream_ffmpeg_dlls CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/third/src/DreamMediaHubNative/third/build/${PLATFORM_NAME}/*.dll"
        )
    endif()
endif()

add_subdirectory(player)
add_subdirectory(recorder)

if(WIN32)
    function(copy_dream_runtime app_target native_target)
        # The DLL can change without the import library triggering an app relink.
        add_custom_target(${app_target}_dream_runtime
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${app_target}>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:${native_target}>" ${dream_ffmpeg_dlls} "$<TARGET_FILE_DIR:${app_target}>"
            VERBATIM
        )
        add_dependencies(${app_target} ${app_target}_dream_runtime)
    endfunction()

    copy_dream_runtime(playerApp Dream::Player)
    copy_dream_runtime(recorderApp Dream::Recorder)
endif()