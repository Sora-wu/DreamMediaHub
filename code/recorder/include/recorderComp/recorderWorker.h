//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QObject>

namespace Dream {
    class DreamRecorder;
}

class RecorderWorker : public QObject {
    Q_OBJECT
public:
    RecorderWorker(Dream::DreamRecorder* recorder, uint32_t fps = 30, QObject* parent = nullptr)
        : QObject(parent), recorder_(recorder), fps_(fps) {}
    ~RecorderWorker() override;

public slots:
    void onImageCaptured(const QImage &image, qint64 pts);
    void stopRecording();

signals:
    void finished();
    void errorOccurred(const QString &msg);

private:
    Dream::DreamRecorder* recorder_ = nullptr;
    uint32_t fps_ = 30;
    int64_t frameCount_ = 0;
    bool isRecording_ = true;
};
