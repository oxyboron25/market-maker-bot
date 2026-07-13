#include "backtest_engine.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace std;

BacktestResult BacktestEngine::run(const BacktestConfig& cfg, const string& tag) {
    MatchingEngine engine;
    NoiseTrader nt(cfg.seed);
    FairValueEstimator fve(200);
    RiskManager rm;
    rm.setMaxInventory(cfg.maxInventory);
    rm.setMaxDrawdownPct(cfg.maxDrawdownPct);

    PnLTracker pnl;
    pnl.startCapital = cfg.startCapital;
    pnl.setAdverseWindow(20);

    bool useMM = cfg.useMarketMaker;

    MarketMaker mm;
    mm.setGamma(cfg.gamma);
    mm.setK(cfg.k);
    mm.setQuoteSize(cfg.quoteSize);
    mm.setMaxInventory(cfg.maxInventory);

    StaticSpreadStrategy ss;
    ss.setSpread(cfg.fixedSpread);
    ss.setQuoteSize(cfg.quoteSize);
    ss.setMaxInventory(cfg.maxInventory);

    auto processTrade = [&](const Trade& t) {
        if (useMM) {
            bool botIsBuyer = (t.buyOrderId >= 1000000 && t.buyOrderId < 2000000);
            bool botInvolved = botIsBuyer || (t.sellOrderId >= 1000000 && t.sellOrderId < 2000000);
            if (botInvolved) {
                mm.onFill(t);
                pnl.onFill(t, botIsBuyer, mm.getReservationPrice());
            }
        } else {
            bool botIsBuyer = (t.buyOrderId >= 2000000 && t.buyOrderId < 3000000);
            bool botInvolved = botIsBuyer || (t.sellOrderId >= 2000000 && t.sellOrderId < 3000000);
            if (botInvolved) {
                ss.onFill(t);
                pnl.onFill(t, botIsBuyer, 0.0);
            }
        }
    };
    engine.onTrade = processTrade;

    uint64_t t = 0;
    for (; t < cfg.numTicks; t++) {
        nt.onTick(engine, t);

        auto tob = engine.getOrderBook().getTopOfBook();
        if (!tob.bid || !tob.ask) continue;

        double mid = ((double)tob.bid->price + tob.ask->price) / 2.0;

        if (!rm.checkDrawdown(pnl.getEquity(mid, useMM ? mm.getInventory() : ss.getInventory()), cfg.startCapital)) {
            rm.kill();
            cout << "[RISK] kill-switch at tick " << t << "\n";
            break;
        }

        if (useMM)
            mm.onTick(engine, fve, pnl, t);
        else
            ss.onTick(engine, fve, pnl, t);

        int64_t inv = useMM ? mm.getInventory() : ss.getInventory();
        double eq = pnl.getEquity(mid, inv);
        pnl.pushEquity(eq);
        pnl.pushInventory(inv);
    }

    BacktestResult res;
    res.realizedPnL = pnl.getRealizedPnL();
    res.sharpe = pnl.getSharpe();
    res.maxDrawdown = pnl.getMaxDrawdown();
    res.fillRate = pnl.getFillRate();
    res.avgAdverseSelection = pnl.getAvgAdverseSelection(useMM ? mm.getInventory() : ss.getInventory());
    res.totalFills = pnl.getTotalFills();
    res.totalQuotes = pnl.getTotalQuotes();

    string eqFile = "results/" + tag + "_equity.csv";
    string invFile = "results/" + tag + "_inventory.csv";
    ofstream eqCsv(eqFile);
    ofstream invCsv(invFile);
    eqCsv << "timestamp,equity,inventory\n";
    auto& eqVec = pnl.getEquityCurve();
    auto& invVec = pnl.getInventoryCurve();
    for (size_t i = 0; i < eqVec.size(); i++) {
        eqCsv << i << "," << fixed << setprecision(2) << eqVec[i]
              << "," << invVec[i] << "\n";
    }
    res.equityCsv = eqFile;
    res.inventoryCsv = invFile;
    return res;
}

void BacktestEngine::runSweep(double gammaMin, double gammaMax, int gammaSteps,
                               double kMin, double kMax, int kSteps,
                               uint64_t numTicks, uint64_t seed, const string& outDir) {
    ofstream csv(outDir + "/sweep_results.csv");
    csv << "gamma,k,sharpe,max_dd,fill_rate,pnl\n";

    double gStep = (gammaMax - gammaMin) / max(gammaSteps - 1, 1);
    double kStep = (kMax - kMin) / max(kSteps - 1, 1);

    for (int gi = 0; gi < gammaSteps; gi++) {
        for (int ki = 0; ki < kSteps; ki++) {
            double g = gammaMin + gi * gStep;
            double kv = kMin + ki * kStep;

            BacktestConfig cfg;
            cfg.numTicks = numTicks;
            cfg.seed = seed;
            cfg.gamma = g;
            cfg.k = kv;
            cfg.useMarketMaker = true;

            BacktestResult r = run(cfg, "sweep_g" + to_string(gi) + "_k" + to_string(ki));
            csv << fixed << setprecision(4) << g << "," << kv << ","
                << r.sharpe << "," << r.maxDrawdown << "," << r.fillRate << ","
                << r.realizedPnL << "\n";
        }
    }
    cout << "[SWEEP] wrote results to " << outDir << "/sweep_results.csv\n";
}
