//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamPlayer/define.h>
#include <DreamPlayer/structs.h>

namespace Dream {
    class DREAM_PLAYER_API DreamPlayer {
    public:
        virtual ~DreamPlayer() = default;
        virtual bool open(const char* url) = 0;
        virtual void play() = 0;
        virtual void pause() = 0;
        virtual void stop() = 0;

        [[nodiscard]] virtual uint32_t getWidth() = 0;
        [[nodiscard]] virtual uint32_t getHeight() = 0;

        // targetPos: 单位秒
        // seekRel: 单位秒，用键盘方向键按下时指定一个seek的范围
        // 如果拖动进度条调用seek(target);
        // 按键盘方向键则seek(currentTime, offset);
        virtual void seek(double targetPos, double seekRel) = 0;
        [[nodiscard]] virtual uint32_t getDuration() const = 0;
        [[nodiscard]] virtual double getCurrentTime() = 0;

        virtual float getSpeed() = 0;
        virtual void setSpeed(double speed) = 0;

        virtual void getVideoFrame(VideoFrame* videoFrame) = 0;
        virtual void getAudioFrame(AudioFrame* audioFrame) = 0;

        // 由于native只负责解码出帧，音频播放位置需要外部回传
        virtual void updateAudioClock(double pts) = 0;

        // 当有seek操作时，会调用回调
        // 由于native只负责解码出帧，清空外部缓存需要注册于此（比如外部音频缓存）
        virtual void setOnAudioSeekCallback(const OnSeekCallback& cb) = 0;
        // 由于native只负责解码出帧，清空外部缓存需要注册于此
        virtual void setOnVideoSeekCallback(const OnSeekCallback& cb) = 0;

        virtual void setOnPlaybackCompletedCallback(const OnPlaybackCompletedCallback& cb) = 0;
        [[nodiscard]] virtual bool isPlaybackCompleted() const = 0;

        virtual void setMasterMode(SyncType syncType) = 0;
        [[nodiscard]] virtual SyncType getMasterMode() const = 0;
    };
}