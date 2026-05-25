//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamPlayer/define.h>
#include <DreamPlayer/structs.h>

namespace Dream {
    class DREAM_PLAYER_API DreamPlayer {
    public:
        virtual ~DreamPlayer() = default;
        virtual bool open(const char* url) = 0;
        virtual void play() = 0;
        virtual void pause() = 0;
        virtual void stop() = 0;

        virtual void seek(double position) = 0;
        [[nodiscard]] virtual uint64_t getDuration() const = 0;

        virtual void setVolume(uint32_t volume) = 0;
        [[nodiscard]] virtual uint32_t getVolume() const = 0;

        virtual void setSpeed(double speed) = 0;

        virtual void getVideoFrame(VideoFrame* videoFrame) = 0;
    };
}