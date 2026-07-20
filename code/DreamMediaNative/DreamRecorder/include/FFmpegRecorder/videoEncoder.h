//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamRecorder/structs.h>
#include <FFmpegRecorder/encoder.h>

extern "C" {
    struct SwsContext;
}

class VideoEncoder : public Encoder {
public:
    VideoEncoder(const Dream::VideoParam& param, uint32_t queSize) : Encoder(queSize), param_(param) {}
    ~VideoEncoder() override;

    void addStream(AVCodecID codecID) override;
    void initCodecContext(AVCodecID codecID) override;

    Dream::VideoFrameBuffer getVideoBuffer();
    void commitVideoBuffer(const Dream::VideoFrameBuffer& buffer, uint64_t pts);;

    void run(std::stop_token st) override;

private:
    Dream::VideoParam param_{};
    SwsContext* swsCtx_ = nullptr;
    AVFramePool userFramePool_;
};
