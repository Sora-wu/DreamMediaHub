//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/ffcore.h>

class FFInstance {
public:
    static FFInstance* instance();

    void setFormatContext(AVFormatContext* fmtCtx) {
        fmtCtx_ = fmtCtx;
    }

    [[nodiscard]] AVFormatContext* getFormatContext() const {
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
};

#define FFINSTANCE FFInstance::instance