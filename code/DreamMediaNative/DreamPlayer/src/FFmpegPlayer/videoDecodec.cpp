//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/videoDecodec.h>
#include <FFmpegPlayer/ffinstance.h>
#include <DreamPlayer/structs.h>

#include <algorithm>

using namespace Dream;

namespace {
    constexpr double SYNC_THRESHOLD_MIN = 0.04;
    constexpr double SYNC_THRESHOLD_MAX = 0.1;
    constexpr double SYNC_FRAMEDUP_THRESHOLD = 0.1;
    constexpr double SYNC_THRESHOLD_MAX_DRIFT = 0.1;

    void updateVideoFrameSize(VideoFrame* videoFrame, int height, const int linesize[3]) {
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

    AVFormatContext* fmtCtx = FFINSTANCE()->getFormatContext();
    maxFrameDuration_ = (fmtCtx->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;
    return true;
}

void VideoDecodec::reopen() {
    resetSyncState();
    Decodec::reopen();
}

void VideoDecodec::startDecode() {
    start("video thread");
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
    AVRational timeBase = stream_->time_base;
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
        pushFrame(frame, pts, duration);
    }

    av_frame_free(&frame);
    av_log(nullptr, AV_LOG_INFO, "Finished decoding video stream\n");
}

void VideoDecodec::resetSyncState() {
    releaseFrame(heldFrame_);
    decodeTimerValid_ = false;
}

void VideoDecodec::getVideoFrame(VideoFrame* videoFrame) {
    for (;;) {
        if (!heldFrame_.frame) {
            Frame frame{};
            if (!acquireFrame(frame)) {
                return;
            }

            heldFrame_ = frame;
            decodecTimer_ = CURRENT_TIME;
            decodeTimerValid_ = true;
            if (!isnan(heldFrame_.pts)) {
                FFINSTANCE()->setVideoClock(heldFrame_.pts, heldFrame_.serial, decodecTimer_);
            }
            fillData(heldFrame_.frame, videoFrame);
            return;
        }

        double delay = 0;
        frameQue_.visit_front([&](const Frame& next) {
            delay = calcDelay(getFrameDuration(heldFrame_, next));
        });

        const double time = CURRENT_TIME;
        if (!decodeTimerValid_) {
            decodecTimer_ = time;
            decodeTimerValid_ = true;
        }

        // 没有到达新帧的播放时间，使用老帧
        if (time < decodecTimer_ + delay || isPause_) {
            // fillData(heldFrame_.frame, videoFrame);
            return;
        }

        decodecTimer_ += delay;
        // 如果和系统时间差距太大，就纠正为系统时间
        if (delay > 0 && time - decodecTimer_ > SYNC_THRESHOLD_MAX_DRIFT) {
            decodecTimer_ = time;
        }

        while (shouldDropLateFrame(time)) {
            std::optional<Frame> dropOpt = frameQue_.try_pop();
            if (!dropOpt) {
                break;
            }

            if (dropOpt->serial != decodecSerial_) {
                decodeTimerValid_ = false;
            }

            releaseFrame(*dropOpt);
        }

        // 当前帧已经播放完毕，释放
        releaseFrame(heldFrame_);
        Frame frame{};
        if (!acquireFrame(frame)) {
            return;
        }

        heldFrame_ = frame;
        if (!isnan(heldFrame_.pts)) {
            FFINSTANCE()->setVideoClock(heldFrame_.pts, heldFrame_.serial, time);
        }
        fillData(heldFrame_.frame, videoFrame);
        return;
    }
}

void VideoDecodec::cleanup() {
    resetSyncState();
    Decodec::cleanup();

    sws_freeContext(swsCtx_);
}

double VideoDecodec::getFrameDuration(const Frame& frame, const Frame& nextFrame) const {
    if (frame.serial != nextFrame.serial) {
        return 0;
    }

    const double duration = nextFrame.pts - frame.pts;
    if (isnan(duration) || duration <= 0 || duration > maxFrameDuration_) {
        return frame.duration;
    }

    return duration;
}

double VideoDecodec::calcDelay(double duration) const {
    if (FFINSTANCE()->getSyncType() == SyncType::VideoMaster) {
        return duration;
    }

    double delay = duration;
    const double diff = FFINSTANCE()->getVideoClock() - FFINSTANCE()->getMasterClock();
    const double syncThreshold = std::clamp(delay, SYNC_THRESHOLD_MIN, SYNC_THRESHOLD_MAX);
    if (!isnan(diff) && std::fabs(diff) < maxFrameDuration_) {
        if (diff <= -syncThreshold) {
            delay = std::max(0.0, delay + diff);
        }
        else if (diff >= syncThreshold && delay > SYNC_FRAMEDUP_THRESHOLD) {
            delay += diff;
        }
        else if (diff >= syncThreshold) {
            delay *= 2;
        }
    }

    return delay;
}

bool VideoDecodec::shouldDropLateFrame(double time) const {
    if (FFINSTANCE()->getSyncType() == SyncType::VideoMaster || frameQue_.size() <= 1) {
        return false;
    }

    bool drop = false;
    frameQue_.visit_front([&](const Frame& next) {
        const double duration = getFrameDuration(heldFrame_, next);
        drop = duration > 0 && time > decodecTimer_ + duration;
    });

    return drop;
}

void VideoDecodec::fillData(AVFrame* frame, VideoFrame* videoFrame) {
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
}