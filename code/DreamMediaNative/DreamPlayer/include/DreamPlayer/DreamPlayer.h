//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <cstdint>
#include <functional>

namespace Dream {
    struct VideoFrame {
        uint32_t frameWidth{};
        uint32_t frameHeight{};

        char* dataY{};
        uint32_t lenY{};

        char* dataU{};
        uint32_t lenU{};

        char* dataV{};
        uint32_t lenV{};
    };

    // std::function<>

    class DreamPlayer {
    public:
        virtual ~DreamPlayer() = default;
        virtual bool open(const char* url) = 0;
        virtual void play() = 0;
        virtual void pause() = 0;
        virtual void stop() = 0;

        virtual void seek(double position) = 0;
        [[nodiscard]] virtual uint64_t getDuration() const = 0;

        virtual void setVolume(uint32_t volume) = 0;
        [[nodiscard]] virtual uint32_t getVolume() const = 0;

        virtual void setSpeed(double speed) = 0;
    };
}