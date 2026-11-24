//
// Author: sora
// Mail: sora-wu@foxmail.com
//


#include <videoComp/audioBufferDevice.h>

#include <QDebug>

#include <chrono>
#include <algorithm>

using namespace std::literals;

namespace {
    constexpr uint32_t MAX_BUFFER_SIZE = 1024 * 40;
    constexpr float MAX_FLOAT_VOLUME = 2.0f;
}

AudioBufferDevice::AudioBufferDevice(QObject *parent) : QIODevice(parent) {
    QIODevice::open(ReadOnly | Unbuffered);
}

AudioBufferDevice::~AudioBufferDevice() {
    stopPull();
}

void AudioBufferDevice::setPlayer(Dream::DreamPlayer* player) {
    player_ = player;

    player_->setOnSeekCallback([this](uint32_t serial){ onSeek(serial); });
}

void AudioBufferDevice::startPull() {
    thread_ = std::jthread([this](std::stop_token st) {
        pull(std::move(st));
    });
}

void AudioBufferDevice::stopPull() {
    {
        std::lock_guard lock(mtx_);
        isClose_ = true;
    }

    thread_.request_stop();
    cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void AudioBufferDevice::pull(std::stop_token st) {
    assert(player_);

    Dream::AudioFrame* audioFrame = nullptr;
    constexpr uint32_t bytesPerSample = CHANNEL_COUNT * SAMPLE_SIZE;
    uint32_t offset = 0;

    std::unique_lock lock(mtx_);
    while (!st.stop_requested()) {
        cv_.wait(lock, [&] {
            return isClose_ || (audioFrame && audioFrame->serial != currentAudioSerial_)
                    || (!isPause_ && MAX_BUFFER_SIZE - dataLen_ >= bytesPerSample);
        });

        if (st.stop_requested() || isClose_) {
            return;
        }
        if (audioFrame && audioFrame->serial != currentAudioSerial_) {
            audioFrame = nullptr;
            offset = 0;
            continue;
        }

        if (!audioFrame) {
            audioFrame = player_->getAudioFrame();

            offset = 0;

            if (!audioFrame || !audioFrame->isUpdated) {
                audioFrame = nullptr;
                const uint32_t currentSerial = currentAudioSerial_;
                // 避免cpu空转，也防止旧帧进入队列
                cv_.wait_for(lock, 5ms, [&] {
                    return isClose_ || st.stop_requested() || currentSerial != currentAudioSerial_;
                });
            }
            continue;
        }

        const uint32_t freeBytes = MAX_BUFFER_SIZE - dataLen_;
        uint32_t frameLen = audioFrame->len - offset;
        const uint32_t consumedBytes = std::min(frameLen, freeBytes - freeBytes % bytesPerSample);

        AudioSegment segment{};
        segment.pts = audioFrame->pts;      // TODO: PTS更新放一放
        segment.serial = audioFrame->serial;
        segment.data.assign(audioFrame->data + offset, audioFrame->data + offset + consumedBytes);
        segQue_.push_back(std::move(segment));
        dataLen_ += consumedBytes;
        offset += consumedBytes;

        if (audioFrame->len == offset) {
            audioFrame = nullptr;
        }

        // 有数据唤醒消费者
        lock.unlock();
        cv_.notify_all();
        postReadyRead();
        lock.lock();
    }

    qInfo() << "audio buffer end";
}

void AudioBufferDevice::pause() {
    std::lock_guard lock(mtx_);
    isPause_ = true;
}

void AudioBufferDevice::play() {
    {
        std::lock_guard lock(mtx_);
        isPause_ = false;
    }

    cv_.notify_all();
    postReadyRead();
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
    const uint64_t targetBytes = bytesPerSec * static_cast<uint64_t>(prebufferMs.count()) / 1000;

    std::unique_lock lock(mtx_);
    return cv_.wait_for(lock, prebufferMs, [&] {
        return dataLen_ >= targetBytes || st.stop_requested() || isClose_;
    }) && dataLen_ >= targetBytes && !isClose_;
}

void AudioBufferDevice::clear() {
    std::lock_guard lock(mtx_);
    segQue_.clear();
    dataLen_ = 0;
}

void AudioBufferDevice::onSeek(uint32_t serial) {
    {
        std::lock_guard lock(mtx_);
        segQue_.clear();
        dataLen_ = 0;
        currentAudioSerial_ = serial;
    }

    cv_.notify_all();
}

qint64 AudioBufferDevice::bytesAvailable() const {
    std::lock_guard lock(mtx_);
    if (isClose_ || isPause_) {
        return 0;
    }
    return QIODevice::bytesAvailable() + dataLen_;
}

qint64 AudioBufferDevice::readData(char *data, qint64 maxlen) {
    std::lock_guard lock(mtx_);

    if (isClose_) {
        return -1;
    }
    if (isPause_ || dataLen_ == 0 || maxlen <= 0) {
        return 0;
    }

    const qint64 readLen = std::min(maxlen, static_cast<qint64>(dataLen_));
    qint64 needReadLen = readLen;
    char* out = data;

    AudioSegment& currentSeg = segQue_.front();
    constexpr double bytesPerSec = SAMPLE_RATE * CHANNEL_COUNT * SAMPLE_SIZE;
    const double speed = player_->getSpeed();
    const double currentPTS = currentSeg.pts + (static_cast<double>(currentSeg.offset + readLen) / bytesPerSec) * speed;
    // 回传
    player_->updateAudioClock(currentPTS, currentSeg.serial);

    while (needReadLen) {
        AudioSegment& seg = segQue_.front();
        qint64 bufferSize = static_cast<qint64>(seg.data.size()) - seg.offset;
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
    cv_.notify_all();
    return readLen;
}

qint64 AudioBufferDevice::writeData(const char *data, qint64 len) {
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}
void AudioBufferDevice::scaleData(char* data, qint64 len) const {
    int16_t* pcmData = reinterpret_cast<int16_t*>(data);
    const uint32_t size = static_cast<uint32_t>(len / sizeof(int16_t));
    for (uint32_t i = 0; i < size; ++i) {
        int32_t sample = pcmData[i] * volume_;
        pcmData[i] = std::clamp(sample, INT16_MIN, INT16_MAX);
    }
}

void AudioBufferDevice::postReadyRead() {
    QMetaObject::invokeMethod(this, [this] {
        if (bytesAvailable() > 0) {
            emit readyRead();
        }
    }, Qt::QueuedConnection);
}