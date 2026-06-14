//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <cstdint>
#include <cmath>
#include <atomic>

class MediaClock {
    struct Clock {
        void init(uint32_t* qSerial);
        double getTime() const;
        void setTime(double pts, uint32_t serial);
        void setTime(double pts, uint32_t serial, double time);
        uint32_t getSerial() const {
            return serial;
        }

        std::atomic<double> pts = NAN;          // 单位：秒
        std::atomic<double> lastUpdatedTime = NAN;
        double speed = 1.0;
        bool paused = true;
        std::atomic<uint32_t> serial = 0;
        uint32_t* queSerial = nullptr;
    };

public:
    void initClock(uint32_t* videoPktQueSerial, uint32_t* audioPktQueSerial);
    [[nodiscard]] double getMasterClock() const;
    [[nodiscard]] double getVideoClock() const;
    [[nodiscard]] double getAudioClock() const;
    [[nodiscard]] uint32_t getVideoSerial() const;
    [[nodiscard]] uint32_t getAudioSerial() const;
    void setMasterClock(double pts, uint32_t serial, double time);
    void setVideoClock(double pts, uint32_t serial, double time);
    void setAudioClock(double pts, uint32_t serial, double time);
    void pauseMasterClock();
    void resumeMasterClock();

private:
    Clock videoClock_;
    Clock audioClock_;
};