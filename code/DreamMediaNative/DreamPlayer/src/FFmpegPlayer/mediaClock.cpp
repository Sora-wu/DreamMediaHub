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

void MediaClock::Clock::setTime(double pts, double time) {
    this->pts = pts;
    this->lastUpdatedTime = time;
}

//////////////////////////MediaClock///////////////////////////////////////
void MediaClock::initClock(uint32_t* videoPktQueSerial, uint32_t* audioPktQueSerial) {
    videoClock_.init(videoPktQueSerial);
    audioClock_.init(audioPktQueSerial);
}

double MediaClock::getVideoClock() const {
    return videoClock_.getTime();
}

double MediaClock::getAudioClock() const {
    return audioClock_.getTime();
}

uint32_t MediaClock::getVideoSerial() const {
    return videoClock_.serial;
}

uint32_t MediaClock::getAudioSerial() const {
    return audioClock_.serial;
}

double MediaClock::getVideoPTS() const {
    return videoClock_.pts;
}

double MediaClock::getAudioPTS() const {
    return audioClock_.pts;
}

void MediaClock::setVideoClock(double pts, uint32_t serial, double time) {
    videoClock_.setTime(pts, serial, time);
}

void MediaClock::setAudioClock(double pts, uint32_t serial, double time) {
    audioClock_.setTime(pts, serial, time);
}

void MediaClock::setVideoClock(double pts, uint32_t serial) {
    videoClock_.setTime(pts, serial);
}

void MediaClock::setAudioClock(double pts, uint32_t serial) {
    audioClock_.setTime(pts, serial);
}

void MediaClock::setVideoClock(double pts, double time) {
    videoClock_.setTime(pts, time);
}

void MediaClock::setAudioClock(double pts, double time) {
    videoClock_.setTime(pts, time);
}

float MediaClock::getSpeed() const {
    float speed = audioClock_.speed;
    if (FFINSTANCE()->getSyncType() == SyncType::VideoMaster) {
        speed = videoClock_.speed;
    }
    return speed;
}

void MediaClock::setSpeed(float speed) {
    videoClock_.speed = speed;
    audioClock_.speed = speed;
}

void MediaClock::pauseVideoClock() {
    double currentPTS = videoClock_.getTime();
    uint32_t serial = videoClock_.serial;
    videoClock_.setTime(currentPTS, serial, CURRENT_TIME);
}

void MediaClock::pauseAudioClock() {
    double currentPTS = audioClock_.getTime();
    uint32_t serial = audioClock_.serial;
    audioClock_.setTime(currentPTS, serial, CURRENT_TIME);
}

void MediaClock::resumeClock() {
    videoClock_.paused = false;
    audioClock_.paused = false;
}
