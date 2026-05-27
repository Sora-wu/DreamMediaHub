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
    av_log(nullptr, AV_LOG_INFO, "Finished decoding audio stream\n");
}

void AudioDecodec::cleanup() {
    Decodec::cleanup();
}

bool AudioDecodec::decodeFrame(AVFrame* frame) {
    Packet packet = pktQue_.pop();

    return false;
}