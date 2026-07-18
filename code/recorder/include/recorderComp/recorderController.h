//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <recorderComp/cameraCapture.h>

namespace Dream {
    class DreamRecorder;
}

class QThread;
class RecorderWorker;

class RecorderController : public QObject {
    Q_OBJECT

public:
    explicit RecorderController(QObject* parent = nullptr) {}

    Q_INVOKABLE void start(const QString& path);
    Q_INVOKABLE void stop();

private:
    Dream::DreamRecorder* recorder_ = nullptr;
    CameraCapture* screenCapture_ = nullptr;
    RecorderWorker* recorderWorker_ = nullptr;
    QThread* workThread_ = nullptr;
};
