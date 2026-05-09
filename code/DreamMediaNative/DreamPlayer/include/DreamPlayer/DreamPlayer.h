//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

namespace Dream {
    class DreamPlayer {
    public:
        virtual ~DreamPlayer() = default;
        virtual bool open(const char* url) = 0;
        virtual void play() = 0;
        virtual void pause() = 0;
        virtual void stop() = 0;

        virtual void seek(double position) = 0;
        virtual double getDuration() const = 0;

        virtual void setVolume(int volume) = 0;
        virtual int getVolume() const = 0;

        virtual void setSpeed(double speed) = 0;
    };
}