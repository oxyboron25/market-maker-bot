#include "noise_trader.hpp"

using namespace std;

NoiseTrader::NoiseTrader(uint64_t seed)
    : rng(seed), arrivals(5), priceWalk(0.0, 0.5) {}

void NoiseTrader::onTick(MatchingEngine& engine, uint64_t timestamp) {
    mid += priceWalk(rng);

    int nOrders = arrivals(rng);
    for (int i = 0; i < nOrders; i++) {
        bool isMarket = uniform_real_distribution<double>(0.0, 1.0)(rng) < cfg.marketOrderRatio;
        bool isBuy = uniform_real_distribution<double>(0.0, 1.0)(rng) < 0.5;
        int64_t offset = max(1L, (int64_t)(exponential_distribution<double>(1.0 / cfg.limitOffsetMean)(rng)));
        int64_t price = (int64_t)mid + (isBuy ? -offset : offset);
        uint64_t qty = cfg.baseQty + (uint64_t)(uniform_int_distribution<int>(0, cfg.qtyVariance)(rng));

        if (isMarket)
            engine.submitOrder(isBuy ? Side::BUY : Side::SELL, OrderType::MARKET, 0, qty);
        else
            engine.submitOrder(isBuy ? Side::BUY : Side::SELL, OrderType::LIMIT, price, qty);
    }
}
