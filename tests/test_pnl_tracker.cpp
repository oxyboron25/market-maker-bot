#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "pnl_tracker.hpp"

using Catch::Approx;

TEST_CASE("realized PnL from buy then sell", "[pnl]") {
    PnLTracker pnl;
    pnl.startCapital = 100000;

    Trade buy{1, 999, 100, 10, 1};
    pnl.onFill(buy, true, 100.0);

    Trade sell{2, 999, 105, 10, 10};
    pnl.onFill(sell, false, 100.0);

    double eq = pnl.getEquity(105, 0);
    REQUIRE(eq == Approx(100000.0 + 0.0 + (-50.0)));
}

TEST_CASE("max drawdown on synthetic curve", "[pnl]") {
    PnLTracker pnl;
    pnl.pushEquity(100);
    pnl.pushEquity(110);
    pnl.pushEquity(105);
    pnl.pushEquity(95);
    pnl.pushEquity(100);
    double dd = pnl.getMaxDrawdown();
    REQUIRE(dd == Approx(15.0 / 110.0));
}

TEST_CASE("fill rate computation", "[pnl]") {
    PnLTracker pnl;
    for (int i = 0; i < 100; i++)
        pnl.recordQuote();
    for (int i = 0; i < 30; i++) {
        Trade t{(uint64_t)i, 999, 100, 1, 1};
        pnl.onFill(t, true, 100.0);
    }
    REQUIRE(pnl.getFillRate() == Approx(0.3));
}

TEST_CASE("equity includes unrealized", "[pnl]") {
    PnLTracker pnl;
    pnl.startCapital = 100000;
    double eq = pnl.getEquity(100, 50);
    REQUIRE(eq == Approx(105000.0));
}

TEST_CASE("zero fills gives zero sharpe", "[pnl]") {
    PnLTracker pnl;
    pnl.pushEquity(100);
    pnl.pushEquity(100);
    pnl.pushEquity(100);
    REQUIRE(pnl.getSharpe() == 0.0);
}
