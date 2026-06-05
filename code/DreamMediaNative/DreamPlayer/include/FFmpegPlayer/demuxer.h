//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/dataHandler.hpp>
#include <common/dreamThread.hpp>

#include <vector>
#include <thread>
#include <condition_variable>

extern "C" {
    struct AVFormatContext;
    struct AVStream;
}

class Demuxer : public DataHandler, public DreamThread {
public:
    ~Demuxer() override {}

    [[nodiscard]] bool open(const char* url);
    [[nodiscard]] uint64_t getDuration() const {
        return duration_;
    }

    [[nodiscard]] std::vector<AVStream*> getStreams() const {
        return streams_;
    }

    void run(std::stop_token st) override;

private:
    void pushNullPacket();

private:
    AVFormatContext* fmtCtx_ = nullptr;
    uint64_t duration_ = 0;

    std::vector<AVStream*> streams_;
    bool isEof_ = false;

    std::condition_variable cvRead_;
    std::mutex mtxRead_;
};