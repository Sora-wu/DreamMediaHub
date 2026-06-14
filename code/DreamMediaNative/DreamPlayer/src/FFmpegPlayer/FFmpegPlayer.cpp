//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/FFmpegPlayer.h>
#include <FFmpegPlayer/ffcore.h>
#include <FFmpegPlayer/demuxer.h>
#include <FFmpegPlayer/videoDecodec.h>
#include <FFmpegPlayer/audioDecodec.h>
#include <FFmpegPlayer/ffinstance.h>

#include <algorithm>

using namespace Dream;

namespace {
    constexpr std::size_t FRAME_VIDEO_SIZE = 3;
    constexpr std::size_t FRAME_AUDIO_SIZE = 32;
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
        }
        else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && !audioDecodec_->isOpen()) {
            if (!audioDecodec_->openCodec(i, stream)) {
                return false;
            }
        }
    }

    if (!audioDecodec_->isOpen()) {
        FFINSTANCE()->setSyncType(SyncType::VideoMaster);
    }

    videoDecodec_->reopen();
    audioDecodec_->reopen();

    demuxer_->start();
    videoDecodec_->start();
    audioDecodec_->start();

    uint32_t* videoPktQueSerial = &videoDecodec_->getSerial();
    uint32_t* audioPktQueSerial = &audioDecodec_->getSerial();

    clock_.initClock(videoPktQueSerial, audioPktQueSerial);
    videoDecodec_->setMediaClock(&clock_);
    audioDecodec_->setMediaClock(&clock_);

    resetPlaybackState();

    return true;
}

void FFmpegPlayer::play() {

}

void FFmpegPlayer::pause() {
}

void FFmpegPlayer::stop() {
}

uint32_t FFmpegPlayer::getWidth() {
    return videoDecodec_->getWidth();
}

uint32_t FFmpegPlayer::getHeight() {
    return videoDecodec_->getHeight();
}

void FFmpegPlayer::seek(double targetPos, double seekRel) {
    resetPlaybackState();

    uint32_t serial = audioDecodec_->getSerial();
    if (FFINSTANCE()->getSyncType() == SyncType::VideoMaster) {
        serial = videoDecodec_->getSerial();
    }

    clock_.setMasterClock(targetPos, serial, CURRENT_TIME);
    demuxer_->seekRequest(targetPos, seekRel);
}

uint32_t FFmpegPlayer::getDuration() const {
    return demuxer_->getDuration();
}

// 确保返回值不是nan或者大于duration
double FFmpegPlayer::getCurrentTime() {
    checkPlaybackCompleted();

    static double cachedTime = 0;
    double currentTime = clock_.getMasterClock();
    if (!isnan(currentTime)) {
        cachedTime = currentTime;
    }

    const double duration = demuxer_->getDuration();
    if (playbackCompleted_) {
        return duration;
    }

    return std::min(cachedTime, duration);
}

void FFmpegPlayer::setVolume(uint32_t volume) {
    volume_ = volume;
}

uint32_t FFmpegPlayer::getVolume() const {
    return volume_;
}

void FFmpegPlayer::setSpeed(double speed) {
    speed_ = speed;
}

void FFmpegPlayer::getVideoFrame(VideoFrame* videoFrame) {
    videoDecodec_->getVideoFrame(videoFrame);
    checkPlaybackCompleted();
}

void FFmpegPlayer::getAudioFrame(AudioFrame* audioFrame) {
    audioDecodec_->getAudioFrame(audioFrame);
    checkPlaybackCompleted();
}

void FFmpegPlayer::updateAudioClock(double pts) {
    uint32_t serial = audioDecodec_->getSerial();
    clock_.setAudioClock(pts, serial, CURRENT_TIME);
    checkPlaybackCompleted();
}

void FFmpegPlayer::setOnAudioSeekCallback(const OnSeekCallback& cb) {
    audioDecodec_->setOnSeekCallback(cb);
}

void FFmpegPlayer::setOnVideoSeekCallback(const OnSeekCallback& cb) {
    videoDecodec_->setOnSeekCallback(cb);
}

void FFmpegPlayer::setOnPlaybackCompletedCallback(const OnPlaybackCompletedCallback& cb) {
    onPlaybackCompletedCallback_ = cb;
}

bool FFmpegPlayer::isPlaybackCompleted() const {
    return playbackCompleted_;
}

void FFmpegPlayer::setMasterMode(SyncType syncType) {
    FFINSTANCE()->setSyncType(syncType);
}

SyncType FFmpegPlayer::getMasterMode() const {
    return FFINSTANCE()->getSyncType();
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

void FFmpegPlayer::cleanup() const {
    demuxer_->exit();
    videoDecodec_->exit();
    audioDecodec_->exit();

    videoDecodec_->shutdown();
    audioDecodec_->shutdown();
}

bool FFmpegPlayer::checkPlaybackCompleted() {
    if (playbackCompleted_) {
        return true;
    }

    if (!demuxer_->isEof()) {
        return false;
    }

    auto isStreamDrained = [](Decodec* decodec) {
        if (!decodec->isOpen()) {
            return true;
        }

        return decodec->isDecodeFinished() && !decodec->hasBufferedFrames();
    };

    if (!isStreamDrained(videoDecodec_.get()) || !isStreamDrained(audioDecodec_.get())) {
        return false;
    }

    markPlaybackCompleted();
    return true;
}

void FFmpegPlayer::markPlaybackCompleted() {
    if (playbackCompleted_) {
        return;
    }

    playbackCompleted_ = true;
    clock_.pauseMasterClock();

    const double duration = demuxer_->getDuration();
    const double currentTime = clock_.getMasterClock();
    if (!isnan(currentTime) && currentTime > duration) {
        uint32_t serial = audioDecodec_->getSerial();
        if (FFINSTANCE()->getSyncType() == SyncType::VideoMaster) {
            serial = videoDecodec_->getSerial();
        }
        clock_.setMasterClock(duration, serial, CURRENT_TIME);
    }

    if (onPlaybackCompletedCallback_) {
        onPlaybackCompletedCallback_();
    }
}

void FFmpegPlayer::resetPlaybackState() {
    playbackCompleted_ = false;
    clock_.resumeMasterClock();
}
