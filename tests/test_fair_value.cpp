#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "matching_engine.hpp"
#include "fair_value_estimator.hpp"

using Catch::Approx;

TEST_CASE("mid price from known book", "[fair_value]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 99, 100);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 101, 100);

    FairValueEstimator fve;
    double mid = fve.getMidPrice(e.getOrderBook());
    REQUIRE(mid == 100.0);
}

TEST_CASE("mid price one-sided book", "[fair_value]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 50, 10);

    FairValueEstimator fve;
    double mid = fve.getMidPrice(e.getOrderBook());
    REQUIRE(mid == 50.0);
}

TEST_CASE("volatility increases with volatile series", "[fair_value]") {
    FairValueEstimator calm(50);
    FairValueEstimator volatile_fve(50);

    for (int i = 0; i < 60; i++) {
        calm.onTick(100.0 + 0.01 * sin(i), i);
        volatile_fve.onTick(100.0 + 5.0 * sin(i), i);
    }

    REQUIRE(volatile_fve.getVolatility() > calm.getVolatility());
}

TEST_CASE("volatility is near default with no data", "[fair_value]") {
    FairValueEstimator fve;
    REQUIRE(fve.getVolatility() == Approx(0.01));
}
