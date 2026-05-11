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
    ~Decodec() override {
        cleanup();
    }

    virtual bool openCodec(uint32_t streamIndex, AVStream* stream);

protected:
    virtual void cleanup();

protected:
    ConcurrentQueue<AVPacket> pktQue_;

    AVCodecContext* codecCtx_ = nullptr;
    AVRational timeBase_{};
    uint32_t streamIndex_ = -1;
};