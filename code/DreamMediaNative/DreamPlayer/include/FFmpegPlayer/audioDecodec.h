//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>

extern "C" {
    struct SwrContext;
}

namespace Dream {
    struct AudioFrame;
}

class AudioDecodec : public Decodec {
public:
    explicit AudioDecodec(std::size_t frameQueueSize) : Decodec(frameQueueSize) {}
    bool openCodec(int streamIndex, AVStream* stream) override;

    void run(std::stop_token st) override;

    void getAudioFrame(Dream::AudioFrame* audioFrame);

protected:
    void cleanup() override;

private:
    void refreshSwr(AVFrame* frame);

private:
    SwrContext* swrCtx_ = nullptr;
    uint64_t cachedChannelLayout = 0;
    int cachedFormat = AV_SAMPLE_FMT_NONE;
    int cachedSampleRate = 0;
};