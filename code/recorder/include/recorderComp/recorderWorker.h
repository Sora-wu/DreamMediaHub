//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QObject>

namespace Dream {
    class DreamRecorder;
}

class VideoWorker : public QObject {
    Q_OBJECT
public:
    VideoWorker(Dream::DreamRecorder* recorder, uint32_t fps = 30, QObject* parent = nullptr)
        : QObject(parent), recorder_(recorder), fps_(fps) {}
    ~VideoWorker() override;

public slots:
    void onImageCaptured(const QImage &image, qint64 pts);
    void stopRecording();

private:
    Dream::DreamRecorder* recorder_ = nullptr;
    uint32_t fps_ = 30;
    int64_t frameCount_ = 0;
    bool isRecording_ = true;
};

class AudioWorker : public QObject {
    Q_OBJECT
public:
    AudioWorker(Dream::DreamRecorder* recorder, QObject* parent = nullptr)
        : QObject(parent), recorder_(recorder) {}
    ~AudioWorker() override;

public slots:
    void onAudioDataCaptured(const char* data, uint32_t size, uint64_t pts) const;
    void stopRecording();

private:
    Dream::DreamRecorder* recorder_ = nullptr;
    bool isRecording_ = true;
};