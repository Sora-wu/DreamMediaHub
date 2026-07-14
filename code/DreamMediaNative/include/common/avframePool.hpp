//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <vector>

extern "C" {
#include <libavutil/frame.h>
}

class AVFramePool {
public:
    ~AVFramePool() {
        clear();
    }

    AVFrame* get() {
        if (!pool_.empty()) {
            AVFrame* frame = pool_.back();
            pool_.pop_back();
            return frame;
        }

        return av_frame_alloc();
    }

    void put(AVFrame* frame) {
        if (!frame) {
            return;
        }

        av_frame_unref(frame);
        pool_.push_back(frame);
    }

    void clear() {
        for (AVFrame* frame : pool_) {
            av_frame_free(&frame);
        }

        pool_.clear();
    }

private:
    std::vector<AVFrame*> pool_;
};