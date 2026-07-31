#pragma once

#include <mutex>
#include <optional>
#include <queue>
#include <utility>

template <typename T>
class LoadResultQueue {
public:
    void push(T value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(value));
    }

    std::optional<T> tryPop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return std::nullopt;

        T value = std::move(m_queue.front());
        m_queue.pop();
        return value;
    }

private:
    std::mutex m_mutex;
    std::queue<T> m_queue;
};
