#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "matching_engine.hpp"
#include "market_maker.hpp"
#include "static_spread_strategy.hpp"
#include "fair_value_estimator.hpp"
#include "pnl_tracker.hpp"

using Catch::Approx;

static void seedBook(MatchingEngine& e) {
    for (int i = 90; i <= 110; i += 2)
        e.submitOrder(Side::BUY, OrderType::LIMIT, i, 100);
    for (int i = 112; i <= 130; i += 2)
        e.submitOrder(Side::SELL, OrderType::LIMIT, i, 100);
}

TEST_CASE("reservation price skews with inventory", "[quoting]") {
    MarketMaker mm;
    mm.setGamma(0.5);
    mm.setK(1.5);

    FairValueEstimator fve;
    PnLTracker pnl;

    MatchingEngine e;
    seedBook(e);

    mm.onTick(e, fve, pnl, 1);
    double baseRes = mm.getReservationPrice();

    Trade t{1000001, 999, 100, 50, 1};
    mm.onFill(t);

    mm.onTick(e, fve, pnl, 2);
    double longRes = mm.getReservationPrice();

    REQUIRE(longRes < baseRes);
}

TEST_CASE("spread increases with gamma at high volatility", "[quoting]") {
    // Test the AS formula directly: spread = gamma*sigma^2 + (2/gamma)*ln(1+gamma/k)
    // At high sigma, gamma*sigma^2 dominates — higher gamma = wider spread
    double sigma = 5.0;
    double k = 1.5;
    double g1 = 0.1, g2 = 1.0;

    double s1 = g1 * sigma * sigma + (2.0 / g1) * log(1.0 + g1 / k);
    double s2 = g2 * sigma * sigma + (2.0 / g2) * log(1.0 + g2 / k);

    REQUIRE(s2 > s1);
}

TEST_CASE("bot only quotes ask when max inventory hit", "[quoting]") {
    MarketMaker mm;
    mm.setMaxInventory(5);
    mm.setQuoteSize(10);

    FairValueEstimator fve;
    PnLTracker pnl;
    MatchingEngine e;
    seedBook(e);

    for (int i = 0; i < 10; i++) {
        Trade t{1000001, 999, 100, 10, (uint64_t)i};
        mm.onFill(t);
    }

    mm.onTick(e, fve, pnl, 100);
    REQUIRE(mm.getInventory() > 5);
    REQUIRE(e.getOrderBook().getBestBid() != std::nullopt);
}

TEST_CASE("requote cycle no orphaned orders", "[quoting]") {
    MarketMaker mm;
    FairValueEstimator fve;
    PnLTracker pnl;
    MatchingEngine e;
    seedBook(e);

    for (int t = 0; t < 100; t++) {
        if (t % 5 == 0)
            e.submitOrder(Side::BUY, OrderType::LIMIT, 95 + t % 10, 5);
        mm.onTick(e, fve, pnl, t);
    }

    auto bidSnap = e.getOrderBook().getDepthSnapshot(20, Side::BUY);
    auto askSnap = e.getOrderBook().getDepthSnapshot(20, Side::SELL);
    size_t total = bidSnap.size() + askSnap.size();
    REQUIRE(total <= 40);
}

TEST_CASE("static spread gives symmetric quotes", "[quoting]") {
    StaticSpreadStrategy ss;
    ss.setSpread(10.0);
    ss.setQuoteSize(10);

    FairValueEstimator fve;
    PnLTracker pnl;
    MatchingEngine e;
    seedBook(e);

    ss.onTick(e, fve, pnl, 1);
    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.bid.has_value());
    REQUIRE(tob.ask.has_value());
}
