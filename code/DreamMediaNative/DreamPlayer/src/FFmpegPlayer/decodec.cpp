//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/decodec.h>

bool Decodec::openCodec(uint32_t streamIndex, AVStream* stream) {
    AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        av_log(nullptr, AV_LOG_FATAL, "avcodec_find_decoder fail!");
        return false;
    }

    codecCtx_ = avcodec_alloc_context3(codec);
    if (!codecCtx_) {
        av_log(nullptr, AV_LOG_FATAL, "avcodec_alloc_context3 fail!");
        return false;
    }

    avcodec_parameters_to_context(codecCtx_, stream->codecpar);
    int res = avcodec_open2(codecCtx_, codec, nullptr);
    if (res < 0) {
        av_log(nullptr, AV_LOG_FATAL, "avcodec_open2 fail! codec_id: %d", stream->codecpar->codec_id);
        return false;
    }

    streamIndex_ = streamIndex;
    timeBase_ = stream->time_base;

    return true;
}

void Decodec::cleanup() {
    if (codecCtx_) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }
}
