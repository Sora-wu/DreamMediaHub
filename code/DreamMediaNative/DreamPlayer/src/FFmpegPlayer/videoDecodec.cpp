//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/videoDecodec.h>
#include <FFmpegPlayer/ffinstance.h>
#include <DreamPlayer/structs.h>

namespace {
    bool padding = false;
    AVPacket* paddingPkt = nullptr;

    void updateVideoFrameSize(Dream::VideoFrame* videoFrame, const int linesize[3]) {
        for (uint32_t i = 0; i < 3; ++i) {
            if (linesize[i] != videoFrame->len[i]) {
                if (videoFrame->data[i]) {
                    delete[] videoFrame->data[i];
                    videoFrame->data[i] = nullptr;
                }

                videoFrame->data[i] = new char[linesize[i]]{};
                videoFrame->len[i] = linesize[i];
            }
        }
    }
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

void VideoDecodec::getVideoFrame(Dream::VideoFrame* videoFrame) {
    // TODO: 同步
    std::optional<Frame> vpOpt = frameQue_.try_pop();
    if (!vpOpt.has_value()) {
        return;
    }

    Frame vp = vpOpt.value();
    AVFrame* frame = vp.frame;
    const int width = frame->width;
    const int height = frame->height;

    videoFrame->frameWidth = frame->width;
    videoFrame->frameHeight = frame->height;

    uint8_t* frameData[3]{};
    int frameDataLen[3]{};
    frameData[0] = frame->data[0];
    frameData[1] = frame->data[1];
    frameData[2] = frame->data[2];
    frameDataLen[0] = frame->linesize[0];
    frameDataLen[1] = frame->linesize[1];
    frameDataLen[2] = frame->linesize[2];

    if (frame->format != AV_PIX_FMT_YUV420P) {
        swsCtx_ = sws_getCachedContext(swsCtx_,
            width, height, (AVPixelFormat)frame->format,
            width, height, AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!swsCtx_) {
            av_frame_unref(frame);
            av_frame_free(&frame);
            return;
        }

        int bufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 32);
        if (!convertBuf_ || convertBufSize_ < bufSize) {
            av_freep(&convertBuf_);
            convertBuf_ = static_cast<uint8_t*>(av_malloc(bufSize));
            convertBufSize_ = bufSize;
        }

        uint8_t* outData[4]{};
        int outLinesize[4]{};
        sws_scale(swsCtx_, frame->data, frame->linesize, 0, height, outData, outLinesize);
        frameData[0] = outData[0];
        frameData[1] = outData[1];
        frameData[2] = outData[2];
        frameDataLen[0] = outLinesize[0];
        frameDataLen[1] = outLinesize[1];
        frameDataLen[2] = outLinesize[2];
    }

    updateVideoFrameSize(videoFrame, frameDataLen);
    for (uint32_t i = 0; i < 3; ++i) {
        memcpy(videoFrame->data[i], frameData[i], frameDataLen[i]);
        videoFrame->len[i] = frameDataLen[i];
    }

    av_frame_unref(frame);
    av_frame_free(&frame);
}

void VideoDecodec::cleanup() {
    Decodec::cleanup();

    sws_freeContext(swsCtx_);
    av_freep(&convertBuf_);
    convertBufSize_ = 0;
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
    AVFrame* dup = av_frame_alloc();
    if (!dup || av_frame_ref(dup, frame) < 0) {
        av_frame_free(&dup);
        return;
    }

    Frame vp{};
    vp.frame = dup;
    vp.serial = serial;
    vp.pts = pts;
    vp.duration = duration;

    frameQue_.push(vp);
}