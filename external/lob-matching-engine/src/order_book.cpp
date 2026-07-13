#include "order_book.hpp"
#include <algorithm>

using namespace std;

OrderBook::~OrderBook() {
    for (auto& [id, loc] : orderIndex_)
        delete loc.orderPtr;
}

AddOrderResult OrderBook::addOrder(Order order) {
    AddOrderResult result;

    if (order.quantity == 0) {
        result.error = OrderErrorCode::INVALID_QUANTITY;
        return result;
    }
    if (order.type != OrderType::MARKET && order.price <= 0) {
        result.error = OrderErrorCode::INVALID_PRICE;
        return result;
    }

    order.remainingQuantity = order.quantity;

    if (order.side == Side::BUY)
        matchBuy(order, result.trades);
    else
        matchSell(order, result.trades);

    if (order.remainingQuantity > 0 && order.type == OrderType::LIMIT) {
        Order* ptr = new Order(order);
        if (order.side == Side::BUY)
            bids_[order.price].push_back(ptr);
        else
            asks_[order.price].push_back(ptr);
        orderIndex_[order.id] = {order.price, order.side, ptr};
    }

    return result;
}

void OrderBook::matchBuy(Order& order, vector<Trade>& trades) {
    while (order.remainingQuantity > 0 && !asks_.empty()) {
        auto bestIt = asks_.begin();
        int64_t bestAsk = bestIt->first;

        if (order.type != OrderType::MARKET && bestAsk > order.price)
            break;

        auto& level = bestIt->second;
        while (!level.empty() && order.remainingQuantity > 0) {
            Order* resting = level.front();
            uint64_t fillQty = min(order.remainingQuantity, resting->remainingQuantity);

            trades.push_back({order.id, resting->id, bestAsk, fillQty, order.timestamp});

            order.remainingQuantity -= fillQty;
            resting->remainingQuantity -= fillQty;

            if (resting->remainingQuantity == 0) {
                level.pop_front();
                orderIndex_.erase(resting->id);
                delete resting;
            }
        }

        if (level.empty())
            asks_.erase(bestIt);
    }
}

void OrderBook::matchSell(Order& order, vector<Trade>& trades) {
    while (order.remainingQuantity > 0 && !bids_.empty()) {
        auto bestIt = bids_.begin();
        int64_t bestBid = bestIt->first;

        if (order.type != OrderType::MARKET && bestBid < order.price)
            break;

        auto& level = bestIt->second;
        while (!level.empty() && order.remainingQuantity > 0) {
            Order* resting = level.front();
            uint64_t fillQty = min(order.remainingQuantity, resting->remainingQuantity);

            trades.push_back({resting->id, order.id, bestBid, fillQty, order.timestamp});

            order.remainingQuantity -= fillQty;
            resting->remainingQuantity -= fillQty;

            if (resting->remainingQuantity == 0) {
                level.pop_front();
                orderIndex_.erase(resting->id);
                delete resting;
            }
        }

        if (level.empty())
            bids_.erase(bestIt);
    }
}

CancelOrderResult OrderBook::cancelOrder(uint64_t orderId) {
    CancelOrderResult result;
    auto it = orderIndex_.find(orderId);
    if (it == orderIndex_.end()) {
        result.error = OrderErrorCode::ORDER_NOT_FOUND;
        return result;
    }

    auto& loc = it->second;
    auto& level = (loc.side == Side::BUY) ? bids_[loc.price] : asks_[loc.price];
    level.remove(loc.orderPtr);

    if (level.empty()) {
        if (loc.side == Side::BUY)
            bids_.erase(loc.price);
        else
            asks_.erase(loc.price);
    }

    auto ptr = loc.orderPtr;
    orderIndex_.erase(it);
    delete ptr;
    result.success = true;
    return result;
}

ModifyOrderResult OrderBook::modifyOrder(uint64_t orderId, int64_t newPrice, uint64_t newQuantity) {
    ModifyOrderResult result;

    auto idxIt = orderIndex_.find(orderId);
    if (idxIt == orderIndex_.end()) {
        result.error = OrderErrorCode::ORDER_NOT_FOUND;
        return result;
    }

    auto& loc = idxIt->second;
    auto side = loc.side;
    auto type = loc.orderPtr->type;

    if (newQuantity == 0) {
        result.error = OrderErrorCode::INVALID_QUANTITY;
        return result;
    }
    if (type != OrderType::MARKET && newPrice <= 0) {
        result.error = OrderErrorCode::INVALID_PRICE;
        return result;
    }

    cancelOrder(orderId);

    Order o;
    o.id = orderId;
    o.side = side;
    o.type = type;
    o.price = newPrice;
    o.quantity = newQuantity;

    auto addResult = addOrder(o);
    result.trades = move(addResult.trades);
    result.error = addResult.error;
    return result;
}

optional<DepthLevel> OrderBook::getBestBid() const {
    if (bids_.empty()) return nullopt;
    auto it = bids_.begin();
    return DepthLevel{it->first, it->second.totalQuantity()};
}

optional<DepthLevel> OrderBook::getBestAsk() const {
    if (asks_.empty()) return nullopt;
    auto it = asks_.begin();
    return DepthLevel{it->first, it->second.totalQuantity()};
}

TopOfBook OrderBook::getTopOfBook() const {
    return {getBestBid(), getBestAsk()};
}

vector<DepthLevel> OrderBook::getDepthSnapshot(int levels, Side side) const {
    vector<DepthLevel> result;
    if (side == Side::BUY) {
        int cnt = 0;
        for (auto it = bids_.begin(); it != bids_.end() && cnt < levels; ++it, ++cnt)
            result.push_back({it->first, it->second.totalQuantity()});
    } else {
        int cnt = 0;
        for (auto it = asks_.begin(); it != asks_.end() && cnt < levels; ++it, ++cnt)
            result.push_back({it->first, it->second.totalQuantity()});
    }
    return result;
}

size_t OrderBook::totalOrders() const {
    return orderIndex_.size();
}

bool OrderBook::empty() const {
    return orderIndex_.empty();
}
