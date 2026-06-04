//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamPlayer/DreamPlayer.h>

namespace Dream {
    class DREAM_PLAYER_API DreamContext {
    public:
        static DreamPlayer* createDreamPlayer();
        static void destroyDreamPlayer(DreamPlayer** player);
        static constexpr int AUDIO_BUFFER_SIZE = 4096;
    };
} // Dream