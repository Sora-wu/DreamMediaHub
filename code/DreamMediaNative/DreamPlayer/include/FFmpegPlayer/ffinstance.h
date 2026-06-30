//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/ffcore.h>
#include <DreamPlayer/structs.h>
#include <FFmpegPlayer/mediaClock.h>

#include <cassert>

class FFInstance {
public:
    static FFInstance* instance();

    void setSyncType(Dream::SyncType syncType) {
        syncType_ = syncType;
    }

    [[nodiscard]] Dream::SyncType getSyncType() const {
        return syncType_;
    }

    void setFormatContext(AVFormatContext* fmtCtx) {
        fmtCtx_ = fmtCtx;
    }

    [[nodiscard]] AVFormatContext* getFormatContext() const {
        assert(fmtCtx_);
        return fmtCtx_;
    }

    [[nodiscard]] AVPacket* getFlushPkt() {
        return &flushPkt_;
    }

    void initClock(uint32_t* videoPktQueSerial, uint32_t* audioPktQueSerial);
    [[nodiscard]] double getMasterClock() const;
    [[nodiscard]] double getVideoClock() const;
    [[nodiscard]] double getAudioClock() const;
    [[nodiscard]] double getMasterSerial() const;
    [[nodiscard]] uint32_t getVideoSerial() const;
    [[nodiscard]] uint32_t getAudioSerial() const;
    [[nodiscard]] double getVideoPTS() const;
    [[nodiscard]] double getAudioPTS() const;
    void setMasterClock(double pts, uint32_t serial, double time);
    void setMasterClock(double pts, uint32_t serial);
    void setMasterClock(double pts, double time);
    void setVideoClock(double pts, uint32_t serial, double time);
    void setAudioClock(double pts, uint32_t serial, double time);
    void setVideoClock(double pts, uint32_t serial);
    void setAudioClock(double pts, uint32_t serial);
    void setVideoClock(double pts, double time);
    void setAudioClock(double pts, double time);

    float getSpeed() const;
    void setSpeed(float speed);

    void pauseMasterClock();
    void resumeMasterClock();

private:
    FFInstance();

private:
    static FFInstance* instance_;

    AVFormatContext* fmtCtx_ = nullptr;
    AVPacket flushPkt_{};

    Dream::SyncType syncType_ = Dream::SyncType::AudioMaster;
    MediaClock clock_{};
};

#define FFINSTANCE FFInstance::instance