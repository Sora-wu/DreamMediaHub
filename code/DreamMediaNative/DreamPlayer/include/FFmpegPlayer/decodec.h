//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/dataHandler.hpp>
#include <common/dreamThread.hpp>
#include <FFmpegPlayer/packetQueue.h>
#include <FFmpegPlayer/ffcore.h>
#include <DreamPlayer/structs.h>

#include <mutex>

struct Frame {
    AVFrame* frame{};
    uint32_t serial{};
    double pts{};
    double duration{};
};

class Decodec : public DataHandler, public DreamThread {
public:
    explicit Decodec(std::size_t frameQueueSize) : frameQue_(frameQueueSize) {}
    ~Decodec() override {
        Decodec::cleanup();
    }

    virtual bool openCodec(int streamIndex, AVStream* stream);
    virtual void reopen();
    virtual void startDecode();

    [[nodiscard]] bool isOpen() const {
        return streamIndex_ != -1;
    }

    [[nodiscard]] int getStreamIndex() const {
        return streamIndex_;
    }

    [[nodiscard]] uint32_t* getPktQueSerial() {
        return pktQue_.getSerial();
    }

    [[nodiscard]] uint64_t getTimer() const {
        return static_cast<uint64_t>(decodecTimer_);
    }

    void handle(void* data) override;

    void shutdown();

    [[nodiscard]] bool isDecodeFinished();
    [[nodiscard]] bool hasBufferedFrames() const;
    void setPause(bool isPause) {
        isPause_ = isPause;
    }

    void setOnSeekCallback(const Dream::OnSeekCallback& cb) {
        onSeekCallback_ = cb;
    }

protected:
    virtual void cleanup();
    [[nodiscard]] virtual bool decodeFrame(AVFrame* frame);
    void pushFrame(AVFrame* frame, double pts, double duration);
    bool acquireFrame(Frame& out);
    static void releaseFrame(Frame& frame);

protected:
    PacketQueue pktQue_;
    ConcurrentQueue<Frame> frameQue_;
    std::mutex mtx_{};

    AVCodecContext* codecCtx_ = nullptr;
    AVStream* stream_ = nullptr;
    int streamIndex_ = -1;
    uint32_t decodecSerial_ = 0;
    uint32_t finishedSerial_ = 0;
    double decodecTimer_ = 0;
    bool decodeTimerValid_ = false;

    bool isPause_ = true;

    Dream::OnSeekCallback onSeekCallback_ = nullptr;
};