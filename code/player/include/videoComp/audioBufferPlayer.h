//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <QIODevice>
#include <QByteArray>
#include <QMutex>

#include <thread>
#include <mutex>
#include <condition_variable>

namespace Dream {
    class DreamPlayer;
}

class AudioBufferDevice : public QIODevice {
public:
    explicit AudioBufferDevice(QObject* parent = nullptr);
    void setPlayer(Dream::DreamPlayer* player) {
        player_ = player;
    }

    void startPull();
    void stopPull();
    void pull();

    void clear();

protected:
    qint64 readData(char* data, qint64 maxlen) override;

    // 禁用 writeData，数据由外部写入
    qint64 writeData(const char *data, qint64 len) override;

private:
    QByteArray buffer_{};

    std::mutex mtx_{};
    std::jthread thread_{};
    std::condition_variable cv_{};

    Dream::DreamPlayer* player_ = nullptr;
};
