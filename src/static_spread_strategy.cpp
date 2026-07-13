#include "static_spread_strategy.hpp"
#include <algorithm>

using namespace std;

void StaticSpreadStrategy::onTick(MatchingEngine& engine, FairValueEstimator& fve, PnLTracker& pnl, uint64_t timestamp) {
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

    double bidPx = mid - fixedSpread / 2.0;
    double askPx = mid + fixedSpread / 2.0;
    if (bidPx < 1.0) bidPx = 1.0;

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
            pnl.onFill(t, true, mid);
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
            pnl.onFill(t, false, mid);
        }
        if (res.trades.empty())
            activeAskId = ask.id;
        pnl.recordQuote();
    }

    pnl.onTick(mid, timestamp);
}

void StaticSpreadStrategy::onFill(const Trade& trade) {
    bool botIsBuyer = (trade.buyOrderId >= 2000000 && trade.buyOrderId < 3000000);
    if (botIsBuyer)
        inventory += trade.quantity;
    else
        inventory -= trade.quantity;
}
