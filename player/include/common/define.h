//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#if defined(PLATFORM_MACOS)
#include <QOpenGLFunctions_4_1_Core>
#define PLAYER_OPENGL_FUNCTIONS QOpenGLFunctions_4_1_Core
#define PLAYER_OPENGL_VERSION_MAJOR 4
#define PLAYER_OPENGL_VERSION_MINOR 1
#define PLAYER_GLSL_VERSION "#version 410 core\n"
#elif defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)
#include <QOpenGLFunctions_4_5_Core>
#define PLAYER_OPENGL_FUNCTIONS QOpenGLFunctions_4_5_Core
#define PLAYER_OPENGL_VERSION_MAJOR 4
#define PLAYER_OPENGL_VERSION_MINOR 5
#define PLAYER_GLSL_VERSION "#version 450 core\n"
#else
#error "Unsupported player platform"
#endif
