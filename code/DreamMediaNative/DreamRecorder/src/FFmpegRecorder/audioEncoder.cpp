//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegRecorder/audioEncoder.h>
#include <FFmpegRecorder/ffcore.h>

namespace {
    constexpr AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_FLTP;
}

void AudioEncoder::run(std::stop_token st) {
}

void AudioEncoder::initCodecContext(AVCodecID codecID) {
    codecCtx_->bit_rate = param_.bitRate;
    codecCtx_->sample_rate = param_.sampleRate;
    codecCtx_->sample_fmt = SAMPLE_FORMAT;
    codecCtx_->channels = param_.channels;
    codecCtx_->channel_layout = av_get_default_channel_layout(param_.channels);
    codecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}
