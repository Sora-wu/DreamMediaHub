//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/dreamThread.hpp>
#include <common/dataHandler.hpp>
#include <common/concurrentQueue.hpp>

extern "C" {
    struct AVFormatContext;
    struct AVPacket;
}

class Muxer : public DreamThread, public DataHandler {
public:
    Muxer() {}
    ~Muxer() override {}

    void init(const char* path);
    void cleanup();
    void run(std::stop_token st) override;

    void handle(void* data) override;

private:
    AVPacket* getPacket();

private:
    AVFormatContext* fmtCtx_ = nullptr;
    ConcurrentQueue<AVPacket*> videoPktQue_;
    ConcurrentQueue<AVPacket*> audioPktQue_;
};