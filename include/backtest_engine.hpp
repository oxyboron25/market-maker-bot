#pragma once
#include "matching_engine.hpp"
#include "noise_trader.hpp"
#include "fair_value_estimator.hpp"
#include "market_maker.hpp"
#include "static_spread_strategy.hpp"
#include "risk_manager.hpp"
#include "pnl_tracker.hpp"
#include <cstdint>
#include <string>

struct BacktestConfig {
    uint64_t numTicks = 5000;
    uint64_t seed = 42;
    double startCapital = 100000.0;
    bool useMarketMaker = true;
    double gamma = 0.1;
    double k = 1.5;
    double fixedSpread = 10.0;
    uint64_t quoteSize = 10;
    int64_t maxInventory = 500;
    double maxDrawdownPct = 0.15;
};

struct BacktestResult {
    double realizedPnL;
    double sharpe;
    double maxDrawdown;
    double fillRate;
    double avgAdverseSelection;
    int64_t totalFills;
    int64_t totalQuotes;
    std::string equityCsv;
    std::string inventoryCsv;
};

class BacktestEngine {
public:
    BacktestResult run(const BacktestConfig& cfg, const std::string& tag = "run");
    void runSweep(double gammaMin, double gammaMax, int gammaSteps,
                  double kMin, double kMax, int kSteps,
                  uint64_t numTicks, uint64_t seed, const std::string& outDir);
};
