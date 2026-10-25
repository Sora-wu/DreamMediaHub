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
    constexpr std::size_t FRAME_AUDIO_SIZE = 16;
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
    shutdown();
}

bool FFmpegPlayer::open(const char* url) {
    shutdown();

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

    exit();
    videoDecodec_->reopen();
    audioDecodec_->reopen();

    start("monitor thread");
    demuxer_->startDemux();
    videoDecodec_->startDecode();
    audioDecodec_->startDecode();

    uint32_t* videoPktQueSerial = videoDecodec_->getPktQueSerial();
    uint32_t* audioPktQueSerial = audioDecodec_->getPktQueSerial();
    FFINSTANCE()->initClock(videoPktQueSerial, audioPktQueSerial);

    return true;
}

void FFmpegPlayer::play() {
    FFINSTANCE()->resumeMasterClock();
    videoDecodec_->setPause(false);
    audioDecodec_->setPause(false);
}

void FFmpegPlayer::pause() {
    FFINSTANCE()->pauseMasterClock();
    videoDecodec_->setPause(true);
    audioDecodec_->setPause(true);
}

bool FFmpegPlayer::isPlaying() const {
    return FFINSTANCE()->getMasterClockPause();
}

void FFmpegPlayer::stop() {
    audioDecodec_->shutdown();
    videoDecodec_->shutdown();

    audioDecodec_->exit();
    videoDecodec_->exit();
    demuxer_->exit();
    exit();
}

uint32_t FFmpegPlayer::getWidth() {
    return videoDecodec_->getWidth();
}

uint32_t FFmpegPlayer::getHeight() {
    return videoDecodec_->getHeight();
}

void FFmpegPlayer::seek(double targetPos, double seekRel) {
    demuxer_->seekRequest(targetPos, seekRel);
}

uint32_t FFmpegPlayer::getDuration() const {
    return demuxer_->getDuration();
}

// 确保返回值不是nan或者大于duration
double FFmpegPlayer::getCurrentTime() {
    static double cachedTime = 0;
    double currentTime = FFINSTANCE()->getMasterClock();
    if (!isnan(currentTime)) {
        cachedTime = currentTime;
    }

    const double duration = demuxer_->getDuration();
    if (playbackCompleted_) {
        return duration;
    }

    return std::min(cachedTime, duration);
}

float FFmpegPlayer::getSpeed() {
    return FFINSTANCE()->getSpeed();
}

void FFmpegPlayer::setSpeed(double speed) {
    double pts = FFINSTANCE()->getAudioPTS();
    FFINSTANCE()->setMasterClock(pts, CURRENT_TIME);

    FFINSTANCE()->setSpeed(static_cast<float>(speed));
    audioDecodec_->setSpeed(static_cast<float>(speed));
}

VideoFrame* FFmpegPlayer::getVideoFrame() {
    return videoDecodec_->getVideoFrame();
}

AudioFrame* FFmpegPlayer::getAudioFrame() {
    return audioDecodec_->getAudioFrame();
}

void FFmpegPlayer::updateAudioClock(double pts, uint32_t serial) {
    FFINSTANCE()->setAudioClock(pts, serial);
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

void FFmpegPlayer::setMasterMode(SyncType syncType) {
    FFINSTANCE()->setSyncType(syncType);
}

SyncType FFmpegPlayer::getMasterMode() const {
    return FFINSTANCE()->getSyncType();
}

void FFmpegPlayer::handle(void* data) {
    AVPacket* pkt = (AVPacket*)data;

    if (playbackCompleted_) {
        videoDecodec_->reopen();
        audioDecodec_->reopen();
        playbackCompleted_ = false;
    }

    if (pkt->stream_index == videoDecodec_->getStreamIndex()) {
        videoDecodec_->handle(pkt);
    }
    else if (pkt->stream_index == audioDecodec_->getStreamIndex()) {
        audioDecodec_->handle(pkt);
    }
}

void FFmpegPlayer::run(std::stop_token st) {
    while (!st.stop_requested()) {
        checkPlaybackCompleted();
        msleep(100);
    }
}

void FFmpegPlayer::shutdown() const {
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

    // 播放完毕把对应的流关闭，避免后面继续解码
    bool isVideoStreamDrained = isStreamDrained(videoDecodec_.get());
    if (isVideoStreamDrained) {
        videoDecodec_->shutdown();
    }
    bool isAudioStreamDrained = isStreamDrained(audioDecodec_.get());
    if (isAudioStreamDrained) {
        audioDecodec_->shutdown();
    }

    if (!isVideoStreamDrained || !isAudioStreamDrained) {
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
    FFINSTANCE()->pauseMasterClock();

    const double duration = demuxer_->getDuration();
    const double currentTime = FFINSTANCE()->getMasterClock();
    if (isnan(currentTime)) {
        return;
    }
    if (currentTime > duration) {
        FFINSTANCE()->setMasterClock(duration, CURRENT_TIME);
    }

    shutdown();
    av_log(nullptr, AV_LOG_INFO, "current media player completed\n");
    setSpeed(1.0f);     // 当前媒体播放完毕之后重置速度

    if (onPlaybackCompletedCallback_) {
        onPlaybackCompletedCallback_();
    }
}