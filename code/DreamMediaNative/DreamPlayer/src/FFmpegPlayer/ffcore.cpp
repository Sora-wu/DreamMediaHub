//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/ffcore.h>

std::string FFUtils::getError(int errnum) {
    char buf[AV_ERROR_MAX_STRING_SIZE]{};
    std::string ret;

    if (av_strerror(errnum, buf, AV_ERROR_MAX_STRING_SIZE) < 0) {
        ret = strerror(AVUNERROR(errnum));
    }

    return ret;
}