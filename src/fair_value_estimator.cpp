#include "fair_value_estimator.hpp"

using namespace std;

FairValueEstimator::FairValueEstimator(int ws) : windowSize(ws) {}

double FairValueEstimator::getMidPrice(const OrderBook& book) {
    auto tob = book.getTopOfBook();
    if (tob.bid && tob.ask) {
        lastMid_ = ((double)tob.bid->price + tob.ask->price) / 2.0;
    } else if (tob.bid) {
        lastMid_ = tob.bid->price;
    } else if (tob.ask) {
        lastMid_ = tob.ask->price;
    }
    return lastMid_;
}

double FairValueEstimator::getVolatility() {
    if (logReturns.size() < 2) return 0.01;

    double sum = 0.0;
    for (double r : logReturns) sum += r;
    double mean = sum / logReturns.size();

    double sq = 0.0;
    for (double r : logReturns) sq += (r - mean) * (r - mean);
    double vol = sqrt(sq / logReturns.size());
    return max(vol, 0.001);
}

void FairValueEstimator::onTick(double midPrice, uint64_t) {
    if (hasLast && lastPrice > 0) {
        double ret = log(midPrice / lastPrice);
        logReturns.push_back(ret);
        if ((int)logReturns.size() > windowSize)
            logReturns.pop_front();
    }
    lastPrice = midPrice;
    hasLast = true;
    priceHistory.push_back(midPrice);
    if ((int)priceHistory.size() > windowSize)
        priceHistory.pop_front();
}
