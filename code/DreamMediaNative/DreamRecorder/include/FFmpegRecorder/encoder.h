//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/dreamThread.hpp>
#include <common/dataHandler.hpp>
#include <common/concurrentQueue.hpp>

#include <mutex>

extern "C" {
#include <libavcodec/codec_id.h>
}

extern "C" {
    struct AVFrame;
    struct AVCodecContext;
    struct AVStream;
}

class Encoder : public DreamThread, public DataHandler {
public:
    explicit Encoder(uint32_t queSize = 0) : frameQue_(queSize) {}
    virtual ~Encoder() {}

    virtual void addStream(AVCodecID codecID);
    virtual void initCodecContext(AVCodecID codecID) = 0;

protected:
    ConcurrentQueue<AVFrame*> frameQue_;
    AVCodecContext* codecCtx_ = nullptr;
    AVStream* stream_ = nullptr;
    std::mutex mtx_{};
};
