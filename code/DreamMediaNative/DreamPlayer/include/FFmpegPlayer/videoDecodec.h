//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>

namespace Dream {
    struct VideoFrame;
}

class MediaClock;

class VideoDecodec : public Decodec {
public:
    explicit VideoDecodec(std::size_t frameQueueSize) : Decodec(frameQueueSize) {}
    ~VideoDecodec() override;

    bool openCodec(int streamIndex, AVStream* stream) override;

    uint32_t getWidth() const;
    uint32_t getHeight() const;

    void run(std::stop_token st) override;

    void setMediaClock(MediaClock* clock) {
        mediaClock_ = clock;
    }

    void reopen();

    void getVideoFrame(Dream::VideoFrame* videoFrame);

protected:
    void cleanup() override;

private:
    void resetSyncState();
    bool acquireFrame(Frame& out);
    static void releaseFrame(Frame& frame);
    bool shouldDropLateFrame(double time) const;
    double getFrameDuration(const Frame& frame, const Frame& nextFrame) const;
    double calcDelay(double duration) const;
    void fillData(AVFrame* frame, Dream::VideoFrame* videoFrame);

private:
    SwsContext* swsCtx_ = nullptr;

    MediaClock* mediaClock_ = nullptr;
    Frame heldFrame_{};
    bool decodeTimerValid_ = false;
    double maxFrameDuration_ = 3600.0;
};