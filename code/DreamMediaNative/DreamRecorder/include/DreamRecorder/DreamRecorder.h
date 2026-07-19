//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamRecorder/define.h>
#include <DreamRecorder/structs.h>

namespace Dream {
    class DREAM_RECORDER_API DreamRecorder {
    public:
        virtual ~DreamRecorder() {}

        virtual void start(const char* path, const VideoParam& videoParam = {}, const AudioParam& audioParam = {}) = 0;
        virtual void stop() = 0;

        // 零拷贝
        virtual VideoFrameBuffer getVideoBuffer() = 0;

        // 下面两个pts但是都是微秒
        virtual void commitVideoBuffer(const VideoFrameBuffer& buffer, uint64_t pts) = 0;
        virtual void pushAudioData(char* data, uint32_t size, uint64_t pts) = 0;
    };
}