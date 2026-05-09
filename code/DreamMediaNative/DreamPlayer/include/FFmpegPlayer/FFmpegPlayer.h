//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamPlayer/DreamPlayer.h>

class FFmpegPlayer : public Dream::DreamPlayer {
public:
    ~FFmpegPlayer() override;

    bool open(const char* url) override;
    void play() override;
    void pause() override;
    void stop() override;

    void seek(double position) override;
    double getDuration() const override;

    void setVolume(int volume) override;
    int getVolume() const override;

    void setSpeed(double speed) override;
};