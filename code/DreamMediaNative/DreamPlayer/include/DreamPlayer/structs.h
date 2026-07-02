//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamPlayer/define.h>

#include <functional>
#include <cstdint>

namespace Dream {
    using OnSeekCallback = std::function<void()>;
    using OnPlaybackCompletedCallback = std::function<void()>;

    enum class SyncType {
        AudioMaster,
        VideoMaster,
    };

    struct DREAM_PLAYER_API VideoFrame {
        uint32_t frameWidth{};
        uint32_t frameHeight{};
        double picWidthHeightRatio{};

        char* data[3]{};
        int len[3]{};
        int stride[3]{};
    };

    struct DREAM_PLAYER_API AudioFrame {
        char* data{};
        int len{};
        uint32_t capacity = 0;
        double pts{};
        uint32_t serial = 0;
        bool isUpdated = false;
    };
}