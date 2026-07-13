#pragma once
#include "matching_engine.hpp"
#include <cstdint>
#include <random>

struct NoiseTraderConfig {
    double midDrift = 0.0;
    double midVol = 0.5;
    double orderIntensity = 5.0;
    double limitOffsetMean = 2.0;
    double marketOrderRatio = 0.15;
    uint64_t baseQty = 5;
    uint64_t qtyVariance = 10;
};

class NoiseTrader {
public:
    explicit NoiseTrader(uint64_t seed = 42);

    void onTick(MatchingEngine& engine, uint64_t timestamp);
    void setConfig(const NoiseTraderConfig& c) { cfg = c; }
    double getMid() const { return mid; }

private:
    NoiseTraderConfig cfg;
    double mid = 100.0;
    std::mt19937 rng;
    std::poisson_distribution<int> arrivals;
    std::normal_distribution<double> priceWalk;
};
