//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/dataHandler.hpp>
#include <common/dreamThread.hpp>
#include <FFmpegPlayer/packetQueue.h>
#include <FFmpegPlayer/ffcore.h>

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
        cleanup();
    }

    virtual bool openCodec(int streamIndex, AVStream* stream);

    [[nodiscard]] bool isOpen() const {
        return streamIndex_ != -1;
    }

    [[nodiscard]] int getStreamIndex() const {
        return streamIndex_;
    }

    [[nodiscard]] uint32_t& getSerial() {
        return decodecSerial_;
    }

    [[nodiscard]] uint64_t getTimer() const {
        return decodecTimer_;
    }

    void handle(void* data) override;

    void shutdown();
    void reopen();

protected:
    virtual void cleanup();
    [[nodiscard]] virtual bool decodeFrame(AVFrame* frame);
    void pushFrame(AVFrame* frame, uint32_t serial, double pts, double duration);

protected:
    PacketQueue pktQue_;
    ConcurrentQueue<Frame> frameQue_;

    AVCodecContext* codecCtx_ = nullptr;
    AVStream* stream_ = nullptr;
    int streamIndex_ = -1;
    uint32_t decodecSerial_ = 0;
    double decodecTimer_ = 0;
};