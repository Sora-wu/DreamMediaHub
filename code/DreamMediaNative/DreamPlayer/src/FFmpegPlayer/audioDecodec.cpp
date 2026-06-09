//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/audioDecodec.h>
#include <FFmpegPlayer/ffinstance.h>
#include <DreamPlayer/structs.h>
#include <DreamPlayer/DreamContext.h>

#include <algorithm>

using namespace Dream;

namespace {
    constexpr int SAMPLE_RATE = 44100;
    constexpr int CHANNEL_COUNT = 2;
    constexpr AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;

    constexpr int AUDIO_DIFF_AVG_NB = 20;             // 用于移动平均的测量次数
    constexpr double AV_NOSYNC_THRESHOLD = 10.0;       // 超过10秒不进行同步，直接重置
    constexpr int SAMPLE_CORRECTION_PERCENT_MAX = 10; // 单次最大调整样本比例 (10%)
    constexpr double AUDIO_DIFF_THRESHOLD_MIN = 0.04; // 最小同步触发阈值 (40ms)
    constexpr double AUDIO_DIFF_THRESHOLD_MAX = 0.1;  // 最大同步触发阈值 (100ms)

    void updateAudioFrameSize(AudioFrame* audioFrame, int buffSize) {
        memset(audioFrame->data, 0, audioFrame->capacity);

        if (audioFrame->capacity >= buffSize) {
            return;
        }

        delete[] audioFrame->data;
        audioFrame->data = new char[buffSize]{};
        audioFrame->len = buffSize;
        audioFrame->capacity = buffSize;
    }
}

bool AudioDecodec::openCodec(int streamIndex, AVStream* stream) {
    if (!Decodec::openCodec(streamIndex, stream)) {
        return false;
    }

    audioDiffCum_ = 0.0;
    audioDiffAvgCount_ = 0;
    // 加权系数算法：exp(log(0.01) / AUDIO_DIFF_AVG_NB)
    audioDiffAvgCoef_ = std::exp(std::log(0.01) / AUDIO_DIFF_AVG_NB);

    int bytesPerSec = SAMPLE_RATE * CHANNEL_COUNT * av_get_bytes_per_sample(SAMPLE_FORMAT);
    int dummyHwBufSize = DreamContext::AUDIO_BUFFER_SIZE * CHANNEL_COUNT * av_get_bytes_per_sample(SAMPLE_FORMAT);
    audioDiffThreshold_ = (double)(dummyHwBufSize) / bytesPerSec;

    audioDiffThreshold_ = std::clamp(audioDiffThreshold_, AUDIO_DIFF_THRESHOLD_MIN, AUDIO_DIFF_THRESHOLD_MAX);

    return true;
}

void AudioDecodec::run(std::stop_token st) {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        av_log(nullptr, AV_LOG_FATAL, "Could not allocate audio frame");
        return;
    }

    double duration{};
    double pts{};

    while (!st.stop_requested()) {
        if (!decodeFrame(frame)) {
            if (st.stop_requested()) {
                break;
            }
            continue;
        }

        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d({ 1, frame->sample_rate });
        duration = av_q2d({ frame->nb_samples, frame->sample_rate });
        pushFrame(frame, pts, duration);
    }

    av_frame_free(&frame);
    av_log(nullptr, AV_LOG_INFO, "Finished decoding audio stream\n");
}

void AudioDecodec::getAudioFrame(AudioFrame* audioFrame) {
    audioFrame->isUpdated = false;

    Frame ap{};
    if (!acquireFrame(ap)) {
        return;
    }

    AVFrame* frame = ap.frame;
    // 更新音频时钟
    decodecTimer_ = NAN;
    if (!isnan(ap.pts)) {
        decodecTimer_ = ap.pts + ap.duration;
    }
    audioFrame->pts = decodecTimer_;

    if (fillData(frame, audioFrame)) {
        audioFrame->isUpdated = true;
    }

    releaseFrame(ap);
}

void AudioDecodec::cleanup() {
    if (swrCtx_) {
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }

    Decodec::cleanup();
}

void AudioDecodec::refreshSwr(AVFrame* frame) {
    if (frame->sample_rate != cachedSampleRate || frame->channel_layout != cachedChannelLayout || frame->format != cachedFormat) {
        swr_free(&swrCtx_);
        swrCtx_ = swr_alloc_set_opts(nullptr,
                                     AV_CH_LAYOUT_STEREO, SAMPLE_FORMAT, SAMPLE_RATE,
                                     frame->channel_layout, (AVSampleFormat)frame->format, frame->sample_rate,
                                     0, nullptr);
        if (!swrCtx_ || swr_init(swrCtx_) < 0) {
            av_log(nullptr, AV_LOG_ERROR, "swr_alloc_set_opts failed\n");
            swr_free(&swrCtx_);
            av_frame_unref(frame);
            av_frame_free(&frame);
            return;
        }

        cachedSampleRate = frame->sample_rate;
        cachedChannelLayout = frame->channel_layout;
        cachedFormat = frame->format;
    }
}

