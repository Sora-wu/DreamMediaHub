//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <DreamPlayer/define.h>

#include <cstdint>

namespace Dream {
    struct DREAM_PLAYER_API VideoFrame {
        uint32_t frameWidth{};
        uint32_t frameHeight{};
        double picWidthHeightRatio{};

        char* data[3]{};
        int len[3]{};
        int stride[3]{};

        void clear() {
            for (uint32_t i = 0; i < 3; ++i) {
                if (data[i]) {
                    delete[] data[i];
                    data[i] = nullptr;
                    len[i] = 0;
                    stride[i] = 0;
                }
            }
        }
    };

    struct DREAM_PLAYER_API AudioFrame {
        char* data{};
        int len{};
        bool isUpdated = false;

        void clear() {
            if (data) {
                delete[] data;
                data = nullptr;
                len = 0;
            }
        }
    };
}