//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/mediaClock.h>
#include <FFmpegPlayer/ffinstance.h>

using namespace Dream;

///////////////////////////Clock///////////////////////////////////////////
void MediaClock::Clock::init(uint32_t* qSerial) {
    pts = NAN;
    lastUpdatedTime = CURRENT_TIME;
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

    double now = CURRENT_TIME;
    return pts + (now - lastUpdatedTime) * speed;
}

void MediaClock::Clock::setTime(double pts, uint32_t serial) {
    double time = CURRENT_TIME;
    setTime(pts, serial, time);
}

void MediaClock::Clock::setTime(double pts, uint32_t serial, double time) {
    this->pts = pts;
    this->lastUpdatedTime = time;
    this->serial = serial;
}

//////////////////////////MediaClock///////////////////////////////////////
void MediaClock::initClock(uint32_t* videoPktQueSerial, uint32_t* audioPktQueSerial) {
    videoClock_.init(videoPktQueSerial);
    audioClock_.init(audioPktQueSerial);
}

double MediaClock::getMasterClock() const {
    SyncType syncType = FFINSTANCE()->getSyncType();

    switch (syncType) {
    case SyncType::VideoMaster:
        return videoClock_.getTime();
    case SyncType::AudioMaster:
        return audioClock_.getTime();
    }

    return 0;
}

double MediaClock::getVideoClock() const {
    return videoClock_.getTime();
}

double MediaClock::getAudioClock() const {
    return audioClock_.getTime();
}

uint32_t MediaClock::getVideoSerial() const {
    return videoClock_.getSerial();
}

uint32_t MediaClock::getAudioSerial() const {
    return audioClock_.getSerial();
}

void MediaClock::setMasterClock(double pts, uint32_t serial, double time) {
    SyncType syncType = FFINSTANCE()->getSyncType();

    switch (syncType) {
    case SyncType::VideoMaster:
        videoClock_.setTime(pts, serial, time);
        break;
    case SyncType::AudioMaster:
        audioClock_.setTime(pts, serial, time);
        break;
    }
}

void MediaClock::setVideoClock(double pts, uint32_t serial, double time) {
    videoClock_.setTime(pts, serial, time);
}

void MediaClock::setAudioClock(double pts, uint32_t serial, double time) {
    audioClock_.setTime(pts, serial, time);
}

void MediaClock::pauseMasterClock() {
    SyncType syncType = FFINSTANCE()->getSyncType();
    Clock* masterClock = syncType == SyncType::VideoMaster ? &videoClock_ : &audioClock_;

    double currentTime = masterClock->getTime();
    if (!isnan(currentTime)) {
        masterClock->setTime(currentTime, masterClock->getSerial(), CURRENT_TIME);
    }
    masterClock->paused = true;
}

void MediaClock::resumeMasterClock() {
    videoClock_.paused = false;
    audioClock_.paused = false;
}