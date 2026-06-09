//
// Author: sora
// Mail: sora-wu@foxmail.com
//


#include <videoComp/playerController.h>
#include <videoComp/audioBufferPlayer.h>

#include <QAudioSink>
#include <QMediaDevices>
#include <QDebug>

#include <DreamPlayer/DreamContext.h>

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
}

PlayerController::PlayerController(QObject* parent) {
    player_ = DreamContext::createDreamPlayer();
    // TEST
#ifdef PLATFORM_LINUX
    player_->open("/home/sora/Videos/test.mp4");
#else
    player_->open("D:/Desktop/Test/test31.mp4");
#endif

    QAudioFormat format;
    format.setSampleRate(AudioBufferDevice::SAMPLE_RATE);
    format.setChannelCount(AudioBufferDevice::CHANNEL_COUNT);
    format.setSampleFormat(QAudioFormat::Int16);
    QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();
    if (!audioDevice.isFormatSupported(format)) {
        qInfo() << "sample rate: " << format.sampleRate();
        qInfo() << "channel count: " << format.channelCount();
        qWarning() << "can not support this audio format or detect any audio device, will play with no audio";

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
    timer_->start(500);

    duration_ = player_->getDuration();
    progressText_ = QString("%1/%2").arg(formatTimeFromSeconds(0), formatTimeFromSeconds(duration_));
}

PlayerController::~PlayerController() {
    delete audioBufferDevice_;
    delete audioSink_;

    DreamContext::destroyDreamPlayer(&player_);
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
    return player_->getVolume();
}

void PlayerController::setVolume(uint32_t newVolume)
{
    player_->setVolume(newVolume);
    emit volumeChanged();
}

uint32_t PlayerController::duration() const
{
    return duration_;
}

uint32_t PlayerController::currentProgress() const
{
    return (uint32_t)player_->getCurrentTime();
}

void PlayerController::play() {
    if (player_) {
        player_->play();
        isPlaying_ = true;
        emit isPlayingChanged();
    }
}

void PlayerController::pause() {
    if (player_) {
        player_->pause();
        isPlaying_ = false;
        emit isPlayingChanged();
    }
}

void PlayerController::togglePlay() {
    isPlaying_ ? pause() : play();
}

void PlayerController::seek(double position) {
    if (player_) {
        player_->seek(position);
        // 手动调用一下
        onTimerUpdate();
    }
}

void PlayerController::onTimerUpdate() {
    if (!player_) {
        return;
    }

    uint32_t cur = (uint32_t)player_->getCurrentTime();
    QString newText = QString("%1/%2").arg(formatTimeFromSeconds(cur), formatTimeFromSeconds(duration_));

    if (progressText_ != newText) {
        progressText_ = newText;
        emit progressTextChanged();
    }

    if (currentProgress_ != cur) {
        currentProgress_ = cur;
        emit currentProgressChanged();
    }

    // emit durationChanged();
}
