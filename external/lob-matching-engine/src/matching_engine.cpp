#include "matching_engine.hpp"

AddOrderResult MatchingEngine::submitOrder(Side side, OrderType type, int64_t price, uint64_t quantity) {
    Order o{};
    o.id = nextOrderId_++;
    o.side = side;
    o.type = type;
    o.price = price;
    o.quantity = quantity;
    o.timestamp = o.id;
    auto result = book_.addOrder(o);
    if (onTrade) {
        for (auto& t : result.trades)
            onTrade(t);
    }
    return result;
}

CancelOrderResult MatchingEngine::cancelOrder(uint64_t orderId) {
    return book_.cancelOrder(orderId);
}

ModifyOrderResult MatchingEngine::modifyOrder(uint64_t orderId, int64_t newPrice, uint64_t newQuantity) {
    return book_.modifyOrder(orderId, newPrice, newQuantity);
}

const OrderBook& MatchingEngine::getOrderBook() const { return book_; }
OrderBook& MatchingEngine::getOrderBook() { return book_; }
uint64_t MatchingEngine::getNextOrderId() { return nextOrderId_; }
