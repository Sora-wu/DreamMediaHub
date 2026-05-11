//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/audioDecodec.h>

bool AudioDecodec::openCodec(uint32_t streamIndex, AVStream* stream) {
    if (!Decodec::openCodec(streamIndex, stream)) {
        return false;
    }

    return true;
}

void AudioDecodec::handle(void* data) {
    Decodec::handle(data);
}

void AudioDecodec::run() {

}

void AudioDecodec::cleanup() {
    Decodec::cleanup();
}
