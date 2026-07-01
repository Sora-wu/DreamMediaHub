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

FFInstance::FFInstance() {
    av_init_packet(&flushPkt_);
    flushPkt_.data = (uint8_t*)&flushPkt_;
}

void FFInstance::initClock(uint32_t* videoPktQueSerial, uint32_t* audioPktQueSerial) {
    clock_.initClock(videoPktQueSerial, audioPktQueSerial);
}

double FFInstance::getMasterClock() const {
    if (syncType_ == Dream::SyncType::VideoMaster) {
        return clock_.getVideoClock();
    }

    return clock_.getAudioClock();
}

double FFInstance::getVideoClock() const {
    return clock_.getVideoClock();
}

double FFInstance::getAudioClock() const {
    return clock_.getAudioClock();
}

double FFInstance::getMasterSerial() const {
    if (syncType_ == Dream::SyncType::VideoMaster) {
        return clock_.getVideoSerial();
    }

    return clock_.getAudioSerial();
}

uint32_t FFInstance::getVideoSerial() const {
    return clock_.getVideoSerial();
}

uint32_t FFInstance::getAudioSerial() const {
    return clock_.getAudioSerial();
}

double FFInstance::getVideoPTS() const {
    return clock_.getVideoPTS();
}

double FFInstance::getAudioPTS() const {
    return clock_.getAudioPTS();
}

void FFInstance::setMasterClock(double pts, uint32_t serial, double time) {
    if (syncType_ == Dream::SyncType::VideoMaster) {
        clock_.setVideoClock(pts, serial, time);
    }

    clock_.setAudioClock(pts, serial, time);
}

void FFInstance::setMasterClock(double pts, uint32_t serial) {
    if (syncType_ == Dream::SyncType::VideoMaster) {
        clock_.setVideoClock(pts, serial);
    }

    clock_.setAudioClock(pts, serial);
}
void FFInstance::setMasterClock(double pts, double time) {
    if (syncType_ == Dream::SyncType::VideoMaster) {
        clock_.setVideoClock(pts, time);
    }

    clock_.setAudioClock(pts, time);
}

void FFInstance::setVideoClock(double pts, uint32_t serial, double time) {
    clock_.setVideoClock(pts, serial, time);
}

void FFInstance::setAudioClock(double pts, uint32_t serial, double time) {
    clock_.setAudioClock(pts, serial, time);
}

void FFInstance::setVideoClock(double pts, uint32_t serial) {
    clock_.setVideoClock(pts, serial);
}

void FFInstance::setAudioClock(double pts, uint32_t serial) {
    clock_.setAudioClock(pts, serial);
}

void FFInstance::setVideoClock(double pts, double time) {
    clock_.setVideoClock(pts, time);
}

void FFInstance::setAudioClock(double pts, double time) {
    clock_.setAudioClock(pts, time);
}

float FFInstance::getSpeed() const {
    return clock_.getSpeed();
}

void FFInstance::setSpeed(float speed) {
    return clock_.setSpeed(speed);
}

void FFInstance::pauseMasterClock() {
    if (syncType_ == Dream::SyncType::VideoMaster) {
        clock_.pauseVideoClock();
    }

    clock_.pauseAudioClock();
}

bool FFInstance::getMasterClockPause() {
    if (syncType_ == Dream::SyncType::VideoMaster) {
        return clock_.getVideoClockPause();
    }

    return clock_.getAudioClockPause();
}

void FFInstance::resumeMasterClock() {
    clock_.resumeClock();
}
