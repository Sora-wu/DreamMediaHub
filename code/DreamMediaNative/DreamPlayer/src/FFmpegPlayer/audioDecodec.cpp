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

void AudioDecodec::run() {

}

void AudioDecodec::cleanup() {
    Decodec::cleanup();
}

bool AudioDecodec::decodeFrame(AVFrame* frame) {
    Packet packet = pktQue_.pop();

    return false;
}