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

Packet PacketQueue::pop() {
    Packet packet = que_.pop();

    {
        std::lock_guard lock(mutex_);
        duration_ -= packet.packet.duration;
        packetSize_ -= (packet.packet.size + sizeof(Packet));
    }

    return packet;
}

void PacketQueue::clear() {
    while (!que_.empty()) {
        Packet packet = que_.pop();
        av_packet_unref(&packet.packet);
    }

    {
        std::lock_guard lock(mutex_);
        duration_ = 0;
        packetSize_ = 0;
    }
}
