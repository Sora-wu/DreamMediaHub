//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamPlayer/DreamPlayer.h>
#include <common/dataHandler.hpp>

#include <memory>

class Demuxer;
class VideoDecodec;
class AudioDecodec;

class FFmpegPlayer : public Dream::DreamPlayer, public DataHandler {
public:
    FFmpegPlayer();
    ~FFmpegPlayer() override;

    bool open(const char* url) override;
    void play() override;
    void pause() override;
    void stop() override;

    [[nodiscard]] uint32_t getWidth() override;
    [[nodiscard]] uint32_t getHeight() override;

    void seek(double position) override;
    [[nodiscard]] uint64_t getDuration() const override;

    void setVolume(uint32_t volume) override;
    [[nodiscard]] uint32_t getVolume() const override;

    void setSpeed(double speed) override;

    void getVideoFrame(Dream::VideoFrame* videoFrame) override;

    void handle(void* data) override;
private:
    void cleanup();

private:
    uint32_t volume_ = 100;
    double speed_ = 1.0;
    bool pause_ = false;
    double duration_ = 0.0;

    std::unique_ptr<Demuxer> demuxer_;
    std::unique_ptr<VideoDecodec> videoDecodec_;
    std::unique_ptr<AudioDecodec> audioDecodec_;
};