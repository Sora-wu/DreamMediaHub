//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamRecorder/DreamRecorder.h>
#include <FFmpegRecorder/muxer.h>
#include <FFmpegRecorder/videoEncoder.h>
#include <FFmpegRecorder/audioEncoder.h>

#include <memory>

class FFmpegRecorder : public Dream::DreamRecorder {
public:
    ~FFmpegRecorder() override {};

    void start(const char* path, const Dream::VideoParam& videoParam, const Dream::AudioParam& audioParam) override;
    void stop() override;

    Dream::VideoFrameBuffer getVideoBuffer() override;
    void commitVideoBuffer(const Dream::VideoFrameBuffer& buffer, uint64_t pts) override;
    void pushAudioData(char* data, uint32_t size, uint64_t pts) override;

private:
    std::unique_ptr<Muxer> mux_;
    std::unique_ptr<VideoEncoder> videoEncoder_;
    std::unique_ptr<AudioEncoder> audioEncoder_;
};
