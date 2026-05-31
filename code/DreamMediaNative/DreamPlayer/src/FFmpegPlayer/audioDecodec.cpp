//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/audioDecodec.h>
#include <DreamPlayer/structs.h>

namespace {
    constexpr int SAMPLE_RATE = 44100;
    constexpr int CHANNEL_COUNT = 2;
    constexpr AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;

    void updateAudioFrameSize(Dream::AudioFrame* audioFrame, int buffSize) {
        if (!audioFrame->data || buffSize != audioFrame->len) {
            delete[] audioFrame->data;
            audioFrame->data = new char[buffSize]{};
            audioFrame->len = buffSize;
        }
    }
}

bool AudioDecodec::openCodec(int streamIndex, AVStream* stream) {
    if (!Decodec::openCodec(streamIndex, stream)) {
        return false;
    }

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
        pushFrame(frame, decodecSerial_, pts, duration);
    }

    av_frame_free(&frame);
    av_log(nullptr, AV_LOG_INFO, "Finished decoding audio stream\n");
}

void AudioDecodec::getAudioFrame(Dream::AudioFrame* audioFrame) {
    audioFrame->isUpdated = false;

    // TODO: 同步 seek
    Frame ap{};
    do {
        std::optional<Frame> apOpt = frameQue_.pop();
        if (!apOpt) {
            return;
        }

        ap = *apOpt;
        if (ap.serial != decodecSerial_) {
            AVFrame* frame = ap.frame;
            av_frame_unref(frame);
            av_frame_free(&frame);
        }
    } while (ap.serial != decodecSerial_);

    AVFrame* frame = ap.frame;
    if (frame->sample_rate != SAMPLE_RATE || frame->channels != CHANNEL_COUNT || frame->format != SAMPLE_FORMAT) {
        swr_free(&swrCtx_);
        swrCtx_ = swr_alloc_set_opts(nullptr,
                                     AV_CH_LAYOUT_STEREO,
                                     SAMPLE_FORMAT, SAMPLE_RATE, frame->channel_layout,
                                     (AVSampleFormat)frame->format, frame->sample_rate,
                                     0, nullptr);
        if (!swrCtx_ || swr_init(swrCtx_) < 0) {
            av_log(nullptr, AV_LOG_ERROR, "swr_alloc_set_opts failed\n");
            swr_free(&swrCtx_);
            av_frame_unref(frame);
            av_frame_free(&frame);
            return;
        }

        const uint8_t** in = (const uint8_t**)frame->extended_data;
        // 简单推导：无论采样率怎么变，这段音频播放的物理时间长度是不变的，而time = nb_samples / sample_rate，分别带入双方变量即可
        const int outSampleCount = av_rescale_rnd(frame->nb_samples, SAMPLE_RATE, frame->sample_rate, AV_ROUND_UP) + 256;
        const int bufferSize = av_samples_get_buffer_size(nullptr, CHANNEL_COUNT, outSampleCount,
                                                                SAMPLE_FORMAT, 1);
        updateAudioFrameSize(audioFrame, bufferSize);
        const int converted = swr_convert(swrCtx_, (uint8_t**)&audioFrame->data, outSampleCount, in, frame->nb_samples);
        if (converted <= 0) {
            av_frame_unref(frame);
            av_frame_free(&frame);
            return;
        }
        // 因为转换的采样数一定不会比outSampleCount大，所以这里可以不重新分配内存
        audioFrame->len = av_samples_get_buffer_size(nullptr, CHANNEL_COUNT, converted, SAMPLE_FORMAT, 1);
    }
    else {
        const int bufferSize = av_samples_get_buffer_size(nullptr, frame->channels, frame->nb_samples, (AVSampleFormat)frame->format, 1);
        audioFrame->len = bufferSize;
        updateAudioFrameSize(audioFrame, bufferSize);
        memcpy(audioFrame->data, frame->data[0], bufferSize);
    }

    audioFrame->isUpdated = true;
    av_frame_unref(frame);
    av_frame_free(&frame);
}

void AudioDecodec::cleanup() {
    if (swrCtx_) {
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }

    Decodec::cleanup();
}
