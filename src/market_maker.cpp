#include "market_maker.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

void MarketMaker::onTick(MatchingEngine& engine, FairValueEstimator& fve, PnLTracker& pnl, uint64_t timestamp) {
    if (activeBidId)
        engine.cancelOrder(*activeBidId);
    if (activeAskId)
        engine.cancelOrder(*activeAskId);
    activeBidId = nullopt;
    activeAskId = nullopt;

    auto tob = engine.getOrderBook().getTopOfBook();
    if (!tob.bid || !tob.ask) return;

    double mid = ((double)tob.bid->price + tob.ask->price) / 2.0;
    fve.onTick(mid, timestamp);
    double sigma = fve.getVolatility();

    reservationPrice = mid - inventory * gamma * sigma * sigma;
    spread = gamma * sigma * sigma + (2.0 / gamma) * log(1.0 + gamma / k);
    spread = clamp(spread, minSpread, maxSpread);

    double bidPx = reservationPrice - spread / 2.0;
    double askPx = reservationPrice + spread / 2.0;
    if (bidPx < 1.0) bidPx = 1.0;
    if (askPx < 2.0) askPx = 2.0;

    if (inventory < maxInventory) {
        Order bid{};
        bid.id = nextOrderId++;
        bid.side = Side::BUY;
        bid.type = OrderType::LIMIT;
        bid.price = (int64_t)bidPx;
        bid.quantity = quoteSize;
        auto res = engine.getOrderBook().addOrder(bid);
        for (auto& t : res.trades) {
            onFill(t);
            pnl.onFill(t, true, reservationPrice);
        }
        if (res.trades.empty())
            activeBidId = bid.id;
        pnl.recordQuote();
    }

    if (inventory > -maxInventory) {
        Order ask{};
        ask.id = nextOrderId++;
        ask.side = Side::SELL;
        ask.type = OrderType::LIMIT;
        ask.price = (int64_t)askPx;
        ask.quantity = quoteSize;
        auto res = engine.getOrderBook().addOrder(ask);
        for (auto& t : res.trades) {
            onFill(t);
            pnl.onFill(t, false, reservationPrice);
        }
        if (res.trades.empty())
            activeAskId = ask.id;
        pnl.recordQuote();
    }

    pnl.onTick(mid, timestamp);
}

void MarketMaker::onFill(const Trade& trade) {
    bool botIsBuyer = (trade.buyOrderId >= 1000000 && trade.buyOrderId < 2000000);
    if (botIsBuyer)
        inventory += trade.quantity;
    else
        inventory -= trade.quantity;
}
