//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <recorderComp/audioCapture.h>
#include <QAudioSource>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QDebug>

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
    if (!audioDevice_) {
        return;
    }

    QByteArray data = audioDevice_->readAll();
    if (data.isEmpty()) {
        return;
    }

    qint64 now = elapsedTimer_.nsecsElapsed() / 1000;
    emit audioDataCaptured(data.constData(), static_cast<uint32_t>(data.size()), now);
}
