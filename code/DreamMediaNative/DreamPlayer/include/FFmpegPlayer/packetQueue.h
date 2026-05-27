//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <common/concurrentQueue.hpp>
#include <FFmpegPlayer/ffcore.h>

struct Packet {
    AVPacket packet{};
    uint32_t serial = 0;
};

class PacketQueue {
public:
    PacketQueue() = default;
    explicit PacketQueue(std::size_t maxSize) : que_(maxSize) {}

    void push(AVPacket* pkt);
    Packet pop();
    void clear();
    void close();
    void reopen();

    void setSerial(uint32_t serial) {
        serial_ = serial;
    }

    [[nodiscard]] uint32_t getSerial() {
        std::lock_guard lk(mutex_);
        return serial_;
    }

    [[nodiscard]] uint64_t getDuration() {
        std::lock_guard lk(mutex_);
        return duration_;
    }

    [[nodiscard]]  uint32_t getPacketSize() {
        std::lock_guard lk(mutex_);
        return packetSize_;
    }

private:
    ConcurrentQueue<Packet> que_{};
    uint32_t serial_ = 0;
    uint64_t duration_ = 0;
    uint32_t packetSize_ = 0;

    std::mutex mutex_{};
};