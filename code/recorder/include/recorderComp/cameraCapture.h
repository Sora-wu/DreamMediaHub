//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QVideoSink>
#include <QElapsedTimer>

class QCamera;
class QMediaCaptureSession;


class CameraCapture : public QObject {
    Q_OBJECT

public:
    explicit CameraCapture(QObject *parent = nullptr, uint32_t fps = 30);
    ~CameraCapture() override;

    [[nodiscard]] QSize getCameraResolution() const;

signals:
    void imageCaptured(const QImage& image);

private slots:
    void onFrameChanged(const QVideoFrame& frame);

private:
    QCamera* camera_ = nullptr;
    QMediaCaptureSession* captureSession_ = nullptr;
    QVideoSink* videoSink_ = nullptr;

    uint32_t fps_ = 30;
    qint64 lastEmitTime_ = 0;
    QElapsedTimer elapsedTimer_{};
};
