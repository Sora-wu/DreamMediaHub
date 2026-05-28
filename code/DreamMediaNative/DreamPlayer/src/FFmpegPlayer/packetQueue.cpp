//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#include <FFmpegPlayer/packetQueue.h>
#include <FFmpegPlayer/ffinstance.h>

void PacketQueue::push(AVPacket* pkt) {
    Packet packet{};
    av_packet_ref(&packet.packet, pkt);
    if (pkt == FFINSTANCE()->getFlushPkt()) {
        ++serial_;
    }
    packet.serial = serial_;

    {
        std::lock_guard lock(mutex_);
        duration_ += pkt->duration;
        packetSize_ += pkt->size + sizeof(Packet);
    }

    que_.push(packet);
}

std::optional<Packet> PacketQueue::pop() {
    std::optional<Packet> packetOpt = que_.pop();
    if (!packetOpt) {
        return std::nullopt;
    }

    Packet packet = *packetOpt;
    {
        std::lock_guard lock(mutex_);
        duration_ -= packet.packet.duration;
        packetSize_ -= (packet.packet.size + sizeof(Packet));
    }

    return packet;
}

void PacketQueue::clear() {
    while (std::optional<Packet> packetOpt = que_.try_pop()) {
        av_packet_unref(&packetOpt->packet);
    }

    {
        std::lock_guard lock(mutex_);
        duration_ = 0;
        packetSize_ = 0;
    }
}

void PacketQueue::close() {
    push(FFINSTANCE()->getFlushPkt());
    que_.close();
}

void PacketQueue::reopen() {
    clear();
    que_.reopen();
}
