//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <thread>

class DreamThraed {
public:
    virtual ~DreamThraed() = default;

    void start() {
        isExit_ = false;
        thread_ = std::jthread(&DreamThraed::run, this);
    }

    void exit() {
        isExit_ = true;
    }

    virtual void run() = 0;

    static void msleep(uint32_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    static void sleep(uint32_t s) {
        std::this_thread::sleep_for(std::chrono::seconds(s));
    }

protected:
    bool isExit_ = false;

private:
    std::jthread thread_;
};