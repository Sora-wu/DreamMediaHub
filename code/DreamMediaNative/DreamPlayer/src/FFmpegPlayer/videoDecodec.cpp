//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/videoDecodec.h>

bool VideoDecodec::openCodec(uint32_t streamIndex, AVStream* stream) {
    if (!Decodec::openCodec(streamIndex, stream)) {
        return false;
    }

    return true;
}

void VideoDecodec::handle(void* data) {
    Decodec::handle(data);
}

void VideoDecodec::run() {

}

void VideoDecodec::cleanup() {
    Decodec::cleanup();
}
