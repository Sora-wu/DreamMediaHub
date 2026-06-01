//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/mediaClock.h>
#include <FFmpegPlayer/ffinstance.h>

///////////////////////////Clock///////////////////////////////////////////
void MediaClock::Clock::init(uint32_t* qSerial) {
    pts = NAN;
    lastUpdatedTime = (double)av_gettime_relative() / 1e6;
    speed = 1.0;
    paused = false;
    serial = 0;
    queSerial = qSerial;
}

double MediaClock::Clock::getTime() const {
    if (*queSerial != serial) {
        return NAN;
    }

    if (paused) {
        return pts;
    }

    // TODO: 加上变速
    return (double)av_gettime_relative() / 1e6;
}

void MediaClock::Clock::setTime(double pts, uint32_t serial) {
    double time = (double)av_gettime_relative() / 1e6;
    setTime(pts, serial, time);
}

void MediaClock::Clock::setTime(double pts, uint32_t serial, double time) {
}

//////////////////////////MediaClock///////////////////////////////////////
void MediaClock::initClock(uint32_t* videoPktQueSerial, uint32_t* audioPktQueSerial) {
    videoClock_.init(videoPktQueSerial);
    audioClock_.init(audioPktQueSerial);
}

double MediaClock::getMasterClock() const {
    SyncType syncType = FFINSTANCE()->getSyncType();

    if (syncType == SyncType::AudioMaster) {
        return audioClock_.getTime();
    }

    switch (syncType) {
    case SyncType::VideoMaster:
        return videoClock_.getTime();
    case SyncType::AudioMaster:
        return audioClock_.getTime();
    }

    return 0;
}
