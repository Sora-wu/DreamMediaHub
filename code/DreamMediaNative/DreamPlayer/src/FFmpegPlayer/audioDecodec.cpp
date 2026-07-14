//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/audioDecodec.h>
#include <FFmpegPlayer/ffinstance.h>
#include <DreamPlayer/structs.h>
#include <DreamPlayer/DreamPlayerContext.h>

#include <algorithm>

using namespace Dream;

namespace {
    constexpr int SAMPLE_RATE = 44100;
    constexpr int CHANNEL_COUNT = 2;
    constexpr AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
    const uint64_t CHANNEL_LAYOUT = AV_CH_LAYOUT_STEREO;

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

AudioDecodec::~AudioDecodec() {
    if (swrCtx_) {
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }

    if (audioFrame_.data) {
        delete[] audioFrame_.data;
        audioFrame_.data = nullptr;
        audioFrame_.len = 0;
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
    int dummyHwBufSize = DreamPlayerContext::AUDIO_BUFFER_SIZE * CHANNEL_COUNT * av_get_bytes_per_sample(SAMPLE_FORMAT);
    audioDiffThreshold_ = (double)(dummyHwBufSize) / bytesPerSec;
    audioDiffThreshold_ = std::clamp(audioDiffThreshold_, AUDIO_DIFF_THRESHOLD_MIN, AUDIO_DIFF_THRESHOLD_MAX);

    audioSpeedFilter_ = std::make_unique<AudioSpeedFilter>();
    audioSpeedFilter_->initFilter(speed_, SAMPLE_RATE, SAMPLE_FORMAT, CHANNEL_LAYOUT);

    return true;
}

void AudioDecodec::run(std::stop_token st) {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        av_log(nullptr, AV_LOG_FATAL, "Could not allocate audio frame");
        return;
    }

    double pts{};
    while (!st.stop_requested()) {
        {
            std::lock_guard lock(mtx_);
            pushFrames(paddingFrames_, decodecTimer_);
            paddingFrames_.clear();
        }

        if (!decodeFrame(frame)) {
            if (st.stop_requested()) {
                break;
            }
            continue;
        }

        std::vector<AVFrame*> frames = processFrame(frame);
        pts = (double)frame->pts / frame->sample_rate;
        pushFrames(frames, pts);
    }

    av_frame_free(&frame);
    av_log(nullptr, AV_LOG_INFO, "Finished decoding audio stream\n");
}

void AudioDecodec::reopen() {
    Decodec::reopen();
}

void AudioDecodec::startDecode() {
    start("audio thread");
}

AudioFrame* AudioDecodec::getAudioFrame() {
    audioFrame_.isUpdated = false;

    Frame ap{};
    if (!acquireFrame(ap)) {
        return &audioFrame_;
    }

    AVFrame* frame = ap.frame;
    // 更新音频时钟
    decodecTimer_ = NAN;
    if (!isnan(ap.pts)) {
        decodecTimer_ = ap.pts + ap.duration;
    }
    audioFrame_.pts = decodecTimer_;
    audioFrame_.serial = ap.serial;

    if (fillData(frame) && !isPause_) {
        audioFrame_.isUpdated = true;
    }

    releaseFrame(ap);
    return &audioFrame_;
}

void AudioDecodec::setSpeed(float speed) {
    if (!FFUtils::isEqual(speed, speed_)) {
        std::lock_guard lock(mtx_);
        speed_ = speed;

        // flush
        audioSpeedFilter_->addFrame(nullptr);
        while (true) {
            AVFrame* out = framePool_.get();
            int res = audioSpeedFilter_->getFrame(out);
            if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
                break;
            }
            if (res < 0) {
                av_log(nullptr, AV_LOG_ERROR, "Error occurred during filtering\n");
                break;
            }

            paddingFrames_.push_back(out);
        }

        audioSpeedFilter_ = std::make_unique<AudioSpeedFilter>();
        audioSpeedFilter_->initFilter(speed_, SAMPLE_RATE, SAMPLE_FORMAT, CHANNEL_LAYOUT);
    }
}

void AudioDecodec::cleanup() {
    if (swrCtx_) {
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }

    audioSpeedFilter_.reset();

    cachedSampleRate_ = 0;
    cachedChannelLayout_ = 0;
    cachedFormat_ = AV_SAMPLE_FMT_NONE;

    Decodec::cleanup();
}

void AudioDecodec::refreshSwr(AVFrame* frame) {
    if (frame->sample_rate != cachedSampleRate_ || frame->channel_layout != cachedChannelLayout_ || frame->format != cachedFormat_) {
        swr_free(&swrCtx_);
        swrCtx_ = swr_alloc_set_opts(nullptr,
                                     CHANNEL_LAYOUT, SAMPLE_FORMAT, SAMPLE_RATE,
                                     frame->channel_layout, (AVSampleFormat)frame->format, frame->sample_rate,
                                     0, nullptr);
        if (!swrCtx_ || swr_init(swrCtx_) < 0) {
            av_log(nullptr, AV_LOG_ERROR, "swr_alloc_set_opts failed\n");
            swr_free(&swrCtx_);
            av_frame_unref(frame);
            av_frame_free(&frame);
            return;
        }

        cachedSampleRate_ = frame->sample_rate;
        cachedChannelLayout_ = frame->channel_layout;
        cachedFormat_ = frame->format;
    }
}

