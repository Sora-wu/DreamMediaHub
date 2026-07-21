//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamRecorder/DreamRecorder.h>

namespace Dream {
    class DreamRecorderContext {
    public:
        static DreamRecorder* createDreamRecorder();
        static void destroyDreamRecorder(DreamRecorder** recorder);

        // 音频缓存大小，需要应用层约束
        static constexpr uint32_t AUDIO_BUFFER_SIZE = 4096;
    };
}
