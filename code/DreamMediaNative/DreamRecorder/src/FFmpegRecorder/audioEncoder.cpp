//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegRecorder/audioEncoder.h>
#include <FFmpegRecorder/ffcore.h>
#include <DreamRecorder/DreamRecorderContext.h>

using namespace Dream;

namespace {
    constexpr AVSampleFormat SAMPLER_FORMAT = AV_SAMPLE_FMT_FLTP;
}

AudioEncoder::~AudioEncoder() {
    if (swrCtx_) {
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }

    if (audioFifo_) {
        av_audio_fifo_free(audioFifo_);
        audioFifo_ = nullptr;
    }

    if (converted_[0]) {
        av_freep(&converted_[0]);
    }
}

void AudioEncoder::addStream(AVCodecID codecID) {
    Encoder::addStream(codecID);
    stream_->time_base = AVRational{1, static_cast<int32_t>(param_.sampleRate)};

    swrCtx_ = swr_alloc_set_opts(nullptr,
        codecCtx_->channel_layout, SAMPLER_FORMAT, codecCtx_->sample_rate,
        codecCtx_->channel_layout, (AVSampleFormat)param_.sampleFormat, codecCtx_->sample_rate,
        0, nullptr);
    swr_init(swrCtx_);

    audioFifo_ = av_audio_fifo_alloc(codecCtx_->sample_fmt, codecCtx_->channels, 1);

    int outLinesize = 0;
    convertedSampleCount_ = DreamRecorderContext::AUDIO_BUFFER_SIZE / (param_.channels * av_get_bytes_per_sample((AVSampleFormat)param_.sampleFormat));
    av_samples_alloc(converted_, &outLinesize, codecCtx_->channels, convertedSampleCount_, SAMPLER_FORMAT, 0);
}

void AudioEncoder::initCodecContext(AVCodecID codecID) {
    codecCtx_->bit_rate = param_.bitRate;
    codecCtx_->sample_rate = param_.sampleRate;
    codecCtx_->sample_fmt = SAMPLER_FORMAT;
    codecCtx_->channels = param_.channels;
    codecCtx_->channel_layout = av_get_default_channel_layout(param_.channels);
    codecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void AudioEncoder::run(std::stop_token st) {
    Encoder::run(st);
    av_log(nullptr, AV_LOG_INFO, "AudioEncoder::run exit\n");
}

void AudioEncoder::pushData(const char* data, uint32_t size, uint64_t pts) {
    const int inSampleCount = size / (param_.channels * av_get_bytes_per_sample((AVSampleFormat)param_.sampleFormat));
    // 如果计算出的采样数小于等于0，则直接返回
    if (inSampleCount <= 0) {
        return;
    }

    uint8_t* inBuf[1] = { (uint8_t*)data };
    const int convertedSampleCount = swr_convert(swrCtx_,
        converted_, convertedSampleCount_,
        (const uint8_t**)inBuf, inSampleCount);
    if (convertedSampleCount > 0) {
        if (av_audio_fifo_realloc(audioFifo_, av_audio_fifo_size(audioFifo_) + convertedSampleCount) != 0) {
            av_log(nullptr, AV_LOG_ERROR, "AudioEncoder::pushData av_audio_fifo_realloc error\n");
            ::exit(1);
        }
        av_audio_fifo_write(audioFifo_, (void**)converted_, convertedSampleCount);
    }

    const int frameSize = codecCtx_->frame_size;
    while (av_audio_fifo_size(audioFifo_) > frameSize) {
        AVFrame* frame = framePool_.get();
        if (frame->channels != param_.channels || frame->sample_rate != param_.sampleRate || frame->format != SAMPLER_FORMAT
                || frame->channel_layout != codecCtx_->channel_layout || frame->nb_samples != frameSize) {
            frame->channels = param_.channels;
            frame->sample_rate = param_.sampleRate;
            frame->format = SAMPLER_FORMAT;
            frame->channel_layout = codecCtx_->channel_layout;
            frame->nb_samples = frameSize;
            av_frame_get_buffer(frame, 0);
        }
        else {
            av_frame_make_writable(frame);
        }

        av_audio_fifo_read(audioFifo_, (void**)frame->data, frameSize);
        // 如果是第一次赋值，取pts换时间基
        if (currentPts_ == AV_NOPTS_VALUE) {
            currentPts_ = av_rescale_q(pts, AV_TIME_BASE_Q, codecCtx_->time_base);
        }
        frame->pts = currentPts_;
        // time_base uses the sample rate, so the sample count is also the PTS increment.
        currentPts_ += frameSize;
        frameQue_.push(frame);
    }
}
