//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <DreamPlayer/DreamContext.h>

#include <FFmpegPlayer/FFmpegPlayer.h>

using namespace Dream;

DreamPlayer* DreamContext::createDreamPlayer() {
    return new FFmpegPlayer;
}

void DreamContext::destroyDreamPlayer(DreamPlayer** player) {
    if (*player) {
        delete *player;
        *player = nullptr;
    }
}