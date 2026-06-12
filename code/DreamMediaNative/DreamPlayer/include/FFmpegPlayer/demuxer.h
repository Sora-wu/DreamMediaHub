//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/dataHandler.hpp>
#include <common/dreamThread.hpp>

#include <vector>
#include <condition_variable>

extern "C" {
    struct AVFormatContext;
    struct AVStream;
}

class Demuxer : public DataHandler, public DreamThread {
public:
    ~Demuxer() override = default;

    [[nodiscard]] bool open(const char* url);
    [[nodiscard]] uint32_t getDuration() const {
        return duration_;
    }

    [[nodiscard]] std::vector<AVStream*> getStreams() const {
        return streams_;
    }

    void run(std::stop_token st) override;

    void seekRequest(double targetPos, double seekRel);

private:
    void seek();
    void pushNullPacket();

private:
    AVFormatContext* fmtCtx_ = nullptr;
    uint32_t duration_ = 0;

    std::vector<AVStream*> streams_;
    bool isEof_ = false;

    bool hasSeekRequest_ = false;
    double seekTargetPos_ = 0;
    double seekRel_ = 0;
    std::condition_variable cvRead_;
    std::mutex mtxRead_;
};