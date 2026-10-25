//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <recorderComp/recorderWorker.h>
#include <DreamRecorder/DreamRecorder.h>

#include <QImage>

using namespace Dream;

VideoWorker::~VideoWorker() {
    stopRecording();
}

void VideoWorker::onImageCaptured(const QImage& image, qint64 pts) {
    if (!isRecording_) {
        return;
    }

    VideoFrameBuffer buffer = recorder_->getVideoBuffer();
    assert(buffer.data);

    const uint32_t bytesPerLine = image.bytesPerLine();
    const uint32_t size = static_cast<uint32_t>(image.sizeInBytes());

    // 如果不一致，那么逐行拷贝
    if (buffer.size != size || buffer.stride != bytesPerLine) {
        char* dst = buffer.data;
        for (uint32_t i = 0; i < static_cast<uint32_t>(image.height()); ++i) {
            memcpy(dst, image.constScanLine(i), bytesPerLine);
            dst += buffer.stride;
        }
    }
    else {
        memcpy(buffer.data, image.constBits(), size);
    }

    // 提交
    recorder_->commitVideoBuffer(buffer, pts);
}

void VideoWorker::stopRecording() {
    isRecording_ = false;
}

//////////////////////////////////VideoWorker end//////////////////////////////////////////////
AudioWorker::~AudioWorker() {
    stopRecording();
}

void AudioWorker::onAudioDataCaptured(const char* data, uint32_t size, uint64_t pts) const {
    recorder_->pushAudioData(data, size, pts);
}

void AudioWorker::stopRecording() {
    isRecording_ = false;
}
