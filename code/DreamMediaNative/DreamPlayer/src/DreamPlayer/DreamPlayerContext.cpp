//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <DreamPlayer/DreamPlayerContext.h>
#include <FFmpegPlayer/FFmpegPlayer.h>

using namespace Dream;

DreamPlayer* DreamPlayerContext::createDreamPlayer() {
    return new FFmpegPlayer;
}

void DreamPlayerContext::destroyDreamPlayer(DreamPlayer** player) {
    if (*player) {
        delete *player;
        *player = nullptr;
    }
}