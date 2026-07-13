#include "backtest_engine.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

int main() {
    cout << "=== Market Maker Bot - Backtest ===\n\n";

    BacktestEngine engine;

    // AS strategy
    BacktestConfig mmCfg;
    mmCfg.numTicks = 10000;
    mmCfg.seed = 42;
    mmCfg.useMarketMaker = true;
    mmCfg.gamma = 0.1;
    mmCfg.k = 1.5;
    mmCfg.quoteSize = 10;
    mmCfg.maxInventory = 500;
    mmCfg.maxDrawdownPct = 0.50;

    auto mmRes = engine.run(mmCfg, "as_strategy");
    cout << fixed << setprecision(4);
    cout << "[AS Strategy]\n";
    cout << "  Realized PnL:       " << mmRes.realizedPnL << "\n";
    cout << "  Sharpe Ratio:       " << mmRes.sharpe << "\n";
    cout << "  Max Drawdown:       " << mmRes.maxDrawdown << "\n";
    cout << "  Fill Rate:          " << mmRes.fillRate << "\n";
    cout << "  Total Fills:        " << mmRes.totalFills << "\n";
    cout << "  Total Quotes:       " << mmRes.totalQuotes << "\n\n";

    // Static spread baseline
    BacktestConfig ssCfg;
    ssCfg.numTicks = 10000;
    ssCfg.seed = 42;
    ssCfg.useMarketMaker = false;
    ssCfg.fixedSpread = 10.0;
    ssCfg.quoteSize = 10;
    ssCfg.maxInventory = 500;
    ssCfg.maxDrawdownPct = 0.50;

    auto ssRes = engine.run(ssCfg, "static_baseline");
    cout << "[Static Spread Baseline]\n";
    cout << "  Realized PnL:       " << ssRes.realizedPnL << "\n";
    cout << "  Sharpe Ratio:       " << ssRes.sharpe << "\n";
    cout << "  Max Drawdown:       " << ssRes.maxDrawdown << "\n";
    cout << "  Fill Rate:          " << ssRes.fillRate << "\n";
    cout << "  Total Fills:        " << ssRes.totalFills << "\n";
    cout << "  Total Quotes:       " << ssRes.totalQuotes << "\n\n";

    cout << "Equity curves saved to results/\n";
    return 0;
}
