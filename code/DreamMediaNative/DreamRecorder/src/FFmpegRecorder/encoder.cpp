//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegRecorder/encoder.h>
#include <FFmpegRecorder/ffcore.h>
#include <FFmpegRecorder/ffinstance.h>

namespace {
    bool padding = false;
    AVFrame* paddingFrame = nullptr;
}

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

void Encoder::run(std::stop_token st) {
    AVPacket* pkt = av_packet_alloc();

    while (!st.stop_requested()) {
        if (!encodePacket(pkt)) {
            if (st.stop_requested()) {
                break;
            }
        }

        AVPacket* muxPkt = av_packet_alloc();
        av_packet_ref(muxPkt, pkt);
        handle(muxPkt);
    }

    av_packet_free(&pkt);
}

void Encoder::close() {
    frameQue_.close();
}

bool Encoder::encodePacket(AVPacket* pkt) {
    int res = AVERROR(EAGAIN);

    while (true) {
        do {
            res = avcodec_receive_packet(codecCtx_, pkt);
            if (res == AVERROR_EOF) {
                return false;
            }
            // 编码成功
            if (res == 0) {
                av_packet_rescale_ts(pkt, codecCtx_->time_base, stream_->time_base);
                pkt->stream_index = stream_->index;
                // av_log(nullptr, AV_LOG_INFO, "pkt: %p, pts: %ld size: %d\n", pkt, pkt->pts, pkt->size);
                return true;
            }
        } while (res != AVERROR(EAGAIN));

        AVFrame* frame = nullptr;
        // 先查看是否有pad
        if (padding) {
            av_frame_move_ref(frame, paddingFrame);
            padding = false;
        }
        else {
            std::optional<AVFrame*> frameOpt = frameQue_.pop();
            if (!frameOpt) {
                return false;
            }
            frame = *frameOpt;
            frame->pts = pts_++;
        }
        res = avcodec_send_frame(codecCtx_, frame);
        // av_log(nullptr, AV_LOG_INFO, "frame: %p, pts: %d width: %d\n", frame, frame->pts, frame->width);
        if (res == AVERROR(EAGAIN)) {
            padding = true;
            av_frame_move_ref(paddingFrame,  frame);
        }

        framePool_.put(frame);
    }
}