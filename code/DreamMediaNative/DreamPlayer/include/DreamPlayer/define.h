//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#ifdef PLATFORM_WINDOWS
    #ifdef DreamPlayer_EXPORTS
        #define DREAM_PLAYER_API __declspec(dllexport)
    #else
        #define DREAM_PLAYER_API __declspec(dllimport)
    #endif
#else
    #define DREAM_PLAYER_API
#endif

namespace Dream {
    struct VideoFrame {
        uint32_t frameWidth{};
        uint32_t frameHeight{};

        char* dataY{};
        uint32_t lenY{};

        char* dataU{};
        uint32_t lenU{};

        char* dataV{};
        uint32_t lenV{};
    };
}