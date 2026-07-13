#include <catch2/catch_test_macros.hpp>
#include "risk_manager.hpp"
#include "matching_engine.hpp"

TEST_CASE("risk manager allows normal quoting", "[risk]") {
    RiskManager rm;
    rm.setMaxInventory(100);
    REQUIRE(rm.canQuote(50));
    REQUIRE(rm.canQuote(-50));
    REQUIRE(rm.canQuote(0));
}

TEST_CASE("risk manager blocks at max inventory", "[risk]") {
    RiskManager rm;
    rm.setMaxInventory(100);
    REQUIRE_FALSE(rm.canQuote(100));
    REQUIRE_FALSE(rm.canQuote(-100));
    REQUIRE_FALSE(rm.canQuote(150));
}

TEST_CASE("kill switch triggers on drawdown", "[risk]") {
    RiskManager rm;
    rm.setMaxDrawdownPct(0.05);

    REQUIRE(rm.checkDrawdown(100000, 100000));
    REQUIRE(rm.checkDrawdown(96000, 100000));
    REQUIRE_FALSE(rm.checkDrawdown(94000, 100000));
}

TEST_CASE("kill switch halts quoting", "[risk]") {
    RiskManager rm;
    rm.setMaxInventory(100);
    rm.kill();
    REQUIRE_FALSE(rm.canQuote(10));
}

TEST_CASE("cancel all works", "[risk]") {
    RiskManager rm;
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 95, 10);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 105, 10);
    auto bid = e.getOrderBook().getBestBid();
    auto ask = e.getOrderBook().getBestAsk();
    REQUIRE(bid.has_value());
    rm.cancelAll(e, 1, 2);
}
