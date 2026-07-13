#pragma once
#include "order.hpp"
#include "price_level.hpp"
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>

enum class OrderErrorCode {
    NONE = 0,
    INVALID_QUANTITY,
    INVALID_PRICE,
    ORDER_NOT_FOUND
};

struct AddOrderResult {
    std::vector<Trade> trades;
    OrderErrorCode error = OrderErrorCode::NONE;
};

struct CancelOrderResult {
    bool success = false;
    OrderErrorCode error = OrderErrorCode::NONE;
};

struct ModifyOrderResult {
    std::vector<Trade> trades;
    OrderErrorCode error = OrderErrorCode::NONE;
};

struct DepthLevel {
    int64_t price;
    uint64_t quantity;
};

struct TopOfBook {
    std::optional<DepthLevel> bid;
    std::optional<DepthLevel> ask;
};

class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook();
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    AddOrderResult addOrder(Order order);
    CancelOrderResult cancelOrder(uint64_t orderId);
    ModifyOrderResult modifyOrder(uint64_t orderId, int64_t newPrice, uint64_t newQuantity);

    std::optional<DepthLevel> getBestBid() const;
    std::optional<DepthLevel> getBestAsk() const;
    TopOfBook getTopOfBook() const;
    std::vector<DepthLevel> getDepthSnapshot(int levels, Side side) const;

    size_t totalOrders() const;
    bool empty() const;

private:
    std::map<int64_t, IntrusiveList, std::greater<int64_t>> bids_;
    std::map<int64_t, IntrusiveList, std::less<int64_t>> asks_;
    std::unordered_map<uint64_t, OrderLocation> orderIndex_;

    void matchBuy(Order& order, std::vector<Trade>& trades);
    void matchSell(Order& order, std::vector<Trade>& trades);
};
