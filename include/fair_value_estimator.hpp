#pragma once
#include "order_book.hpp"
#include <deque>
#include <cstdint>
#include <cmath>

class FairValueEstimator {
public:
    explicit FairValueEstimator(int windowSize = 200);

    double getMidPrice(const OrderBook& book);
    double getVolatility();
    void onTick(double midPrice, uint64_t timestamp);

private:
    int windowSize;
    std::deque<double> priceHistory;
    std::deque<double> logReturns;
    double lastPrice = 0.0;
    double lastMid_ = 100.0;
    bool hasLast = false;
};
