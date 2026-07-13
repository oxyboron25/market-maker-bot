#pragma once
#include <atomic>
#include <array>
#include <cstddef>
#include <optional>

template<typename T, size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "must be power of 2");
    static_assert(Capacity >= 2);

public:
    bool push(const T& item) {
        size_t h = head_.load(relaxed);
        size_t nxt = (h + 1) & (Capacity - 1);
        if (nxt == tail_.load(acquire)) return false;
        buf_[h] = item;
        head_.store(nxt, release);
        return true;
    }

    bool push(T&& item) {
        size_t h = head_.load(relaxed);
        size_t nxt = (h + 1) & (Capacity - 1);
        if (nxt == tail_.load(acquire)) return false;
        buf_[h] = std::move(item);
        head_.store(nxt, release);
        return true;
    }

    std::optional<T> pop() {
        size_t t = tail_.load(relaxed);
        if (t == head_.load(acquire)) return std::nullopt;
        T item = std::move(buf_[t]);
        tail_.store((t + 1) & (Capacity - 1), release);
        return item;
    }

    bool empty() const {
        return head_.load(acquire) == tail_.load(acquire);
    }

    size_t size() const {
        size_t h = head_.load(acquire);
        size_t t = tail_.load(acquire);
        return (h - t + Capacity) & (Capacity - 1);
    }

private:
    using atomic_size = std::atomic<size_t>;
    static constexpr auto relaxed = std::memory_order_relaxed;
    static constexpr auto acquire = std::memory_order_acquire;
    static constexpr auto release = std::memory_order_release;

    alignas(64) atomic_size head_{0};
    alignas(64) atomic_size tail_{0};
    std::array<T, Capacity> buf_;
};
