//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/ffinstance.h>

FFInstance* FFInstance::instance_ = nullptr;

FFInstance* FFInstance::instance() {
    if (!instance_) {
        instance_ = new FFInstance;
    }

    return instance_;
}