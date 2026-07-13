#pragma once
#include "matching_engine.hpp"
#include "fair_value_estimator.hpp"
#include "pnl_tracker.hpp"
#include <cstdint>
#include <optional>

class StaticSpreadStrategy {
public:
    StaticSpreadStrategy() = default;

    void onTick(MatchingEngine& engine, FairValueEstimator& fve, PnLTracker& pnl, uint64_t timestamp);
    void onFill(const Trade& trade);

    int64_t getInventory() const { return inventory; }

    void setSpread(double s) { fixedSpread = s; }
    void setQuoteSize(uint64_t s) { quoteSize = s; }
    void setMaxInventory(int64_t m) { maxInventory = m; }

private:
    double fixedSpread = 10.0;
    uint64_t quoteSize = 10;
    int64_t inventory = 0;
    int64_t maxInventory = 500;
    std::optional<uint64_t> activeBidId;
    std::optional<uint64_t> activeAskId;
    int64_t nextOrderId = 2000000;
};
