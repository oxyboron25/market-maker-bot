#pragma once
#include "order_book.hpp"
#include <cstdint>
#include <vector>

enum class Side : uint8_t;

class PnLTracker {
public:
    PnLTracker() = default;

    void onFill(const Trade& trade, bool botBuy, double reservationPrice);
    void onTick(double markPrice, uint64_t timestamp);

    double getRealizedPnL() const { return realizedPnL; }
    double getUnrealizedPnL(double currentMid, int64_t inventory) const;
    double getEquity(double currentMid, int64_t inventory) const;

    const std::vector<double>& getEquityCurve() const { return equityCurve; }
    const std::vector<double>& getInventoryCurve() const { return inventoryCurve; }
    const std::vector<uint64_t>& getTimestamps() const { return timestamps; }

    double getSharpe();
    double getMaxDrawdown();
    double getFillRate() const;
    double getAvgAdverseSelection(int64_t inventory) const;

    void recordQuote() { totalQuotes++; }
    int64_t getTotalFills() const { return totalFills; }
    int64_t getTotalQuotes() const { return totalQuotes; }

    void pushEquity(double e) { equityCurve.push_back(e); }
    void pushInventory(int64_t inv) { inventoryCurve.push_back(inv); }

    void setAdverseWindow(int w) { adverseWindow = w; }
    void setAdverseLookback(int64_t tick) { adverseLookback = tick; }

    double startCapital = 100000.0;

private:
    double realizedPnL = 0.0;
    int64_t totalFills = 0;
    int64_t totalQuotes = 0;

    std::vector<double> equityCurve;
    std::vector<double> inventoryCurve;
    std::vector<uint64_t> timestamps;
    std::vector<double> fillPrices;
    std::vector<double> postFillMids;
    std::vector<int64_t> fillInventory;
    int adverseWindow = 20;
    int64_t adverseLookback = 0;
};
