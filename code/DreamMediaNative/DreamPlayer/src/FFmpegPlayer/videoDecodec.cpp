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

    void updateVideoFrameSize(Dream::VideoFrame* videoFrame, int height, const int linesize[3]) {
        const int planeHeights[3] = { height, height / 2, height / 2 };
        for (uint32_t i = 0; i < 3; ++i) {
            if (!linesize[i]) {
                continue;
            }

            const int bufSize = linesize[i] * planeHeights[i];
            if (bufSize == videoFrame->len[i] && videoFrame->data[i]) {
                continue;
            }

            if (videoFrame->data[i]) {
                delete[] videoFrame->data[i];
                videoFrame->data[i] = nullptr;
            }

            videoFrame->data[i] = new char[bufSize]{};
            videoFrame->len[i] = bufSize;
        }
    }
}

bool VideoDecodec::openCodec(int streamIndex, AVStream* stream) {
    if (!Decodec::openCodec(streamIndex, stream)) {
        return false;
    }



    return true;
}

uint32_t VideoDecodec::getWidth() const {
    return codecCtx_->width;
}

uint32_t VideoDecodec::getHeight() const {
    return codecCtx_->height;
}

void VideoDecodec::run(std::stop_token st) {
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

    while (!st.stop_requested()) {
        if (!decodeFrame(frame)) {
            if (st.stop_requested()) {
                break;
            }
            continue;
        }

        frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(fmtCtx, stream_, frame);
        duration = av_q2d(frameRate) > 0 ? av_q2d(av_inv_q(frameRate)) : 0.0;
        pts = frame->pts == AV_NOPTS_VALUE ? NAN : frame->pts * av_q2d(timeBase);
        pushFrame(frame, decodecSerial_, pts, duration);
    }

    av_frame_free(&frame);
    av_log(nullptr, AV_LOG_INFO, "Finished decoding video stream\n");
}

void VideoDecodec::getVideoFrame(Dream::VideoFrame* videoFrame) {
    // TODO: 同步
    std::optional<Frame> vpOpt = frameQue_.try_pop();
    if (!vpOpt) {
        return;
    }

    Frame vp = vpOpt.value();
    AVFrame* frame = vp.frame;
    const int width = frame->width;
    const int height = frame->height;

    videoFrame->frameWidth = width;
    videoFrame->frameHeight = height;
    videoFrame->picWidthHeightRatio = av_q2d(frame->sample_aspect_ratio);

    uint8_t* frameData[3]{};
    int frameLensize[3]{};
    for (uint32_t i = 0; i < 3; ++i) {
        frameData[i] = frame->data[i];
        frameLensize[i] = frame->linesize[i];
    }

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

        int bufSize = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1);
        if (!convertBuf_ || convertBufSize_ < bufSize) {
            av_freep(&convertBuf_);
            convertBuf_ = static_cast<uint8_t*>(av_malloc(bufSize));
            convertBufSize_ = bufSize;
        }

        uint8_t* outData[4]{};
        int outLinesize[4]{};
        sws_scale(swsCtx_, frame->data, frame->linesize, 0, height, outData, outLinesize);
        for (uint32_t i = 0; i < 3; ++i) {
            frameData[i] = outData[i];
            frameLensize[i] = outLinesize[i];
        }
    }

    updateVideoFrameSize(videoFrame, height, frameLensize);
    const int planeHeights[3] = { height, height / 2, height / 2 };
    for (uint32_t i = 0; i < 3; ++i) {
        const int stride = frameLensize[i];
        const int planeHeight = planeHeights[i];
        for (int row = 0; row < planeHeight; ++row) {
            memcpy(videoFrame->data[i] + row * stride, frameData[i] + row * stride, stride);
        }
        videoFrame->stride[i] = stride;
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