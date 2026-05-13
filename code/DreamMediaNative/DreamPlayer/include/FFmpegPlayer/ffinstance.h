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

private:
    static FFInstance* instance_;

    AVFormatContext* fmtCtx_ = nullptr;
};

#define FFINSTANCE FFInstance::instance