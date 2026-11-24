#pragma once

#include <atomic>
#include <mutex>
#include <utility>

namespace Dream {
    class DreamPlayer;
}

class VideoRenderAccess {
public:
    explicit VideoRenderAccess(Dream::DreamPlayer* player) : player_(player) {}

    template<class Draw>
    bool withPlayer(Draw&& draw) {
        std::lock_guard lock(mutex_);
        // stop() 可能在前一次检查与拿到锁之间发生
        if (closed_.load()) {
            return false;
        }

        std::forward<Draw>(draw)(*player_);
        return true;
    }

    void closeAndWait() {
        closed_.store(true);
        std::lock_guard lock(mutex_);
        player_ = nullptr;
    }

private:
    std::atomic<bool> closed_{false};
    std::mutex mutex_;
    Dream::DreamPlayer* player_;
};
