//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/audioDecodec.h>

bool AudioDecodec::openCodec(int streamIndex, AVStream* stream) {
    if (!Decodec::openCodec(streamIndex, stream)) {
        return false;
    }

    return true;
}

void AudioDecodec::run(std::stop_token st) {
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        av_log(nullptr, AV_LOG_FATAL, "Could not allocate audio frame");
        return;
    }

    double duration{};
    double pts{};

    while (!st.stop_requested()) {
        if (!decodeFrame(frame)) {
            if (st.stop_requested()) {
                break;
            }
            continue;
        }

        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d({ 1, frame->sample_rate });
        duration = av_q2d({ frame->nb_samples, frame->sample_rate });
        pushFrame(frame, decodecSerial_, pts, duration);
    }

    av_frame_free(&frame);
    av_log(nullptr, AV_LOG_INFO, "Finished decoding audio stream\n");
}

void AudioDecodec::cleanup() {
    Decodec::cleanup();
}