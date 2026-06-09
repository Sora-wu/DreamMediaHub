//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/ffcore.h>
#include <DreamPlayer/structs.h>

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

private:
    FFInstance();

private:
    static FFInstance* instance_;

    AVFormatContext* fmtCtx_ = nullptr;
    AVPacket flushPkt_{};

    Dream::SyncType syncType_ = Dream::SyncType::AudioMaster;
};

#define FFINSTANCE FFInstance::instance