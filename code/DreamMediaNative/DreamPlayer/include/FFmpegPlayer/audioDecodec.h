//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>

extern "C" {
    struct SwrContext;
}

class AudioDecodec : public Decodec {
public:
    explicit AudioDecodec(std::size_t frameQueueSize) : Decodec(frameQueueSize) {}
    bool openCodec(int streamIndex, AVStream* stream) override;

    void run(std::stop_token st) override;

    void getAudioFrame(Dream::AudioFrame* frame);

protected:
    void cleanup() override;
    [[nodiscard]] bool decodeFrame(AVFrame* frame) override;

private:
    SwrContext* swrCtx_ = nullptr;
};