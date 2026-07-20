//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QObject>

namespace Dream {
    class DreamRecorder;
}

class QThread;
class CameraCapture;
class VideoWorker;
class AudioCapture;
class AudioWorker;

class RecorderController : public QObject {
    Q_OBJECT

public:
    explicit RecorderController(QObject* parent = nullptr) {}

    Q_INVOKABLE void start(const QString& path);
    Q_INVOKABLE void stop();

private:
    Dream::DreamRecorder* recorder_ = nullptr;

    CameraCapture* cameraCapture_ = nullptr;
    VideoWorker* videoWorker_ = nullptr;
    QThread* videoThread_ = nullptr;

    AudioCapture* audioCapture_ = nullptr;
    AudioWorker* audioWorker_ = nullptr;
    QThread* audioThread_ = nullptr;
};
