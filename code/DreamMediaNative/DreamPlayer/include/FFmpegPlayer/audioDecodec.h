//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>

class AudioDecodec : public Decodec {
public:
    explicit AudioDecodec(std::size_t frameQueueSize) : Decodec(frameQueueSize) {}
    bool openCodec(int streamIndex, AVStream* stream) override;

    void run() override;

protected:
    void cleanup() override;

private:

};