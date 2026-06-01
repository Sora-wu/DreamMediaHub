//
// Author: sora
// Mail: sora-wu@foxmail.com
//


#include <videoComp/audioBufferPlayer.h>

#include <DreamPlayer/DreamPlayer.h>
#include <DreamPlayer/structs.h>

#include <QDebug>

#include <cstring>

namespace {
    constexpr uint32_t MAX_BUFFER_SIZE = 1024 * 1024 * 50;
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
            cv_.wait(lock, [&](){ return dataLen + audioFrame.len <= MAX_BUFFER_SIZE || st.stop_requested(); });
            if (st.stop_requested()) {
                break;
            }

            AudioSegment segment{};
            segment.pts = audioFrame.pts;
            segment.data.assign(audioFrame.data, audioFrame.data + audioFrame.len);
            segQue_.push_back(std::move(segment));
            dataLen += audioFrame.len;

            // 有数据唤醒消费者
            cv_.notify_one();
        }
    }

    audioFrame.clear();
    qInfo() << "audio buffer end";
}

bool AudioBufferDevice::waitForBuffered(std::chrono::milliseconds prebufferMs, std::stop_token st) {
    constexpr uint64_t bytesPerSec = SAMPLE_RATE * CHANNEL_COUNT * SAMPLE_SIZE;
    const uint64_t targetBytes = bytesPerSec * static_cast<uint64_t>(prebufferMs.count()) / 1000;

    std::unique_lock lock(mtx_);
    return cv_.wait_for(lock, prebufferMs, [&]() {
        return dataLen >= targetBytes || st.stop_requested() || isClose_;
    }) && dataLen >= targetBytes && !isClose_;
}

void AudioBufferDevice::clear() {
    std::lock_guard lock(mtx_);
    segQue_.clear();
    dataLen = 0;
}

qint64 AudioBufferDevice::readData(char *data, qint64 maxlen) {
    std::unique_lock lock(mtx_);

    cv_.wait(lock, [this](){ return dataLen > 0 || !isClose_; });
    if (dataLen == 0 && isClose_) {
        return 0;
    }

    const qint64 readLen = std::min(maxlen, (qint64)dataLen);
    qint64 needReadLen = readLen;

    AudioSegment& currentSeg = segQue_.front();
    const double bytesPerSec = SAMPLE_RATE * CHANNEL_COUNT * SAMPLE_SIZE;
    const double currentPTS = currentSeg.pts + (double)((currentSeg.offset + readLen) / bytesPerSec);
    player_->updateAudioClock(currentPTS);

    while (needReadLen) {
        AudioSegment& currentSeg = segQue_.front();
        qint64 bufferSize = (qint64)currentSeg.data.size() - currentSeg.offset;
        if (needReadLen <= bufferSize) {
            memcpy(data, currentSeg.data.data() + currentSeg.offset, needReadLen);
            currentSeg.offset += needReadLen;
            if (currentSeg.data.size() == currentSeg.offset) {
                segQue_.pop_front();
            }
            break;
        }

        memcpy(data, currentSeg.data.data() + currentSeg.offset, bufferSize);
        data += bufferSize;
        needReadLen -= bufferSize;
        segQue_.pop_front();
    }

    dataLen -= readLen;

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
    return QIODevice::bytesAvailable() + dataLen;
}
