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

    [[nodiscard]] bool waitForBuffered(std::chrono::milliseconds prebufferMs, std::stop_token st = {});

    void clear();

protected:
    qint64 readData(char* data, qint64 maxlen) override;

    // 禁用 writeData，数据由外部写入
    qint64 writeData(const char *data, qint64 len) override;

    qint64 bytesAvailable() const override;

private:
    void clearQueue();

private:    
    std::deque<AudioSegment> segQue_{};
    uint32_t dataLen{};

    mutable std::mutex mtx_{};
    std::jthread thread_{};
    std::condition_variable cv_{};
    bool isClose_ = false;

    Dream::DreamPlayer* player_ = nullptr;
};
