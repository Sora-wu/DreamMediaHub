//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <FFmpegPlayer/decodec.h>

class AudioDecodec : public Decodec {
public:
    bool openCodec(uint32_t streamIndex, AVStream* stream) override;

    void handle(void* data) override;
    void run() override;

protected:
    void cleanup() override;

private:

};