//
// Author: sora
// Mail: sora-wu@foxmail.com
//


#include <videoComp/audioBufferPlayer.h>

#include <DreamPlayer/DreamPlayer.h>
#include <DreamPlayer/structs.h>

#include <QDebug>

namespace {
    constexpr uint32_t MAX_BUFFER_SIZE = 1024 * 1024 * 50;
}

AudioBufferDevice::AudioBufferDevice(QObject *parent) : QIODevice(parent) {
    QIODevice::open(ReadWrite);
}

AudioBufferDevice::~AudioBufferDevice() {
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

        {
            std::unique_lock lock(mtx_);
            cv_.wait(lock, [&](){ return buffer_.size() + audioFrame.len <= MAX_BUFFER_SIZE || st.stop_requested(); });
            if (st.stop_requested()) {
                break;
            }

            buffer_.insert(buffer_.end(), audioFrame.data, audioFrame.data + audioFrame.len);
        }
    }

    audioFrame.clear();
    qInfo() << "audio buffer end";
}

void AudioBufferDevice::clear() {
    std::lock_guard lock(mtx_);
    buffer_.clear();
}

qint64 AudioBufferDevice::readData(char *data, qint64 maxlen) {
    std::unique_lock lock(mtx_);

    const qint64 readLen = std::min(maxlen, (qint64)buffer_.size());
    std::copy_n(buffer_.begin(), readLen, data);
    buffer_.erase(buffer_.begin(), buffer_.begin() + readLen);

    cv_.notify_one();
    return readLen;
}

qint64 AudioBufferDevice::writeData(const char *data, qint64 len) {
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 AudioBufferDevice::bytesAvailable() const {
    return buffer_.size();
}
