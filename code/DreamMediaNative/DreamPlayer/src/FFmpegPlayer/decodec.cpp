//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/decodec.h>
#include <FFmpegPlayer/ffinstance.h>

namespace {
    bool padding = false;
    AVPacket* paddingPkt = nullptr;
}

bool Decodec::openCodec(int streamIndex, AVStream* stream) {
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

    int res = avcodec_parameters_to_context(codecCtx_, stream->codecpar);
    if (res < 0) {
        av_log(nullptr, AV_LOG_FATAL, "avcodec_parameters_to_context fail!");
    }

    res = avcodec_open2(codecCtx_, codec, nullptr);
    if (res < 0) {
        av_log(nullptr, AV_LOG_FATAL, "avcodec_open2 fail! codec_id: %d", stream->codecpar->codec_id);
        return false;
    }

    stream_ =  stream;
    streamIndex_ = streamIndex;

    return true;
}

void Decodec::reopen() {
    pktQue_.reopen();
    while (std::optional<Frame> frameOpt = frameQue_.try_pop()) {
        av_frame_unref(frameOpt->frame);
        av_frame_free(&frameOpt->frame);
    }
    frameQue_.reopen();

    decodecSerial_ = 0;
}

void Decodec::handle(void* data) {
    pktQue_.push((AVPacket*)data);
}

void Decodec::shutdown() {
    pktQue_.close();
    frameQue_.close();
}

void Decodec::cleanup() {
    if (codecCtx_) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }

    streamIndex_ = -1;
}


bool Decodec::decodeFrame(AVFrame* frame) {
    // 先Frame后Packet，两个操作错开，避免更多的AVERROR(EAGAIN)
    AVPacket* prePkt = nullptr;
    int res = AVERROR(EAGAIN);

    while (true) {
        if (pktQue_.getSerial() == decodecSerial_) {
            do {
                res = avcodec_receive_frame(codecCtx_, frame);
                if (res >= 0) {
                    frame->pts = frame->best_effort_timestamp;
                    return true;
                }
                if (res == AVERROR_EOF) {
                    avcodec_flush_buffers(codecCtx_);
                    return false;
                }
            } while (res != AVERROR(EAGAIN));
        }

        do {
            if (padding) {
                padding = false;
                av_packet_move_ref(prePkt, paddingPkt);
            }
            else {
                std::optional<Packet> packetOpt = pktQue_.pop();
                if (!packetOpt) {
                    return false;
                }

                auto&[pkt, serial] = *packetOpt;
                decodecSerial_ = serial;

                prePkt = &pkt;
                if (pktQue_.getSerial() != decodecSerial_) {
                    av_packet_unref(&pkt);
                }
            }
        } while (pktQue_.getSerial() != decodecSerial_);

        if (prePkt->data == FFINSTANCE()->getFlushPkt()->data) {
            // 是flush包
            avcodec_flush_buffers(codecCtx_);
            if (onSeekCallback_) {
                onSeekCallback_();
            }
            continue;
        }

        if (AVERROR(EAGAIN) == avcodec_send_packet(codecCtx_, prePkt)) {
            padding = true;
            av_packet_move_ref(paddingPkt, prePkt);
        }

        av_packet_unref(prePkt);
    }
}

void Decodec::pushFrame(AVFrame* frame, double pts, double duration) {
    AVFrame* dup = av_frame_alloc();
    if (!dup || av_frame_ref(dup, frame) < 0) {
        av_frame_free(&dup);
        return;
    }

    Frame vp{};
    vp.frame = dup;
    vp.serial = decodecSerial_;
    vp.pts = pts;
    vp.duration = duration;

    frameQue_.push(vp);
}

bool Decodec::acquireFrame(Frame& out) {
    while (true) {
        std::optional<Frame> frameOpt = frameQue_.pop();
        if (!frameOpt) {
            return false;
        }

        if (frameOpt->serial != decodecSerial_) {
            releaseFrame(*frameOpt);
            decodeTimerValid_ = false;
            continue;
        }

        out = *frameOpt;
        return true;
    }
}

void Decodec::releaseFrame(Frame& frame) {
    if (!frame.frame) {
        return;
    }

    av_frame_unref(frame.frame);
    av_frame_free(&frame.frame);
    frame = {};
}
