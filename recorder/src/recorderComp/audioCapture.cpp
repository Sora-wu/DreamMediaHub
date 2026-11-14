//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <recorderComp/audioCapture.h>
#include <DreamRecorder/DreamRecorderContext.h>

#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QDebug>

using namespace Dream;

AudioCapture::AudioCapture(QObject* parent) : QObject(parent) {
    QAudioDevice defaultDevice = QMediaDevices::defaultAudioInput();
    format_.setSampleRate(44100);
    format_.setChannelCount(2);
    format_.setSampleFormat(QAudioFormat::Int16);
    if (!defaultDevice.isFormatSupported(format_)) {
        qWarning() << "Default audio format not supported, using preferred format.";
        format_ = defaultDevice.preferredFormat();
    }

    audioSource_ = new QAudioSource(format_, this);

    elapsedTimer_.start();
    audioDevice_ = audioSource_->start();
    if (audioDevice_) {
        connect(audioDevice_, &QIODevice::readyRead, this,&AudioCapture::onReadyRead);
    }
    else {
        qWarning() << "Failed to start audio source!";
    }
}

AudioCapture::~AudioCapture() {
    if (audioSource_) {
        audioSource_->stop();
        delete audioSource_;
        audioSource_ = nullptr;
    }
}

void AudioCapture::onReadyRead() {
    static QByteArray audioBuffer;

    if (!audioDevice_) {
        return;
    }

    QByteArray data = audioDevice_->read(DreamRecorderContext::AUDIO_BUFFER_SIZE);
    if (data.isEmpty() && audioBuffer.isEmpty()) {
        return;
    }
    audioBuffer.append(data);
    const uint32_t frameSize = format_.bytesPerFrame() / format_.channelCount();
    const uint32_t validBytes = audioBuffer.size() / frameSize * frameSize;       // 取出采样的完整数据帧，避免杂音、噪音
    if (validBytes > 0) {
        const qint64 now = elapsedTimer_.nsecsElapsed() / 1000;
        const QByteArray sendBuffer{ audioBuffer.constData(), validBytes };
        emit audioDataCaptured(sendBuffer, now);                                   // 避免数据失效，因为是QueuedConnection，非线性在此期间会修改缓冲区,emit会考一份
        audioBuffer.remove(0, validBytes);
    }
}
