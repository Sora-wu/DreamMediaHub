//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
}

class FFUtils {
public:
    static std::string getError(int errnum);
};