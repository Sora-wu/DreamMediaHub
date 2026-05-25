//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <cstdint>

#ifdef PLATFORM_WINDOWS
    #ifdef DreamPlayer_EXPORTS
        #define DREAM_PLAYER_API __declspec(dllexport)
    #else
        #define DREAM_PLAYER_API __declspec(dllimport)
    #endif
#else
    #define DREAM_PLAYER_API
#endif
