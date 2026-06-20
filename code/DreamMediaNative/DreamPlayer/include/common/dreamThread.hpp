//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <thread>

class DreamThread {
public:
    virtual ~DreamThread() = default;

    void start(const std::string& threadName = {}) {
        thread_ = std::jthread([this](std::stop_token st) {
            run(st);
        });

        if (!threadName.empty()) {
            auto threadHandle = thread_.native_handle();
#ifdef PLATFORM_LINUX
            pthread_setname_np(threadHandle, threadName.c_str());
#elif PLATFORM_WINDOWS
            SetThreadDescription(threadHandle, threadName.c_str());
#endif
        }
    }

    void exit() {
        thread_.request_stop();
    }

    virtual void run(std::stop_token st) = 0;

    static void msleep(uint32_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    static void sleep(uint32_t s) {
        std::this_thread::sleep_for(std::chrono::seconds(s));
    }

private:
    std::jthread thread_;
};