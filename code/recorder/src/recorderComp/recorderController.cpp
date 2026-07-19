//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <recorderComp/recorderController.h>
#include <DreamRecorder/DreamRecorderContext.h>
#include <recorderComp/recorderWorker.h>

#include <QSize>
#include <QThread>

namespace {
    constexpr uint32_t FPS = 30;
}

using namespace Dream;

void RecorderController::start(const QString& path) {
    screenCapture_ = new CameraCapture(this, FPS);
    const QSize size = screenCapture_->getCameraResolution();
    VideoParam videoParam{};
    videoParam.width = size.width();
    videoParam.height = size.height();
    videoParam.fps = FPS;
    recorder_ = DreamRecorderContext::createDreamRecorder();
    recorder_->start(path.toLocal8Bit(), videoParam, AudioParam{});

    workThread_ = new QThread(this);
    recorderWorker_ = new RecorderWorker(recorder_, FPS);
    recorderWorker_->moveToThread(workThread_);
    connect(screenCapture_, &CameraCapture::imageCaptured, recorderWorker_, &RecorderWorker::onImageCaptured, Qt::QueuedConnection);
    connect(workThread_, &QThread::finished, recorderWorker_, &QObject::deleteLater);
    workThread_->start();
}

void RecorderController::stop() {
    if (screenCapture_) {
        delete screenCapture_;
        screenCapture_ = nullptr;
    }

    if (workThread_) {
        workThread_->exit();
        workThread_->wait();
        delete workThread_;
        workThread_ = nullptr;
    }

    if (recorder_) {
        recorder_->stop();
        DreamRecorderContext::destroyDreamRecorder(&recorder_);
    }
}
