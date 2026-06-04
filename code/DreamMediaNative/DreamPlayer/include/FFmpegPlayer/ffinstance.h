//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/ffcore.h>

#include <cassert>

class FFInstance {
public:
    static FFInstance* instance();

    void setSyncType(SyncType syncType) {
        syncType_ = syncType;
    }

    [[nodiscard]] SyncType getSyncType() const {
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

private:
    FFInstance();

private:
    static FFInstance* instance_;

    AVFormatContext* fmtCtx_ = nullptr;
    AVPacket flushPkt_{};

    SyncType syncType_ = SyncType::AudioMaster;
    // SyncType syncType_ = SyncType::VideoMaster;
};

#define FFINSTANCE FFInstance::instance