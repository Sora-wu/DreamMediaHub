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

PlayerController::PlayerController(QObject* parent) {
    player_ = DreamContext::createDreamPlayer();

    QAudioFormat format;
    format.setSampleRate(AudioBufferDevice::SAMPLE_RATE);
    format.setChannelCount(AudioBufferDevice::CHANNEL_COUNT);
    format.setSampleFormat(QAudioFormat::Int16);
    QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();
    if (!audioDevice.isFormatSupported(format)) {
        qInfo() << "sample rate: " << format.sampleRate();
        qInfo() << "channel count: " << format.channelCount();
        qWarning() << "can not support this audio format or detect any audio device, will play with no audio";
    }
    audioBufferDevice_ = new AudioBufferDevice;
    audioSink_ = new QAudioSink(audioDevice, format);

    initController();
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
    return player_->getDuration();
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

void PlayerController::initController() {
    // TEST
#ifdef PLATFORM_LINUX
    player_->open("/home/sora/Videos/test.mp4");
#else
    player_->open("D:/Desktop/Test/test31.mp4");
#endif

    audioBufferDevice_->setPlayer(player_);

    audioBufferDevice_->startPull();
    // 预缓存
    if (!audioBufferDevice_->waitForBuffered(std::chrono::milliseconds(800))) {
        qWarning() << "prebuffer timeout";
    }
    audioSink_->setBufferSize(DreamContext::AUDIO_BUFFER_SIZE);
    audioSink_->start(audioBufferDevice_);

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &PlayerController::onTimerUpdate);
    timer_->start(500);
}

void PlayerController::onTimerUpdate() {
    if (!player_) {
        return;
    }

    uint32_t cur = static_cast<uint32_t>(player_->getCurrentTime());
    uint32_t dur = player_->getDuration();

    QString newText = QString::number(cur) + "/" + QString::number(dur);

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
