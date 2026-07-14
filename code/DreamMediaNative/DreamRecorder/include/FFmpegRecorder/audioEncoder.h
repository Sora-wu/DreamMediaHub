//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegRecorder/encoder.h>
#include <DreamRecorder/structs.h>

class AudioEncoder : public Encoder {
public:
    AudioEncoder(const Dream::AudioParam& param, uint32_t queSize) : Encoder(queSize), param_(param) {}

    void run(std::stop_token st) override;

    void initCodecContext(AVCodecID codecID) override;

private:
    const Dream::AudioParam& param_;
};
