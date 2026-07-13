#pragma once
#include "order_book.hpp"
#include <functional>

class MatchingEngine {
public:
    AddOrderResult submitOrder(Side side, OrderType type, int64_t price, uint64_t quantity);
    CancelOrderResult cancelOrder(uint64_t orderId);
    ModifyOrderResult modifyOrder(uint64_t orderId, int64_t newPrice, uint64_t newQuantity);

    const OrderBook& getOrderBook() const;
    OrderBook& getOrderBook();
    uint64_t getNextOrderId();

    std::function<void(const Trade&)> onTrade;

private:
    OrderBook book_;
    uint64_t nextOrderId_ = 1;
};
