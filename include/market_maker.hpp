#pragma once
#include "matching_engine.hpp"
#include "fair_value_estimator.hpp"
#include "pnl_tracker.hpp"
#include <cstdint>
#include <optional>

class MarketMaker {
public:
    MarketMaker() = default;

    void onTick(MatchingEngine& engine, FairValueEstimator& fve, PnLTracker& pnl, uint64_t timestamp);
    void onFill(const Trade& trade);

    double getReservationPrice() const { return reservationPrice; }
    double getSpread() const { return spread; }
    int64_t getInventory() const { return inventory; }

    void setGamma(double g) { gamma = g; }
    void setK(double kv) { k = kv; }
    void setQuoteSize(uint64_t s) { quoteSize = s; }
    void setMaxInventory(int64_t m) { maxInventory = m; }
    void setSpreadBounds(double lo, double hi) { minSpread = lo; maxSpread = hi; }

private:
    double gamma = 0.1;
    double k = 1.5;
    double minSpread = 1.0;
    double maxSpread = 50.0;
    uint64_t quoteSize = 10;
    int64_t inventory = 0;
    int64_t maxInventory = 500;
    std::optional<uint64_t> activeBidId;
    std::optional<uint64_t> activeAskId;
    double reservationPrice = 0.0;
    double spread = 0.0;
    int64_t nextOrderId = 1000000;
};
