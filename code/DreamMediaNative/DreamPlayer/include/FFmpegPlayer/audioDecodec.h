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
    bool fillData(AVFrame* frame, Dream::AudioFrame* audioFrame);
    // 直接参考ffplay的实现
    int adjustSamples(int nbSamples);

private:
    SwrContext* swrCtx_ = nullptr;
    uint64_t cachedChannelLayout = 0;
    int cachedFormat = AV_SAMPLE_FMT_NONE;
    int cachedSampleRate = 0;

    double audioDiffCum_ = 0.0;         // 累计时钟差值
    double audioDiffAvgCoef_ = 0.0;     // 加权平均系数
    int audioDiffAvgCount_ = 0;         // 差值测量计数器
    double audioDiffThreshold_ = 0.0;   // 触发采点调整的阈值
};