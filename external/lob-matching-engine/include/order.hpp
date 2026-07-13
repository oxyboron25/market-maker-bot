#pragma once
#include <cstdint>

enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1,
    IOC = 2
};

struct Order {
    uint64_t id;
    Side side;
    OrderType type;
    int64_t price;
    uint64_t quantity;
    uint64_t remainingQuantity;
    uint64_t timestamp;

    Order* prev = nullptr;
    Order* next = nullptr;
};

struct Trade {
    uint64_t buyOrderId;
    uint64_t sellOrderId;
    int64_t price;
    uint64_t quantity;
    uint64_t timestamp;
};

struct OrderLocation {
    int64_t price;
    Side side;
    Order* orderPtr = nullptr;
};
