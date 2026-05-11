//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/FFmpegPlayer.h>
#include <FFmpegPlayer/ffcore.h>
#include <FFmpegPlayer/demuxer.h>
#include <FFmpegPlayer/videoDecodec.h>
#include <FFmpegPlayer/audioDecodec.h>

namespace {

}

FFmpegPlayer::FFmpegPlayer() {
    avdevice_register_all();
    avformat_network_init();

    demuxer_ = std::make_unique<Demuxer>();
    videoDecodec_ = std::make_unique<VideoDecodec>();
    audioDecodec_ = std::make_unique<AudioDecodec>();
}

FFmpegPlayer::~FFmpegPlayer() {
    cleanup();
}

bool FFmpegPlayer::open(const char* url) {
    cleanup();

    if (!demuxer_->open(url)) {
        return false;
    }

    std::vector<AVStream*> streams = demuxer_->getStreams();
    for (uint32_t i = 0; i < streams.size(); ++i) {
        AVStream* stream = streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (!videoDecodec_->openCodec(i, stream)) {
                return false;
            }
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (!audioDecodec_->openCodec(i, stream)) {
                return false;
            }
        }
    }

    return true;
}

void FFmpegPlayer::play() {
}

void FFmpegPlayer::pause() {
}

void FFmpegPlayer::stop() {
}

void FFmpegPlayer::seek(double position) {
}

uint64_t FFmpegPlayer::getDuration() const {
    return demuxer_->getDuration();
}

void FFmpegPlayer::setVolume(uint32_t volume) {
}

uint32_t FFmpegPlayer::getVolume() const {
    return volume_;
}

void FFmpegPlayer::setSpeed(double speed) {
}

void FFmpegPlayer::cleanup() {

}
