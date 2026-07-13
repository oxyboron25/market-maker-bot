#include <catch2/catch_test_macros.hpp>
#include "matching_engine.hpp"

TEST_CASE("basic buy limit rests on bid side", "[book]") {
    MatchingEngine e;
    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 10);

    REQUIRE(r.error == OrderErrorCode::NONE);
    REQUIRE(r.trades.empty());

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.bid.has_value());
    REQUIRE(tob.bid->price == 100);
    REQUIRE(tob.bid->quantity == 10);
    REQUIRE_FALSE(tob.ask.has_value());
}

TEST_CASE("basic sell limit rests on ask side", "[book]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 105, 8);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE_FALSE(tob.bid.has_value());
    REQUIRE(tob.ask.has_value());
    REQUIRE(tob.ask->price == 105);
    REQUIRE(tob.ask->quantity == 8);
}

TEST_CASE("best bid/ask with multiple levels", "[book]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 99, 5);
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 10);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 102, 7);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 105, 3);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.bid->price == 100);
    REQUIRE(tob.bid->quantity == 10);
    REQUIRE(tob.ask->price == 102);
    REQUIRE(tob.ask->quantity == 7);
}

TEST_CASE("depth snapshot top-N", "[book]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 98, 1);
    e.submitOrder(Side::BUY, OrderType::LIMIT, 99, 2);
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 3);

    auto snap = e.getOrderBook().getDepthSnapshot(2, Side::BUY);
    REQUIRE(snap.size() == 2);
    REQUIRE(snap[0].price == 100);
    REQUIRE(snap[1].price == 99);
}

TEST_CASE("total order count", "[book]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 10);
    e.submitOrder(Side::BUY, OrderType::LIMIT, 99, 5);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 105, 8);

    REQUIRE(e.getOrderBook().totalOrders() == 3);
}
