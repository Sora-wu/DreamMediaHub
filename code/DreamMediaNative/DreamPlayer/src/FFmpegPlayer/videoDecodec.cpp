//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/videoDecodec.h>
#include <FFmpegPlayer/ffinstance.h>
#include <DreamPlayer/structs.h>

namespace {
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

VideoDecodec::~VideoDecodec() {
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
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
    videoFrame->frameWidth = 0;
    videoFrame->frameHeight = 0;

    // TODO: 同步 seek
    Frame vp{};
    do {
        std::optional<Frame> vpOpt = frameQue_.pop();
        if (!vpOpt) {
            return;
        }

        vp = *vpOpt;
        if (vp.serial != decodecSerial_) {
            AVFrame* frame = vp.frame;
            av_frame_unref(frame);
            av_frame_free(&frame);
        }
    } while (vp.serial != decodecSerial_);

    AVFrame* frame = vp.frame;
    const int width = frame->width;
    const int height = frame->height;

    videoFrame->frameWidth = width;
    videoFrame->frameHeight = height;
    videoFrame->picWidthHeightRatio = frame->sample_aspect_ratio.den == 0 ? 0 : av_q2d(frame->sample_aspect_ratio);

    if (frame->format != AV_PIX_FMT_YUV420P) {
        swsCtx_ = sws_getCachedContext(swsCtx_,
            width, height, (AVPixelFormat)frame->format,
            width, height, AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!swsCtx_) {
            av_log(nullptr, AV_LOG_ERROR, "Cannot initialize the conversion context\n");
            av_frame_unref(frame);
            av_frame_free(&frame);
            return;
        }

        int outLinesize[4]{};
        if (av_image_fill_linesizes(outLinesize, AV_PIX_FMT_YUV420P, width) < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Cannot fill linesize\n");
            av_frame_unref(frame);
            av_frame_free(&frame);
            return;
        }
        updateVideoFrameSize(videoFrame, height, outLinesize);
        sws_scale(swsCtx_, frame->data, frame->linesize, 0, height, (uint8_t* const*)videoFrame->data, videoFrame->len);
    }
    else {
        updateVideoFrameSize(videoFrame, height, frame->linesize);
        const int planeHeights[3] = { height, height / 2, height / 2 };
        for (uint32_t i = 0; i < 3; ++i) {
            const int stride = frame->linesize[i];
            const int planeHeight = planeHeights[i];
            for (int row = 0; row < planeHeight; ++row) {
                memcpy(videoFrame->data[i] + row * stride, frame->data[i] + row * stride, stride);
            }
            videoFrame->stride[i] = stride;
        }
    }

    av_frame_unref(frame);
    av_frame_free(&frame);
}

void VideoDecodec::cleanup() {
    Decodec::cleanup();

    sws_freeContext(swsCtx_);
}