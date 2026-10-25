//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/ffcore.h>

std::string FFUtils::getError(int errnum) {
    char buf[AV_ERROR_MAX_STRING_SIZE]{};
    std::string ret;

    if (av_strerror(errnum, buf, AV_ERROR_MAX_STRING_SIZE) < 0) {
#if defined(PLATFORM_WINDOWS) && defined(_MSC_VER)
        strerror_s(buf, sizeof(buf), AVUNERROR(errnum));
        ret = buf;
#else
        ret = strerror(AVUNERROR(errnum));
#endif
    }

    return ret;
}

bool FFUtils::isEqual(float a, float b) {
    return fabs(a - b) < 1e-6;
}

bool FFUtils::isEqual(double a, double b) {
    return fabs(a - b) < 1e-6;
}
