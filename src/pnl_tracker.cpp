#include "pnl_tracker.hpp"
#include <cmath>
#include <algorithm>

using namespace std;

void PnLTracker::onFill(const Trade& trade, bool botBuy, double reservationPrice) {
    double fillPx = trade.price;
    fillPrices.push_back(fillPx);
    postFillMids.push_back(0);
    fillInventory.push_back(0);
    totalFills++;

    double side = botBuy ? 1.0 : -1.0;
    realizedPnL += side * (fillPx - reservationPrice) * trade.quantity;
}

void PnLTracker::onTick(double markPrice, uint64_t timestamp) {
    timestamps.push_back(timestamp);
}

double PnLTracker::getUnrealizedPnL(double currentMid, int64_t inventory) const {
    if (fillPrices.empty()) return 0.0;
    double unrealized = 0.0;
    for (size_t i = 0; i < fillPrices.size(); i++) {
        if (postFillMids[i] == 0) continue;
        double move = currentMid - fillPrices[i];
        unrealized += move * fillInventory[i];
    }
    return unrealized;
}

double PnLTracker::getEquity(double currentMid, int64_t inventory) const {
    return startCapital + realizedPnL + (double)inventory * currentMid;
}

double PnLTracker::getSharpe() {
    if (equityCurve.size() < 10) return 0.0;

    vector<double> returns;
    for (size_t i = 1; i < equityCurve.size(); i++) {
        if (equityCurve[i - 1] != 0)
            returns.push_back((equityCurve[i] - equityCurve[i - 1]) / equityCurve[i - 1]);
    }
    if (returns.size() < 2) return 0.0;

    double sum = 0.0;
    for (double r : returns) sum += r;
    double mean = sum / returns.size();

    double sq = 0.0;
    for (double r : returns) sq += (r - mean) * (r - mean);
    double sd = sqrt(sq / returns.size());
    if (sd < 1e-12) return 0.0;

    return (mean / sd) * sqrt(252.0);
}

double PnLTracker::getMaxDrawdown() {
    if (equityCurve.empty()) return 0.0;
    double peak = equityCurve[0], maxDd = 0.0;
    for (double e : equityCurve) {
        peak = max(peak, e);
        double dd = (peak - e) / peak;
        maxDd = max(maxDd, dd);
    }
    return maxDd;
}

double PnLTracker::getFillRate() const {
    if (totalQuotes == 0) return 0.0;
    return (double)totalFills / totalQuotes;
}

double PnLTracker::getAvgAdverseSelection(int64_t inventory) const {
    if (fillPrices.empty()) return 0.0;
    double total = 0.0;
    int cnt = 0;
    for (size_t i = 0; i < fillPrices.size(); i++) {
        if (postFillMids[i] == 0) continue;
        double move = postFillMids[i] - fillPrices[i];
        total += fabs(move);
        cnt++;
    }
    return cnt > 0 ? total / cnt : 0.0;
}
