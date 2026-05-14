//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>

class VideoDecodec : public Decodec {
public:
    explicit VideoDecodec(std::size_t frameQueueSize) : Decodec(frameQueueSize) {}
    bool openCodec(int streamIndex, AVStream* stream) override;

    void run() override;

protected:
    void cleanup() override;
    [[nodiscard]] bool decodeFrame(AVFrame* frame) override;

private:
};