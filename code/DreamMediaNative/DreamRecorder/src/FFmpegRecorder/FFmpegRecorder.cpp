//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegRecorder/FFmpegRecorder.h>
#include <FFmpegRecorder/ffinstance.h>

namespace {
    constexpr uint32_t VIDEO_QUE_SIZE = 5;
    constexpr uint32_t AUDIO_QUE_SIZE = 16;
}

using namespace Dream;

void FFmpegRecorder::start(const char* path, const VideoParam& videoParam, const AudioParam& audioParam) {
    mux_ = std::make_unique<Muxer>();
    mux_->init(path);

    if (videoParam.enable) {
        videoEncoder_ = std::make_unique<VideoEncoder>(videoParam, VIDEO_QUE_SIZE);
        videoEncoder_->addStream(AV_CODEC_ID_H264);
        videoEncoder_->setNextHandler(mux_.get());
        videoEncoder_->start("video encoder thread");
    }

    if (audioParam.enable) {
        audioEncoder_ = std::make_unique<AudioEncoder>(audioParam, AUDIO_QUE_SIZE);
        audioEncoder_->addStream(AV_CODEC_ID_AAC);
        audioEncoder_->setNextHandler(mux_.get());
        audioEncoder_->start("audio encoder thread");
    }

    mux_->start("muxer thread");
}

void FFmpegRecorder::stop() {
    if (videoEncoder_) {
        videoEncoder_->exit();
        videoEncoder_->close();
        videoEncoder_ = nullptr;
    }

    if (audioEncoder_) {
        audioEncoder_->exit();
        audioEncoder_->close();
        audioEncoder_ = nullptr;
    }

    mux_->exit();
    mux_->close();
    mux_ = nullptr;
}

VideoFrameBuffer FFmpegRecorder::getVideoBuffer() {
    return videoEncoder_->getVideoBuffer();
}

void FFmpegRecorder::commitVideoBuffer(const VideoFrameBuffer& buffer, uint64_t pts) {
    videoEncoder_->commitVideoBuffer(buffer, pts);
}

void FFmpegRecorder::pushAudioData(char* data, uint32_t size, uint64_t pts) {
}
