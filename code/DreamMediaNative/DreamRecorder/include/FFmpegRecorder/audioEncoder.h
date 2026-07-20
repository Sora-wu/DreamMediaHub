//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegRecorder/encoder.h>
#include <DreamRecorder/structs.h>

extern "C" {
    struct SwrContext;
    struct AVAudioFifo;
}

class AudioEncoder : public Encoder {
public:
    AudioEncoder(const Dream::AudioParam& param, uint32_t queSize) : Encoder(queSize), param_(param) {}
    ~AudioEncoder() override;

    void addStream(AVCodecID codecID) override;
    void initCodecContext(AVCodecID codecID) override;

    void run(std::stop_token st) override;

    void pushData(const char* data, uint32_t size, uint64_t pts);

private:
    Dream::AudioParam param_;
    SwrContext* swrCtx_ = nullptr;

    AVAudioFifo* audioFifo_ = nullptr;
    int64_t currentPts_ = AV_NOPTS_VALUE;
    uint8_t* converted_[1] = { nullptr };
};
