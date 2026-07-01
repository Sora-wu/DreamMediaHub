//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamPlayer/DreamPlayer.h>
#include <common/dataHandler.hpp>
#include <common/dreamThread.hpp>

#include <memory>
#include <mutex>

class Demuxer;
class VideoDecodec;
class AudioDecodec;

class FFmpegPlayer final : public Dream::DreamPlayer, public DataHandler, private DreamThread {
public:
    FFmpegPlayer();
    ~FFmpegPlayer() override;

    bool open(const char* url) override;
    void play() override;
    void pause() override;
    bool isPlaying() const override;
    void stop() override;

    [[nodiscard]] uint32_t getWidth() override;
    [[nodiscard]] uint32_t getHeight() override;

    void seek(double targetPos, double seekRel) override;
    [[nodiscard]] uint32_t getDuration() const override;
    [[nodiscard]] double getCurrentTime() override;

    float getSpeed() override;
    void setSpeed(double speed) override;

    void getVideoFrame(Dream::VideoFrame* videoFrame) override;
    void getAudioFrame(Dream::AudioFrame* audioFrame) override;

    void updateAudioClock(double pts, uint32_t serial) override;

    void setOnAudioSeekCallback(const Dream::OnSeekCallback& cb) override;
    void setOnVideoSeekCallback(const Dream::OnSeekCallback& cb) override;

    void setOnPlaybackCompletedCallback(const Dream::OnPlaybackCompletedCallback& cb) override;
    [[nodiscard]] bool isPlaybackCompleted() const override;

    void setMasterMode(Dream::SyncType syncType) override;
    [[nodiscard]] Dream::SyncType getMasterMode() const override;

    // 处理来自解封装器的数据
    void handle(void* data) override;
private:
    // 检测线程
    void run(std::stop_token st) override;

    void shutdown() const;
    // 仅检测线程
    bool checkPlaybackCompleted();
    void markPlaybackCompleted();
    // void resetPlaybackState();

private:
    bool pause_ = false;
    bool playbackCompleted_ = false;

    Dream::OnPlaybackCompletedCallback onPlaybackCompletedCallback_ = nullptr;

    std::unique_ptr<Demuxer> demuxer_;
    std::unique_ptr<VideoDecodec> videoDecodec_;
    std::unique_ptr<AudioDecodec> audioDecodec_;
    std::mutex mtx_{};
};