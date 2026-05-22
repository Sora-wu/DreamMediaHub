//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/videoDecodec.h>
#include <FFmpegPlayer/ffinstance.h>

namespace {
    bool padding = false;
    AVPacket* paddingPkt = nullptr;
}

bool VideoDecodec::openCodec(int streamIndex, AVStream* stream) {
    if (!Decodec::openCodec(streamIndex, stream)) {
        return false;
    }

    return true;
}

void VideoDecodec::run() {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        av_log(nullptr, AV_LOG_FATAL, "Could not allocate video frame");
        return;
    }

    AVFormatContext* fmtCtx = FFINSTANCE()->getFormatContext();
    AVRational timeBase = codecCtx_->time_base;
    AVRational frameRate = av_guess_frame_rate(fmtCtx, stream_, nullptr);
    double duration{};
    double pts{};

    while  (!isExit_) {
        if (!decodeFrame(frame)) {
            continue;
        }
        
        duration = av_q2d(frameRate) > 0 ? av_q2d(av_inv_q(frameRate)) : 0.0;
        pts = frame->pts == AV_NOPTS_VALUE ? NAN : frame->pts * av_q2d(timeBase);
        pushFrame(frame, decodecSerial_, pts, duration);
    }

    av_frame_free(&frame);
}

Dream::VideoFrame VideoDecodec::getVideoFrame() {
    // TODO: 同步
    Frame vp = frameQue_.pop();
    AVFrame* frame = vp.frame;

    uint8_t* data[3]{};
    int len[3]{};

    Dream::VideoFrame ret{};
    ret.frameWidth = frame->width;
    ret.frameHeight = frame->height;

    uint8_t* frameData[3]{};
    int frameDataLen[3]{};
    frameData[0] = frame->data[0];
    frameData[1] = frame->data[1];
    frameData[2] = frame->data[2];
    frameDataLen[0] = frame->linesize[0];
    frameDataLen[1] = frame->linesize[1];
    frameDataLen[2] = frame->linesize[2];

    if (frame->format != AV_PIX_FMT_YUV420P) {
        int width = frame->width;
        int height = frame->height;
        swsCtx_ = sws_getCachedContext(swsCtx_, 
            width, height, (AVPixelFormat)frame->format, 
            width, height, AV_PIX_FMT_YUV420P, 
            SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!swsCtx_) {
            av_log(nullptr, AV_LOG_FATAL, "Cannot initialize the conversion context");
            return ret;
        }

        sws_scale(swsCtx_, frame->data, frame->linesize, 0, height, data, len);
        frameData[0] = data[0];
        frameData[1] = data[1];
        frameData[2] = data[2];
        frameDataLen[0] = len[0];
        frameDataLen[1] = len[1];
        frameDataLen[2] = len[2];
    }

    ret.data[0] = (char*)frameData[0];
    ret.data[1] = (char*)frameData[1];
    ret.data[2] = (char*)frameData[2];
    ret.len[0] = frameDataLen[0];
    ret.len[1] = frameDataLen[1];
    ret.len[2] = frameDataLen[2];

    return ret;
}

void VideoDecodec::cleanup() {
    Decodec::cleanup();

    sws_freeContext(swsCtx_);
}

bool VideoDecodec::decodeFrame(AVFrame* frame) {
    // TODO: 如果要退出
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
                Packet packet = pktQue_.pop();
                auto&[pkt, serial] = packet;
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
            continue;
        }

        if (AVERROR(EAGAIN) == avcodec_send_packet(codecCtx_, prePkt)) {
            padding = true;
            av_packet_move_ref(paddingPkt, prePkt);
        }

        av_packet_unref(prePkt);
    }
}

void VideoDecodec::pushFrame(AVFrame* frame, uint32_t serial, double pts, double duration) {
    Frame vp{};

    vp.frame = frame;
    vp.serial = serial;
    vp.pts = pts;
    vp.duration = duration;

    av_frame_move_ref(vp.frame, frame);
    frameQue_.push(vp);
}