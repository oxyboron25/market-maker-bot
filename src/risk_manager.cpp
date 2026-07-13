#include "risk_manager.hpp"
#include <cmath>

using namespace std;

bool RiskManager::canQuote(int64_t inventory) const {
    if (killed) return false;
    return abs(inventory) < maxInventory;
}

bool RiskManager::checkDrawdown(double equity, double startCap) const {
    if (killed) return false;
    double dd = (startCap - equity) / startCap;
    return dd < maxDrawdownPct;
}

void RiskManager::cancelAll(MatchingEngine& engine, uint64_t bidId, uint64_t askId) {
    engine.cancelOrder(bidId);
    engine.cancelOrder(askId);
}
