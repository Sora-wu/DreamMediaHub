//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamRecorder/DreamRecorder.h>

namespace Dream {
    class DreamRecorderContext {
        static DreamRecorder* createDreamRecorder();
        static void destroyDreamRecorder(DreamRecorder** recorder);
    };
}
