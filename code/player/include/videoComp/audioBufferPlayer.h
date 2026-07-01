//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QIODevice>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>

namespace Dream {
    class DreamPlayer;
}

class AudioBufferDevice final : public QIODevice {
    struct AudioSegment {
        std::vector<char> data;
        double pts{};
        uint32_t serial{};
        uint32_t offset{};
    };

public:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNEL_COUNT = 2;
    static constexpr int SAMPLE_SIZE = 2;

    explicit AudioBufferDevice(QObject* parent = nullptr);
    ~AudioBufferDevice() override;

    void setPlayer(Dream::DreamPlayer* player);

    void startPull();
    void stopPull();
    void pull(std::stop_token);

    // 暂停播放音频
    void pause();
    // 继续播放音频
    void play();

    // 音量范围【0-3.0】
    void setVolume(float volume);
    float getVolume() const;

    [[nodiscard]] bool waitForBuffered(std::chrono::milliseconds prebufferMs, std::stop_token st = {});

    void clear();
    void clearQueue();

protected:
    qint64 readData(char* data, qint64 maxlen) override;
    // 禁用 writeData，数据由外部写入
    qint64 writeData(const char *data, qint64 len) override;

    qint64 bytesAvailable() const override;

private:
    void scaleData(char* data, qint64 len) const;

private:    
    std::deque<AudioSegment> segQue_{};
    uint32_t dataLen_{};

    mutable std::mutex mtx_{};
    std::jthread thread_{};
    std::condition_variable cv_{};
    bool isClose_ = false;
    bool isPause_ = false;          // 因为native层仅负责编解码，因为音频有缓冲，所以暂停操作需要由上层配合
    float volume_ = 1.0f;

    Dream::DreamPlayer* player_ = nullptr;
};
