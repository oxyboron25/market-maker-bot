#pragma once
#include "matching_engine.hpp"
#include <cstdint>

class RiskManager {
public:
    RiskManager() = default;

    bool canQuote(int64_t inventory) const;
    bool checkDrawdown(double equity, double startCapital) const;
    void setMaxInventory(int64_t m) { maxInventory = m; }
    void setMaxDrawdownPct(double pct) { maxDrawdownPct = pct; }
    bool isKilled() const { return killed; }
    void kill() { killed = true; }
    void reset() { killed = false; }

    void cancelAll(MatchingEngine& engine, uint64_t bidId, uint64_t askId);

private:
    int64_t maxInventory = 500;
    double maxDrawdownPct = 0.05;
    bool killed = false;
};