bool AudioDecodec::fillData(AVFrame* frame, AudioFrame* audioFrame) {
    const int nbSamples = adjustSamples(frame->nb_samples);

    if (frame->sample_rate != SAMPLE_RATE || frame->channels != CHANNEL_COUNT || frame->format != SAMPLE_FORMAT) {
        refreshSwr(frame);
        if (nbSamples != frame->nb_samples) {
            const int sampleDelta = av_rescale_rnd(nbSamples - frame->nb_samples, SAMPLE_RATE, frame->sample_rate, AV_ROUND_UP);
            const  int compensationDistance = av_rescale_rnd(nbSamples, frame->sample_rate, SAMPLE_RATE, AV_ROUND_UP);
            if (swr_set_compensation(swrCtx_, sampleDelta, compensationDistance) < 0) {
                av_log(nullptr, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return false;
            }
        }

        const uint8_t** in = (const uint8_t**)frame->extended_data;
        // 简单推导：无论采样率怎么变，这段音频播放的物理时间长度是不变的，而time = nb_samples / sample_rate，分别带入双方变量即可
        const int outSampleCount = av_rescale_rnd(nbSamples, SAMPLE_RATE, frame->sample_rate, AV_ROUND_UP) + 256;
        const int bufferSize = av_samples_get_buffer_size(nullptr, CHANNEL_COUNT, outSampleCount,
                                                                SAMPLE_FORMAT, 1);
        updateAudioFrameSize(audioFrame, bufferSize);
        const int converted = swr_convert(swrCtx_, (uint8_t**)&audioFrame->data, outSampleCount, in, nbSamples);
        if (converted <= 0) {
            av_frame_unref(frame);
            av_frame_free(&frame);
            return false;
        }
        // 因为转换的采样数一定不会比outSampleCount大，所以这里可以不重新分配内存
        audioFrame->len = av_samples_get_buffer_size(nullptr, CHANNEL_COUNT, converted, SAMPLE_FORMAT, 1);
    }
    else {
        const int bufferSize = av_samples_get_buffer_size(nullptr, frame->channels, nbSamples, (AVSampleFormat)frame->format, 1);
        audioFrame->len = bufferSize;
        updateAudioFrameSize(audioFrame, bufferSize);
        memcpy(audioFrame->data, frame->data[0], bufferSize);
    }

    return true;
}

int AudioDecodec::adjustSamples(int nbSamples) {
    int wantedNbSamples = nbSamples;

    if (FFINSTANCE()->getSyncType() != SyncType::AudioMaster) {
        double diff = mediaClock_->getAudioClock() - mediaClock_->getMasterClock();

        if (!std::isnan(diff) && std::fabs(diff) < AV_NOSYNC_THRESHOLD) {
            audioDiffCum_ = diff + audioDiffAvgCoef_ * audioDiffCum_;

            if (audioDiffAvgCount_ < AUDIO_DIFF_AVG_NB) {
                // 测量次数不够，暂不进行对齐估算
                audioDiffAvgCount_++;
            } else {
                // 计算滤波后的平均时钟差值
                double avgDiff = audioDiffCum_ * (1.0 - audioDiffAvgCoef_);

                // 当平均偏差超过设定的阈值时，触发采样数调整
                if (std::fabs(avgDiff) >= audioDiffThreshold_) {
                    wantedNbSamples = nbSamples + static_cast<int>(diff * SAMPLE_RATE);
                    int delta = wantedNbSamples - nbSamples;
                    int maxDelta = (SAMPLE_CORRECTION_PERCENT_MAX * nbSamples) / 100;

                    // 边界控制：限制单次变调/调整的最大比例，防止声音过度撕裂或卡顿
                    if (delta < 0) {
                        if (delta < -maxDelta) {
                            wantedNbSamples = nbSamples - maxDelta;
                        }
                    } else {
                        if (delta > maxDelta) {
                            wantedNbSamples = nbSamples + maxDelta;
                        }
                    }
                }
            }
        } else {
            // 偏差太大或出现异常（例如执行了 Seek 接入），直接重置同步状态
            audioDiffAvgCount_ = 0;
            audioDiffCum_ = 0;
        }
    }

    return wantedNbSamples;
}
