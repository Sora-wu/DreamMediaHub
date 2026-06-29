//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>
#include <FFmpegPlayer/audioSpeedFilter.h>
#include <FFmpegPlayer/avframePool.hpp>

#include <atomic>

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
    void reopen() override;
    void startDecode() override;

    void getAudioFrame(Dream::AudioFrame* audioFrame);

    void setSpeed(float speed);

protected:
    void cleanup() override;

private:
    void refreshSwr(AVFrame* frame);
    bool fillData(AVFrame* frame, Dream::AudioFrame* audioFrame);
    // 直接参考ffplay的实现
    bool adjustSamples(AVFrame* frame);

    std::vector<AVFrame*> processFrame(AVFrame* frame);
    AVFrame* convertFrame(AVFrame* frame);
    // 将重采样/过滤镜的输出结果推入队列
    void pushFrames(const std::vector<AVFrame*>& frames, double pts);

private:
    SwrContext* swrCtx_ = nullptr;
    std::unique_ptr<AudioSpeedFilter> audioSpeedFilter_;
    std::atomic<float> speed_ = 1.0f;
    std::vector<AVFrame*> paddingFrames_;

    uint64_t cachedChannelLayout_ = 0;
    int cachedFormat_ = AV_SAMPLE_FMT_NONE;
    int cachedSampleRate_ = 0;

    double audioDiffCum_ = 0.0;         // 累计时钟差值
    double audioDiffAvgCoef_ = 0.0;     // 加权平均系数
    int audioDiffAvgCount_ = 0;         // 差值测量计数器
    double audioDiffThreshold_ = 0.0;   // 触发采点调整的阈值

    AVFramePool framePool_;
};