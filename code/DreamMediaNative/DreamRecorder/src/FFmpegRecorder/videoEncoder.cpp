//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegRecorder/videoEncoder.h>
#include <FFmpegRecorder/ffcore.h>

namespace {
    constexpr uint32_t GOP_SIZE = 50;
    constexpr AVPixelFormat PIXEL_FORMAT = AV_PIX_FMT_YUV420P;

}

using namespace Dream;

VideoEncoder::~VideoEncoder() {
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }
}

void VideoEncoder::addStream(AVCodecID codecID) {
    Encoder::addStream(codecID);
    stream_->time_base = (AVRational){ 1, 90000 };

    swsCtx_ = sws_getCachedContext(swsCtx_,
        param_.width, param_.height, (AVPixelFormat)param_.pixelFormat,
        param_.width, param_.height, PIXEL_FORMAT,
        SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsCtx_) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot initialize the sws context\n");
        return;
    }
}

void VideoEncoder::initCodecContext(AVCodecID codecID) {
    codecCtx_->bit_rate = param_.bitRate;
    codecCtx_->width = param_.width;
    codecCtx_->height = param_.height;
    codecCtx_->max_b_frames = 0;
    codecCtx_->gop_size = GOP_SIZE;
    codecCtx_->time_base = (AVRational){ 1, 1000 }; // 毫秒
    codecCtx_->framerate = (AVRational){(int)param_.fps, 1};
    codecCtx_->pix_fmt = PIXEL_FORMAT;
    codecCtx_->codec_id = codecID;
    codecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

VideoFrameBuffer VideoEncoder::getVideoBuffer() {
    AVFrame* frame = userFramePool_.get();
    if (frame->format != (AVPixelFormat)param_.pixelFormat || frame->width != param_.width || frame->height != param_.height) {
        // 如果不一致，先清掉内存，重新申请
        av_frame_unref(frame);

        frame->format = (AVPixelFormat)param_.pixelFormat;
        frame->width = param_.width;
        frame->height = param_.height;
        av_frame_get_buffer(frame, 32);
    }
    else {
        // 如果一致，仅需要保证当前帧可用
        av_frame_make_writable(frame);
    }

    VideoFrameBuffer buffer{};
    buffer.data = (char*)frame->data[0];
    buffer.size = frame->linesize[0] * frame->height;
    buffer.stride = frame->linesize[0];
    buffer.opaque = frame;
    return buffer;
}

void VideoEncoder::commitVideoBuffer(const VideoFrameBuffer& buffer, uint64_t pts) {
    AVFrame* frame = (AVFrame*)buffer.opaque;
    frame->pts = pts;

    // 拿到数据之后，转为PIXEL_FORMAT
    AVFrame* convertedFrame = framePool_.get();
    if (convertedFrame->format == PIXEL_FORMAT && convertedFrame->width == frame->width && convertedFrame->height == frame->height) {
        av_frame_make_writable(convertedFrame);
    }
    else {
        // 如果不一致，先清掉内存，重新申请
        av_frame_unref(convertedFrame);

        convertedFrame->format = PIXEL_FORMAT;
        convertedFrame->width = frame->width;
        convertedFrame->height = frame->height;
        av_frame_get_buffer(convertedFrame, 32);
    }

    sws_scale(swsCtx_, frame->data, frame->linesize,
        0, frame->height, convertedFrame->data, convertedFrame->linesize);
    convertedFrame->pts = av_rescale_q(pts, AV_TIME_BASE_Q, codecCtx_->time_base);
    frameQue_.push(convertedFrame);
    // 前面的老帧，回收
    userFramePool_.put(frame);
}

void VideoEncoder::run(std::stop_token st) {
    Encoder::run(st);
    av_log(nullptr, AV_LOG_INFO, "VideoEncoder::run exit\n");
}

