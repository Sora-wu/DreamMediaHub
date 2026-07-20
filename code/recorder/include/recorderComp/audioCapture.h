//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QElapsedTimer>
#include <QAudioFormat>

class QAudioSource;
class QIODevice;

class AudioCapture : public QObject {
    Q_OBJECT

public:
    explicit AudioCapture(QObject* parent = nullptr);
    ~AudioCapture() override;

    QAudioFormat getAudioFormat() const {
        return format_;
    }

signals:
    void audioDataCaptured(const char* data, uint32_t size, uint64_t pts);

private slots:
    void onReadyRead();

private:
    QAudioSource* audioSource_ = nullptr;
    QIODevice* audioDevice_ = nullptr;
    QElapsedTimer elapsedTimer_;
    QAudioFormat format_{};
};
