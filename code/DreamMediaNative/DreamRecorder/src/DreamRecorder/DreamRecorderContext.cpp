//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <DreamRecorder/DreamRecorderContext.h>
#include <FFmpegRecorder/FFmpegRecorder.h>

using namespace Dream;

DreamRecorder* DreamRecorderContext::createDreamRecorder() {
    return new FFmpegRecorder;
}

void DreamRecorderContext::destroyDreamRecorder(DreamRecorder** recorder) {
    if (recorder) {
        delete *recorder;
        *recorder = nullptr;
    }
}
