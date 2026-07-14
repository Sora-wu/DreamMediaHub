//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegRecorder/ffinstance.h>

FFInstance* FFInstance::instance() {
    static FFInstance instance;
    return &instance;
}