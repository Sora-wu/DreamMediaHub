//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <cstdint>

#ifdef PLATFORM_WINDOWS
    #ifdef DreamRecorder_EXPORTS
        #define DREAM_RECORDER_API __declspec(dllexport)
    #else
        #define DREAM_RECORDER_API __declspec(dllimport)
    #endif
#else
    #define DREAM_RECORDER_API
#endif
