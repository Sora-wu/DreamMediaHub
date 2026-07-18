//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamRecorder/define.h>

namespace Dream {
    enum class PixelFormat {
        RGB = 2,
        BGR = 3,
    };

    enum class SampleFormat {
        S16 = 1,
        FLTP = 8
    };

    struct DREAM_RECORDER_API VideoParam {
        uint32_t width = 1920;
        uint32_t height = 1080;
        uint32_t fps = 30;
        uint32_t bitRate = 4000000;
        PixelFormat pixelFormat = PixelFormat::RGB;
        bool enable = true;
    };

    struct DREAM_RECORDER_API AudioParam {
        uint32_t sampleRate = 44100;
        uint32_t channels = 2;
        uint32_t bitRate = 64000;
        SampleFormat sampleFormat = SampleFormat::FLTP;
        bool enable = false;                // 暂时屏蔽
    };

    struct DREAM_RECORDER_API VideoFrameBuffer {
        char* data = nullptr;
        uint32_t stride = 0;
        uint32_t size = 0;
        void* opaque = nullptr;             // native层自用
    };
}