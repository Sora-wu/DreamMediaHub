//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

// FFmpeg 4.x uses a C99 compound literal here. Preserve the macro's meaning
// while providing the equivalent C++ initializer for MSVC.
#if defined(PLATFORM_WINDOWS) && defined(_MSC_VER)
#undef AV_TIME_BASE_Q
#define AV_TIME_BASE_Q AVRational{1, AV_TIME_BASE}
#endif