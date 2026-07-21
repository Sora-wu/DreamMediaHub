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
    if (!audioDevice_) {
        return;
    }

    QByteArray data = audioDevice_->read(DreamRecorderContext::AUDIO_BUFFER_SIZE);
    if (data.isEmpty()) {
        return;
    }
    audioBuffer_.append(data);
    const uint32_t frameSize = format_.bytesPerFrame() / format_.channelCount();
    const uint32_t validBytes = (audioBuffer_.size() / frameSize) * frameSize;       // 取出采样的完整数据帧，避免杂音、噪音
    if (validBytes > 0) {
        qint64 now = elapsedTimer_.nsecsElapsed() / 1000;
        emit audioDataCaptured(audioBuffer_.constData(), validBytes, now);
        audioBuffer_.remove(0, validBytes);
    }
}
