#pragma once
#include "order.hpp"
#include <cstddef>

class IntrusiveList {
public:
    IntrusiveList() = default;
    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    void push_back(Order* order) {
        order->prev = tail_;
        order->next = nullptr;
        if (tail_)
            tail_->next = order;
        else
            head_ = order;
        tail_ = order;
        ++sz;
    }

    Order* front() const { return head_; }

    void pop_front() {
        if (!head_) return;
        auto old = head_;
        head_ = head_->next;
        if (head_)
            head_->prev = nullptr;
        else
            tail_ = nullptr;
        old->prev = old->next = nullptr;
        --sz;
    }

    void remove(Order* o) {
        if (o->prev)
            o->prev->next = o->next;
        else
            head_ = o->next;
        if (o->next)
            o->next->prev = o->prev;
        else
            tail_ = o->prev;
        o->prev = o->next = nullptr;
        --sz;
    }

    bool empty() const { return sz == 0; }
    size_t size() const { return sz; }

    uint64_t totalQuantity() const {
        uint64_t tot = 0;
        for (auto cur = head_; cur; cur = cur->next)
            tot += cur->remainingQuantity;
        return tot;
    }

private:
    Order* head_ = nullptr;
    Order* tail_ = nullptr;
    size_t sz = 0;
};
