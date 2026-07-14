//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegRecorder/ffcore.h>

#include <cassert>

extern "C" {
    struct AVFormatContext;
}

class FFInstance {
public:
    static FFInstance* instance();

    [[nodiscard]] AVFormatContext* getFormatContext() const {
        assert(fmtCtx_);
        return fmtCtx_;
    }

    void setFormatContext(AVFormatContext* fmtCtx) {
        fmtCtx_ = fmtCtx;
    }

private:
    FFInstance() = default;
    AVFormatContext* fmtCtx_ = nullptr;
};

#define FFINSTANCE FFInstance::instance