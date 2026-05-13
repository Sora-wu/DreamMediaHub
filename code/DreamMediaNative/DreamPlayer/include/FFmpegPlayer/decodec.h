//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/dataHandler.hpp>
#include <common/dreamThread.hpp>
#include <common/concurrentQueue.hpp>
#include <FFmpegPlayer/ffcore.h>

class Decodec : public DataHandler, public DreamThraed {
public:
    explicit Decodec(std::size_t frameQueueSize) : pktQue_(frameQueueSize) {}
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

    void handle(void* data) override;

protected:
    virtual void cleanup();

protected:
    ConcurrentQueue<AVPacket> pktQue_;

    AVCodecContext* codecCtx_ = nullptr;
    AVStream* stream_ = nullptr;
    int streamIndex_ = -1;
};