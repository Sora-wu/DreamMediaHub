//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>

namespace Dream {
    struct VideoFrame;
}

class VideoDecodec : public Decodec {
public:
    explicit VideoDecodec(std::size_t frameQueueSize) : Decodec(frameQueueSize) {}
    bool openCodec(int streamIndex, AVStream* stream) override;

    uint32_t getWidth() const;
    uint32_t getHeight() const;

    void run(std::stop_token st) override;

    void getVideoFrame(Dream::VideoFrame* videoFrame);

protected:
    void cleanup() override;
    [[nodiscard]] bool decodeFrame(AVFrame* frame) override;

private:
    void pushFrame(AVFrame* frame, uint32_t serial, double pts, double duration);

private:
    SwsContext* swsCtx_ = nullptr;
    uint8_t* convertBuf_ = nullptr;
    int convertBufSize_ = 0;
};