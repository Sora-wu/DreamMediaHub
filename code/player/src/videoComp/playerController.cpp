//
// Author: sora
// Mail: sora-wu@foxmail.com
//


#include <videoComp/playerController.h>
#include <videoComp/audioBufferPlayer.h>
#include <DreamPlayer/DreamContext.h>

#include <QAudioSink>
#include <QMediaDevices>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

using namespace Dream;

namespace {
    QString formatTimeFromSeconds(int totalSeconds) {
        if (totalSeconds < 0) {
            return "00:00:00";
        }

        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;

        QString formattedTime = QString("%1:%2:%3")
                                .arg(hours, 2, 10, QChar('0'))
                                .arg(minutes, 2, 10, QChar('0'))
                                .arg(seconds, 2, 10, QChar('0'));

        return formattedTime;
    }

    double roundToOneDecimal(double value) {
        return std::round(value * 10.0) / 10.0;
    }
}

PlayerController::PlayerController(QObject* parent) {
    player_ = DreamContext::createDreamPlayer();
    // TEST
#ifdef PLATFORM_LINUX
    open("/home/sora/Videos/test_bak.mp4");
#else
    open("D:/Desktop/Test/test56.mp4");
#endif
    isPlaying_ = true;

    QAudioFormat format;
    format.setSampleRate(AudioBufferDevice::SAMPLE_RATE);
    format.setChannelCount(AudioBufferDevice::CHANNEL_COUNT);
    format.setSampleFormat(QAudioFormat::Int16);
    QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();
    if (!audioDevice.isFormatSupported(format)) {
        qInfo() << "sample rate: " << format.sampleRate();
        qInfo() << "channel count: " << format.channelCount();
        qWarning() << "can not support this audio format or detect any audio device, will play without audio";

        player_->setMasterMode(SyncType::VideoMaster);
    }
    else {
        audioBufferDevice_ = new AudioBufferDevice;
        audioSink_ = new QAudioSink(audioDevice, format);
        audioBufferDevice_->setPlayer(player_);
        audioBufferDevice_->startPull();
        // 预缓存
        if (!audioBufferDevice_->waitForBuffered(std::chrono::milliseconds(800))) {
            qWarning() << "prebuffer timeout";
        }
        audioSink_->setBufferSize(DreamContext::AUDIO_BUFFER_SIZE);
        audioSink_->start(audioBufferDevice_);
    }

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &PlayerController::onTimerUpdate);
    timer_->start(250);

    duration_ = player_->getDuration();
    progressText_ = QString("%1/%2").arg(formatTimeFromSeconds(0), formatTimeFromSeconds(duration_));

    player_->setOnPlaybackCompletedCallback([this] {
        QMetaObject::invokeMethod(this, &PlayerController::onPlaybackCompleted, Qt::QueuedConnection);
    });
}

QString PlayerController::mediaName() const
{
    return mediaName_;
}

QString PlayerController::progressText() const
{
    return progressText_;
}

bool PlayerController::isPlaying() const
{
    return isPlaying_;
}

uint32_t PlayerController::volume() const
{
    return volume_;
}

uint32_t PlayerController::duration() const
{
    return duration_;
}

double PlayerController::speed() const {
    return speed_;
}

void PlayerController::setSpeed(double speed) {
    if (speed_ != speed) {
        speed_ = roundToOneDecimal(speed);
        qInfo() << "speed:" << speed_;
        player_->setSpeed(speed);
        emit speedChanged();
    }
}

uint32_t PlayerController::currentProgress() const
{
    return currentProgress_;
}

void PlayerController::setCurrentProgress(uint32_t newCurrentProgress)
{
    if (currentProgress_ == newCurrentProgress)
        return;
    qInfo() << "new currentProgress" << newCurrentProgress;
    currentProgress_ = newCurrentProgress;
    emit currentProgressChanged();
}

void PlayerController::open(const QString& path) {
    if (player_) {
        player_->open(path.toLocal8Bit().constData());
        const QFileInfo fileInfo(path);
        mediaName_ = fileInfo.fileName();

        isPlaying_ = false;
    }
}

void PlayerController::play() {
    if (player_) {
        player_->play();
        audioBufferDevice_->play();
        isPlaying_ = true;
        emit isPlayingChanged();
    }
}

void PlayerController::pause() {
    if (player_) {
        player_->pause();
        audioBufferDevice_->pause();
        isPlaying_ = false;
        emit isPlayingChanged();
    }
}

void PlayerController::togglePlay() {
    isPlaying_ ? pause() : play();
}

void PlayerController::seek(int second, bool isRelative) {
    if (!player_) {
        return;
    }

    if (!seekActive_) {
        seekTarget_ = player_->getCurrentTime();
    }

    seekActive_ = true;
    seekTarget_ = isRelative ? std::clamp(seekTarget_ + second, 0.0, (double)duration_) : second;

    play();
    audioBufferDevice_->clearQueue();
    player_->seek(seekTarget_, second);

    const uint32_t targetProgress = (uint32_t)seekTarget_;
    if (currentProgress_ != targetProgress) {
        currentProgress_ = targetProgress;
        emit currentProgressChanged();
    }

    if (!seekTimer_) {
        seekTimer_ = new QTimer(this);
        seekTimer_->setSingleShot(true);
        seekTimer_->setInterval(300);
        connect(seekTimer_, &QTimer::timeout, this, [this]() {
            seekActive_ = false;
        });
    }

    seekTimer_->start();
}

void PlayerController::setVolume(uint32_t newVolume)
{
    if (!audioBufferDevice_ || volume_ == newVolume)
        return;
    audioBufferDevice_->setVolume((float)newVolume / 100);
    volume_ = newVolume;
    emit volumeChanged();
}

void PlayerController::cleanup() {
    player_->stop();

    if (timer_) {
        timer_->stop();
        delete timer_;
        timer_ = nullptr;
    }

    if (audioBufferDevice_) {
        delete audioBufferDevice_;
        audioBufferDevice_ = nullptr;
    }

    if (audioSink_) {
        audioSink_->stop();
        delete audioSink_;
        audioSink_ = nullptr;
    }

    DreamContext::destroyDreamPlayer(&player_);
}

void PlayerController::onTimerUpdate() {
    if (!player_) {
        return;
    }

    const uint32_t cur = seekActive_ ? (uint32_t)seekTarget_ : (uint32_t)player_->getCurrentTime();
    QString newText = QString("%1/%2").arg(formatTimeFromSeconds(cur), formatTimeFromSeconds(duration_));

    if (progressText_ != newText) {
        progressText_ = newText;
        emit progressTextChanged();
    }

    if (!seekActive_ && currentProgress_ != cur) {
        currentProgress_ = cur;
        emit currentProgressChanged();
    }
}

void PlayerController::onPlaybackCompleted() {
    if (!isPlaying_) {
        onTimerUpdate();
        return;
    }

    isPlaying_ = false;
    emit isPlayingChanged();
    onTimerUpdate();
}
