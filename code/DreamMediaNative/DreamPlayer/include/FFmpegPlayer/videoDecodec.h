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
    void reopen() override;
    void startDecode() override;

    uint32_t getWidth() const;
    uint32_t getHeight() const;

    void run(std::stop_token st) override;

    Dream::VideoFrame* getVideoFrame();

protected:
    void cleanup() override;

private:
    void resetSyncState();
    bool shouldDropLateFrame(double time) const;
    double getFrameDuration(const Frame& frame, const Frame& nextFrame) const;
    double calcDelay(double duration) const;
    bool fillData(AVFrame* frame);

private:
    SwsContext* swsCtx_ = nullptr;

    Frame heldFrame_{};
    Dream::VideoFrame videoFrame_{};
    double maxFrameDuration_ = 3600.0;
    int cachedFormat_ = AV_PIX_FMT_NONE;
};