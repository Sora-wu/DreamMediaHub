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
    constexpr std::size_t FRAME_VIDEO_SIZE = 3;
    constexpr std::size_t FRAME_AUDIO_SIZE = 9;
}

FFmpegPlayer::FFmpegPlayer() {
    avdevice_register_all();
    avformat_network_init();

    demuxer_ = std::make_unique<Demuxer>();
    demuxer_->setNextHandler(this);

    videoDecodec_ = std::make_unique<VideoDecodec>(FRAME_VIDEO_SIZE);
    audioDecodec_ = std::make_unique<AudioDecodec>(FRAME_AUDIO_SIZE);
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
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && !videoDecodec_->isOpen()) {
            if (!videoDecodec_->openCodec(i, stream)) {
                return false;
            }
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && !audioDecodec_->isOpen()) {
            if (!audioDecodec_->openCodec(i, stream)) {
                return false;
            }
        }
    }

    demuxer_->start();
    videoDecodec_->start();
    // audioDecodec_->start();

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
    speed_ = speed;
}

void FFmpegPlayer::getVideoFrame(Dream::VideoFrame* videoFrame) {
    videoDecodec_->getVideoFrame(videoFrame);
}

void FFmpegPlayer::handle(void* data) {
    AVPacket* pkt = (AVPacket*)data;

    if (pkt->stream_index == videoDecodec_->getStreamIndex()) {
        videoDecodec_->handle(pkt);
    }
    else if (pkt->stream_index == audioDecodec_->getStreamIndex()) {
        audioDecodec_->handle(pkt);
    }
}


void FFmpegPlayer::cleanup() {

}
