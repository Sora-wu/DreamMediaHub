//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <recorderComp/recorderController.h>
#include <DreamRecorder/DreamRecorderContext.h>
#include <recorderComp/recorderWorker.h>

#include <QSize>
#include <QThread>

using namespace Dream;

void RecorderController::start(const QString& path) {
    screenCapture_ = new CameraCapture(this, 30);
    const QSize size = screenCapture_->getCameraResolution();
    VideoParam videoParam{};
    videoParam.width = size.width();
    videoParam.height = size.height();
    recorder_ = DreamRecorderContext::createDreamRecorder();
    recorder_->start(path.toLocal8Bit(), videoParam, AudioParam{});

    workThread_ = new QThread(this);
    recorderWorker_ = new RecorderWorker(recorder_, 30);
    recorderWorker_->moveToThread(workThread_);
    connect(screenCapture_, &CameraCapture::imageCaptured, recorderWorker_, &RecorderWorker::onImageCaptured, Qt::QueuedConnection);
    connect(workThread_, &QThread::finished, recorderWorker_, &QObject::deleteLater);
    workThread_->start();
}

void RecorderController::stop() {
    workThread_->exit();
    workThread_->wait();

    recorder_->stop();
    DreamRecorderContext::destroyDreamRecorder(&recorder_);
}
