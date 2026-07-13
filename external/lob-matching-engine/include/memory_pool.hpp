#pragma once
#include <cstddef>
#include <new>

template<typename T, size_t PoolSize>
class MemoryPool {
    struct FreeNode {
        FreeNode* next;
    };

public:
    MemoryPool() {
        for (size_t i = 0; i < PoolSize - 1; ++i)
            reinterpret_cast<FreeNode*>(pool_ + i * sizeof(T))->next =
                reinterpret_cast<FreeNode*>(pool_ + (i + 1) * sizeof(T));
        reinterpret_cast<FreeNode*>(pool_ + (PoolSize - 1) * sizeof(T))->next = nullptr;
        freeList_ = reinterpret_cast<FreeNode*>(pool_);
    }

    template<typename... Args>
    T* allocate(Args&&... args) {
        if (!freeList_) return nullptr;
        auto node = freeList_;
        freeList_ = freeList_->next;
        return new (node) T(std::forward<Args>(args)...);
    }

    void deallocate(T* ptr) {
        if (!ptr) return;
        ptr->~T();
        auto node = reinterpret_cast<FreeNode*>(ptr);
        node->next = freeList_;
        freeList_ = node;
    }

    size_t available() const {
        size_t cnt = 0;
        for (auto cur = freeList_; cur; cur = cur->next) ++cnt;
        return cnt;
    }

private:
    alignas(T) char pool_[sizeof(T) * PoolSize];
    FreeNode* freeList_ = nullptr;
};
