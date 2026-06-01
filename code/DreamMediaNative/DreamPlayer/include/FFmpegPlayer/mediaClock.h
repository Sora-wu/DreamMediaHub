//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <cstdint>
#include <cmath>

class MediaClock {
    struct Clock {
        void init(uint32_t* qSerial);
        double getTime() const;
        void setTime(double pts, uint32_t serial);
        void setTime(double pts, uint32_t serial, double time);


        double pts = NAN;          // 单位：毫秒
        double lastUpdatedTime = NAN;
        double speed = 1.0;
        bool paused = true;
        uint32_t serial = 0;
        uint32_t* queSerial = nullptr;
    };

public:
    void initClock(uint32_t* videoPktQueSerial, uint32_t* audioPktQueSerial);
    [[nodiscard]] double getMasterClock() const;

private:
    Clock videoClock_;
    Clock audioClock_;
};