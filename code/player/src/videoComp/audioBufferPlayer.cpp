//
// Author: sora
// Mail: sora-wu@foxmail.com
//


#include <videoComp/audioBufferPlayer.h>

AudioBufferDevice::AudioBufferDevice(QObject *parent)
{
    open(QIODevice::ReadWrite);
}

void AudioBufferDevice::writeAudioData(char *data, uint32_t len)
{

    buffer_.append(data, len);
}

void AudioBufferDevice::clear()
{
    std::lock_guard lock(mtx_);
    buffer_.clear();
}

qint64 AudioBufferDevice::readData(char *data, qint64 maxlen)
{
    std::unique_lock lock(mtx_);
    qint64 readLen = std::min(maxlen, buffer_.size());
    memcpy(data, buffer_.constData(), readLen);
    buffer_.remove(0, readLen);
    return readLen;
}

qint64 AudioBufferDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}