bool AudioDecodec::fillData(AVFrame* frame) {
    if (!adjustSamples(frame)) {
        return false;
    }

    const int bufferSize = av_samples_get_buffer_size(nullptr, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);
    audioFrame_.len = bufferSize;
    updateAudioFrameSize(&audioFrame_, bufferSize);
    memcpy(audioFrame_.data, frame->data[0], bufferSize);

    return true;
}

bool AudioDecodec::adjustSamples(AVFrame* frame) {
    if (!FFUtils::isEqual(1.0f, speed_)) {
        return true;
    }

    const int nbSamples = frame->nb_samples;
    if (FFINSTANCE()->getSyncType() != SyncType::AudioMaster) {
        int wantedNbSamples = 0;
        const double diff = FFINSTANCE()->getAudioClock() - FFINSTANCE()->getMasterClock();

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

        if (wantedNbSamples != frame->nb_samples) {
            const int sampleDelta = av_rescale_rnd(nbSamples - frame->nb_samples, SAMPLE_RATE, frame->sample_rate, AV_ROUND_UP);
            const  int compensationDistance = av_rescale_rnd(nbSamples, frame->sample_rate, SAMPLE_RATE, AV_ROUND_UP);
            if (swr_set_compensation(swrCtx_, sampleDelta, compensationDistance) < 0) {
                av_log(nullptr, AV_LOG_ERROR, "swr_set_compensation() failed\n");
                return false;
            }
        }
    }

    return true;
}

std::vector<AVFrame*> AudioDecodec::processFrame(AVFrame* frame) {
    std::vector<AVFrame*> frames;

    std::lock_guard lock(mtx_);
    if (!audioSpeedFilter_) {
        AVFrame* dup = framePool_.get();
        if (dup && av_frame_ref(dup, frame) >= 0) {
            frames.push_back(dup);
        }
        return frames;
    }

    AVFrame* converted = convertFrame(frame);
    if (!converted) {
        AVFrame* dup = framePool_.get();
        if (dup && av_frame_ref(dup, frame) >= 0) {
            frames.push_back(dup);
        }
        return frames;
    }

    if (audioSpeedFilter_->addFrame(converted) < 0) {
        framePool_.put(converted);
        AVFrame* dup = framePool_.get();
        if (dup && av_frame_ref(dup, frame) >= 0) {
            frames.push_back(dup);
        }
        return frames;
    }
    framePool_.put(converted);

    while (true) {
        AVFrame* out = framePool_.get();
        if (!out) {
            break;
        }

        const int res = audioSpeedFilter_->getFrame(out);
        if (res == AVERROR(EAGAIN) || res == AVERROR_EOF) {
            framePool_.put(out);
            break;
        }
        if (res < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Error while receiving audio from the filter\n");
            framePool_.put(out);
            break;
        }

        frames.push_back(out);
    }

    return frames;
}

AVFrame* AudioDecodec::convertFrame(AVFrame* frame) {
    if (frame->sample_rate == SAMPLE_RATE
        && frame->format == SAMPLE_FORMAT
        && frame->channel_layout == CHANNEL_LAYOUT) {
        AVFrame* dup = framePool_.get();
        if (!dup || av_frame_ref(dup, frame) < 0) {
            framePool_.put(dup);
            return nullptr;
        }
        return dup;
    }

    refreshSwr(frame);
    if (!swrCtx_) {
        return nullptr;
    }

    const int outSampleCount = av_rescale_rnd(frame->nb_samples, SAMPLE_RATE, frame->sample_rate, AV_ROUND_UP) + 256;
    AVFrame* dst = framePool_.get();
    if (!dst) {
        return nullptr;
    }

    dst->format = SAMPLE_FORMAT;
    dst->sample_rate = SAMPLE_RATE;
    dst->channel_layout = CHANNEL_LAYOUT;
    dst->nb_samples = outSampleCount;
    if (av_frame_get_buffer(dst, 0) < 0) {
        framePool_.put(dst);
        return nullptr;
    }

    const int converted = swr_convert(swrCtx_, dst->data, outSampleCount, (const uint8_t**)frame->extended_data, frame->nb_samples);
    if (converted <= 0) {
        framePool_.put(dst);
        return nullptr;
    }

    dst->nb_samples = converted;
    av_frame_copy_props(dst, frame);

    // 确保所需的属性正确，因为已经经过重采样
    dst->format = SAMPLE_FORMAT;
    dst->sample_rate = SAMPLE_RATE;
    dst->channel_layout = CHANNEL_LAYOUT;

    return dst;
}

void AudioDecodec::pushFrames(const std::vector<AVFrame*>& frames, double pts) {
    if (frames.empty()) {
        return;
    }

    double duration{};
    // 经过滤镜处理可能有多个音频帧，需要累加pts
    for (AVFrame* f : frames) {
        duration = (double)f->nb_samples / f->sample_rate * speed_;
        pushFrame(f, pts, duration);
        pts += duration;
        decodecTimer_ = pts;

        framePool_.put(f);
    }
}
