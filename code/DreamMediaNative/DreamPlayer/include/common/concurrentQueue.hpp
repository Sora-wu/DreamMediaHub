//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

template <typename T, typename Deque = std::deque<T>>
class ConcurrentQueue {
public:
    ConcurrentQueue() : maxSize_(std::numeric_limits<std::size_t>::max()) {}
    explicit  ConcurrentQueue(std::size_t maxSize) : maxSize_(maxSize) {}

    [[nodiscard]] bool empty() {
        std::unique_lock lock(mutex_);
        return queue_.empty();
    }

    [[nodiscard]] std::size_t size() {
        std::unique_lock lock(mutex_);
        return queue_.size();
    }

    // 推入数据，如果满，则阻塞
    void push(T value) {
        std::unique_lock lock(mutex_);
        while (queue_.size() >= maxSize_) {
            cvFull_.wait(lock);
        }

        queue_.push_back(std::move(value));
        cvEmpty_.notify_one();
    }

    // 尝试推数据，不阻塞，如果满，返回 false
    bool try_push(T value) {
        std::unique_lock lock(mutex_);
        if  (queue_.size() >= maxSize_) {
            return false;
        }

        queue_.push_back(std::move(value));
        cvEmpty_.notify_one();
        return true;
    }

    // 尝试推数据，如果满，等待一段时间，超时返回 false
    bool try_push_for(T value, std::chrono::steady_clock::duration timeout) {
        std::unique_lock lock(mutex_);
        if (!cvFull_.wait_for(lock, timeout, [&] { return queue_.size() < maxSize_; })) {
            return false;
        }

        queue_.push_back(std::move(value));
        cvEmpty_.notify_one();
        return true;
    }

    // 尝试推数据，如果满，等待直到时间点，超时返回 false
    bool try_push_until(T value, std::chrono::steady_clock::time_point timeout) {
        std::unique_lock lock(mutex_);
        if (!cvFull_.wait_until(lock, timeout, [&] { return queue_.size() < maxSize_; })) {
            return false;
        }

        queue_.push_back(std::move(value));
        cvEmpty_.notify_one();
        return true;
    }

    // 陷入阻塞，直到有数据
    T pop() {
        std::unique_lock lock(mutex_);
        while (queue_.empty()) {
            cvEmpty_.wait(lock);
        }

        T value = std::move(queue_.front());
        queue_.pop_front();
        cvFull_.notify_one();
        return value;
    }

    // 尝试取数据，不阻塞，如果没有，返回 nullopt
    std::optional<T> try_pop() {
        std::unique_lock lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T value = std::move(queue_.front());
        queue_.pop_front();
        cvFull_.notify_one();
        return value;
    }

    // 尝试取数据，如果没有，等待一段时间，超时返回 nullopt
    std::optional<T> try_pop_for(std::chrono::steady_clock::duration timeout) {
        std::unique_lock lock(mutex_);
        if (!cvEmpty_.wait_for(lock, timeout, [&] { return !queue_.empty(); })) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop_front();
        cvFull_.notify_one();
        return value;
    }

    // 尝试取数据，如果没有，等待直到时间点，超时返回 nullopt
    std::optional<T> try_pop_until(std::chrono::steady_clock::time_point timeout) {
        std::unique_lock lock(mutex_);
        if (!cvEmpty_.wait_until(lock, timeout, [&] { return !queue_.empty(); })) {
            return std::nullopt;
        }
        
        T value = std::move(queue_.front());
        queue_.pop_front();
        cvFull_.notify_one();
        return value;
    }

private:
    Deque queue_;
    std::mutex mutex_;
    std::condition_variable cvEmpty_;
    std::condition_variable cvFull_;
    std::size_t maxSize_{};
};