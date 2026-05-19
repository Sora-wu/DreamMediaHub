//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>
#include <DreamPlayer/define.h>

namespace Dream {
    struct VideoFrame;
}

class VideoDecodec : public Decodec {
public:
    explicit VideoDecodec(std::size_t frameQueueSize) : Decodec(frameQueueSize) {}
    bool openCodec(int streamIndex, AVStream* stream) override;

    void run() override;

    Dream::VideoFrame getVideoFrame();

protected:
    void cleanup() override;
    [[nodiscard]] bool decodeFrame(AVFrame* frame) override;

private:
    void pushFrame(AVFrame* frame, uint32_t serial, double pts, double duration);

private:
    SwsContext* swsCtx_ = nullptr;
};