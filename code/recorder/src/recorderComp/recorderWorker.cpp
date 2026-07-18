//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <recorderComp/recorderWorker.h>
#include <DreamRecorder/DreamRecorder.h>

#include <QImage>

using namespace Dream;

RecorderWorker::~RecorderWorker() {
    stopRecording();
}

void RecorderWorker::onImageCaptured(const QImage& image) {
    if (!isRecording_) {
        return;
    }

    VideoFrameBuffer buffer = recorder_->getVideoBuffer();
    assert(buffer.data);

    const uint32_t bytesPerLine = image.bytesPerLine();
    const uint32_t size = image.sizeInBytes();

    // 如果不一致，那么逐行拷贝
    if (buffer.size != size || buffer.stride != bytesPerLine) {
        char* dst = buffer.data;
        for (uint32_t i = 0; i < image.height(); ++i) {
            memcpy(dst, image.constScanLine(i), bytesPerLine);
            dst += buffer.stride;
        }
    }
    else {
        memcpy(buffer.data, image.constBits(), size);
    }

    // 提交
    const uint32_t FRAME_DURATION = 1e6 / fps_;
    const int64_t pts = frameCount_ * FRAME_DURATION;
    recorder_->commitVideoBuffer(buffer, pts);
}

void RecorderWorker::stopRecording() {
    isRecording_ = false;
}
