//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <recorderComp/cameraCapture.h>

#include <QCamera>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QDebug>


CameraCapture::CameraCapture(QObject* parent, uint32_t fps) : QObject(parent), fps_(fps) {
    QCameraDevice defaultDevice = QMediaDevices::defaultVideoInput();
    camera_ = new QCamera(defaultDevice, this);
    QList<QCameraFormat> formats = defaultDevice.videoFormats();
    if (!formats.isEmpty()) {
        camera_->setCameraFormat(formats.first());
    }
    captureSession_ = new QMediaCaptureSession(this);
    videoSink_ = new QVideoSink(this);

    captureSession_->setCamera(camera_);
    captureSession_->setVideoSink(videoSink_);

    connect(videoSink_, &QVideoSink::videoFrameChanged, this, &CameraCapture::onFrameChanged);

    elapsedTimer_.start();
    camera_->start();
}

CameraCapture::~CameraCapture() {
    if (camera_) {
        camera_->stop();
        delete camera_;
        camera_ = nullptr;
    }

    if (captureSession_) {
        delete captureSession_;
        captureSession_ = nullptr;
    }

    if (videoSink_) {
        delete videoSink_;
        videoSink_ = nullptr;
    }

    lastEmitTime_ = 0;
}

QSize CameraCapture::getCameraResolution() const {
    if (camera_) {
        return camera_->cameraFormat().resolution();
    }

    return QSize{};
}

void CameraCapture::onFrameChanged(const QVideoFrame& frame) {
    const qint64 interval = 1000 / fps_;
    qint64 now = elapsedTimer_.nsecsElapsed() / 1000;

    if (lastEmitTime_ && now - lastEmitTime_ < interval) {
        return;
    }

    lastEmitTime_ = now;

    QVideoFrame cloneFrame{ frame };
    if (!cloneFrame.map(QVideoFrame::ReadOnly)) {
        return;
    }

    QImage image = cloneFrame.toImage();
    if (!image.isNull()) {
        if (image.format() != QImage::Format_RGB888) {
            image.convertTo(QImage::Format_RGB888);
        }

        QImage result = image.copy();
        emit imageCaptured(result, now);
    }

    cloneFrame.unmap();
}
