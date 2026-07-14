//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/dreamThread.hpp>
#include <common/dataHandler.hpp>

extern "C" {
    struct AVFormatContext;
}

class Muxer : public DreamThread, public DataHandler {
public:
    Muxer() {}
    ~Muxer() override {}

    void init(const char* path);
    void cleanup();
    void run(std::stop_token st) override;

private:
    AVFormatContext* fmtCtx_ = nullptr;
};