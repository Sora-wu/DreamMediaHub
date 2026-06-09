//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#define CURRENT_TIME (double)av_gettime_relative() / 1e6

class FFUtils {
public:
    static std::string getError(int errnum);
};