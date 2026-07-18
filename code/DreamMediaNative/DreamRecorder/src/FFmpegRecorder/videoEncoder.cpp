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

void VideoEncoder::addStream(AVCodecID codecID) {
    Encoder::addStream(codecID);

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
    codecCtx_->time_base = (AVRational){1, (int)param_.fps};
    codecCtx_->framerate = (AVRational){(int)param_.fps, 1};
    codecCtx_->pix_fmt = PIXEL_FORMAT;
    codecCtx_->codec_id = codecID;
    codecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

VideoFrameBuffer VideoEncoder::getVideoBuffer() {
    AVFrame* frame = framePool_.get();
    if (frame->format != (AVPixelFormat)param_.pixelFormat || frame->width != param_.width || frame->height != param_.height) {
        // 没有被分配过内存，分配内存设置参数
        frame->format = (AVPixelFormat)param_.pixelFormat;
        frame->width = param_.width;
        frame->height = param_.height;
        av_frame_get_buffer(frame, 32);
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
    frameQue_.push(frame);
    framePool_.put(frame);
}

void VideoEncoder::run(std::stop_token st) {
    Encoder::run(st);
    av_log(nullptr, AV_LOG_INFO, "VideoEncoder::run exit\n");
}

