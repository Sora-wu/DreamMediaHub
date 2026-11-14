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
    {
        std::lock_guard lock(mtx_);
        isClose_ = true;
    }

    stopPull();
    cvBuffer_.notify_all();
}

void AudioBufferDevice::setPlayer(Dream::DreamPlayer* player) {
    player_ = player;

    player_->setOnAudioSeekCallback([this]{ onSeek(); });
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
    assert(player_);

    while (!st.stop_requested()) {
        // 如果audioFrame为空或者frame的数据已经被消耗完，取新frame
        if (!audioFrame_ || audioFrame_->len == 0) {
            audioFrame_ = player_->getAudioFrame();
        }

        if (!audioFrame_->isUpdated) {
            audioFrame_ = nullptr;
            continue;
        }

        {
            std::unique_lock lock(mtx_);
            cvPause_.wait(lock, [&] { return !isPause_; });
            cvBuffer_.wait(lock, [&]{ return dataLen_ < MAX_BUFFER_SIZE || st.stop_requested() || isClose_; });

            uint32_t consumedBytes = audioFrame_->len;
            consumedBytes = std::min(consumedBytes, MAX_BUFFER_SIZE - dataLen_);

            AudioSegment segment{};
            segment.pts = audioFrame_->pts;      // TODO: PTS更新放一放
            segment.serial = audioFrame_->serial;
            segment.data.assign(audioFrame_->data, audioFrame_->data + consumedBytes);
            segQue_.push_back(std::move(segment));
            dataLen_ += consumedBytes;

            memcpy(audioFrame_->data, audioFrame_->data + consumedBytes, audioFrame_->len - consumedBytes);
            audioFrame_->len -= static_cast<int>(consumedBytes);
        }

        // 有数据唤醒消费者
        cvBuffer_.notify_all();
        postReadyRead();
    }

    qInfo() << "audio buffer end";
}

void AudioBufferDevice::pause() {
    std::lock_guard lock(mtx_);
    isPause_ = true;
}

void AudioBufferDevice::play() {
    std::lock_guard lock(mtx_);
    if (!isPause_) {
        return;
    }
    isPause_ = false;
    cvPause_.notify_all();
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
    return cvBuffer_.wait_for(lock, prebufferMs, [&] {
        return dataLen_ >= targetBytes || st.stop_requested() || isClose_;
    }) && dataLen_ >= targetBytes && !isClose_;
}

void AudioBufferDevice::clear() {
    std::lock_guard lock(mtx_);
    segQue_.clear();
    dataLen_ = 0;
}

void AudioBufferDevice::onSeek() {
    std::lock_guard lock(mtx_);
    segQue_.clear();
    dataLen_ = 0;
    cvBuffer_.notify_all();

    audioFrame_ = nullptr;          // seek时，清一下audioFrame_，避免旧数据放入队列
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
    cvBuffer_.notify_all();
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
        pcmData[i] = std::clamp(sample, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
    }
}

void AudioBufferDevice::postReadyRead() {
    QMetaObject::invokeMethod(this, [this] {
        if (bytesAvailable() > 0) {
            emit readyRead();
        }
    }, Qt::QueuedConnection);
}
