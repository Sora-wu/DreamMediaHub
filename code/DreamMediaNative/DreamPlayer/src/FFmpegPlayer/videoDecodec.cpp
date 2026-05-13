//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/videoDecodec.h>
#include <FFmpegPlayer/ffinstance.h>

#include <cassert>

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
    assert(fmtCtx);

    AVRational frameRate = av_guess_frame_rate(fmtCtx, stream_, nullptr);

    while  (!isExit_) {

    }

    av_frame_free(&frame);
}

void VideoDecodec::cleanup() {
    Decodec::cleanup();
}
