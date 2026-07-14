//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegRecorder/encoder.h>
#include <FFmpegRecorder/ffcore.h>
#include <FFmpegRecorder/ffinstance.h>

void Encoder::addStream(AVCodecID codecID) {
    AVFormatContext* fmtCtx = FFINSTANCE()->getFormatContext();
    AVCodec* codec = avcodec_find_encoder(codecID);
    if (!codec) {
        av_log(nullptr, AV_LOG_ERROR, "avcodec_find_encoder %d fail!\n", codecID);
        return;
    }

    codecCtx_ = avcodec_alloc_context3(codec);
    if (!codecCtx_) {
        av_log(nullptr, AV_LOG_ERROR, "avcodec_alloc_context3 fail!\n");
        return;
    }

    initCodecContext(codecID);

    int res = avcodec_open2(codecCtx_, codec, nullptr);
    if (res < 0) {
        av_log(nullptr, AV_LOG_ERROR, "avcodec_open2 fail!\n");
        return;
    }

    stream_ = avformat_new_stream(fmtCtx, codec);
    if (!stream_) {
        av_log(nullptr, AV_LOG_ERROR, "avformat_new_stream fail!\n");
        return;
    }

    stream_->codecpar->codec_tag = 0;
    res = avcodec_parameters_from_context(stream_->codecpar, codecCtx_);
    if (res < 0) {
        av_log(nullptr, AV_LOG_ERROR, "avcodec_parameters_from_context fail!\n");
        return;
    }

    av_dump_format(fmtCtx, 0, nullptr, true);
}
