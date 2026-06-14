//
// Author: sora
// Mail: sora-wu@foxmail.com
//


#include <videoComp/audioBufferPlayer.h>

#include <DreamPlayer/DreamPlayer.h>
#include <QDebug>

#include <algorithm>

namespace {
    constexpr uint32_t MAX_BUFFER_SIZE = 1024 * 1024 * 50;
    constexpr float MAX_FLOAT_VOLUME = 2.0f;
}

AudioBufferDevice::AudioBufferDevice(QObject *parent) : QIODevice(parent) {
    QIODevice::open(ReadWrite);
}

AudioBufferDevice::~AudioBufferDevice() {
    {
        std::lock_guard lock(mtx_);
        isClose_ = true;
    }

    stopPull();
    cv_.notify_all();
}

void AudioBufferDevice::setPlayer(Dream::DreamPlayer* player) {
    player_ = player;

    player_->setOnAudioSeekCallback([this]{ clearQueue(); });
}

void AudioBufferDevice::startPull() {
    thread_ = std::jthread([this](std::stop_token st) {
        pull(std::move(st));
    });
}

void AudioBufferDevice::stopPull() {
    thread_.request_stop();
}

void AudioBufferDevice::pull(std::stop_token st) {
    Dream::AudioFrame audioFrame{};
    assert(player_);

    while (!st.stop_requested()) {
        player_->getAudioFrame(&audioFrame);
        if (!audioFrame.isUpdated) {
            continue;
        }

        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [&](){ return dataLen_ + audioFrame.len <= MAX_BUFFER_SIZE || st.stop_requested(); });
            if (st.stop_requested()) {
                break;
            }

            AudioSegment segment{};
            segment.pts = audioFrame.pts;
            segment.data.assign(audioFrame.data, audioFrame.data + audioFrame.len);
            segQue_.push_back(std::move(segment));
            dataLen_ += audioFrame.len;

            // 有数据唤醒消费者
            cv_.notify_one();
        }
    }

    audioFrame.clear();
    qInfo() << "audio buffer end";
}

void AudioBufferDevice::pause() {
    std::lock_guard lock(mtx_);
    isPause_ = true;
}

void AudioBufferDevice::play() {
    std::lock_guard lock(mtx_);
    isPause_ = false;
}

void AudioBufferDevice::setVolume(float volume) {
    std::lock_guard lock(mtx_);
    volume_ = std::clamp(volume, 0.0f, MAX_FLOAT_VOLUME);
}

float AudioBufferDevice::getVolume() const {
    std::lock_guard lock(mtx_);
    return volume_;
}

bool AudioBufferDevice::waitForBuffered(std::chrono::milliseconds prebufferMs, std::stop_token st) {
    constexpr uint64_t bytesPerSec = SAMPLE_RATE * CHANNEL_COUNT * SAMPLE_SIZE;
    const uint64_t targetBytes = bytesPerSec * (uint64_t)(prebufferMs.count()) / 1000;

    std::unique_lock lock(mtx_);
    return cv_.wait_for(lock, prebufferMs, [&]() {
        return dataLen_ >= targetBytes || st.stop_requested() || isClose_;
    }) && dataLen_ >= targetBytes && !isClose_;
}

void AudioBufferDevice::clear() {
    std::lock_guard lock(mtx_);
    segQue_.clear();
    dataLen_ = 0;
}

qint64 AudioBufferDevice::readData(char *data, qint64 maxlen) {
    std::unique_lock lock(mtx_);

    cv_.wait(lock, [this](){ return (dataLen_ > 0 && !segQue_.empty()) || !isClose_; });
    if ((dataLen_ == 0 && segQue_.empty() && isClose_) || isPause_) {
        return 0;
    }

    const qint64 readLen = std::min(maxlen, (qint64)dataLen_);
    qint64 needReadLen = readLen;
    char* out = data;

    AudioSegment& currentSeg = segQue_.front();
    constexpr double bytesPerSec = SAMPLE_RATE * CHANNEL_COUNT * SAMPLE_SIZE;
    const double currentPTS = currentSeg.pts + ((double)(currentSeg.offset + readLen) / bytesPerSec);
    player_->updateAudioClock(currentPTS);

    while (needReadLen) {
        AudioSegment& seg = segQue_.front();
        qint64 bufferSize = (qint64)seg.data.size() - seg.offset;
        if (needReadLen <= bufferSize) {
            memcpy(data, seg.data.data() + seg.offset, needReadLen);
            seg.offset += needReadLen;
            if (seg.data.size() == seg.offset) {
                segQue_.pop_front();
            }
            break;
        }

        memcpy(data, seg.data.data() + seg.offset, bufferSize);
        data += bufferSize;
        needReadLen -= bufferSize;
        segQue_.pop_front();
    }
    dataLen_ -= readLen;

    scaleData(out, readLen);

    // 数据减少，唤醒生产者
    cv_.notify_one();
    return readLen;
}

qint64 AudioBufferDevice::writeData(const char *data, qint64 len) {
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 AudioBufferDevice::bytesAvailable() const {
    std::lock_guard lock(mtx_);
    return QIODevice::bytesAvailable() + dataLen_;
}

void AudioBufferDevice::scaleData(char* data, qint64 len) const {
    int16_t* pcmData = reinterpret_cast<int16_t*>(data);
    const uint32_t size = len / sizeof(int16_t);
    for (uint32_t i = 0; i < size; ++i) {
        int32_t sample = pcmData[i] * volume_;
        pcmData[i] = std::clamp(sample, INT16_MIN, INT16_MAX);
    }
}

void AudioBufferDevice::clearQueue() {
    std::lock_guard lock(mtx_);
    segQue_.clear();
    dataLen_ = 0;
    cv_.notify_all();
}
