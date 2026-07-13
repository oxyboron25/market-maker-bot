#include <catch2/catch_test_macros.hpp>
#include "matching_engine.hpp"

TEST_CASE("reject qty 0", "[edge]") {
    MatchingEngine e;
    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 0);
    REQUIRE(r.error == OrderErrorCode::INVALID_QUANTITY);
    REQUIRE(r.trades.empty());
}

TEST_CASE("reject negative price on limit", "[edge]") {
    MatchingEngine e;
    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, -5, 10);
    REQUIRE(r.error == OrderErrorCode::INVALID_PRICE);
}

TEST_CASE("reject zero price on limit", "[edge]") {
    MatchingEngine e;
    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 0, 10);
    REQUIRE(r.error == OrderErrorCode::INVALID_PRICE);
}

TEST_CASE("market order accepts price=0", "[edge]") {
    MatchingEngine e;
    auto r = e.submitOrder(Side::BUY, OrderType::MARKET, 0, 10);
    REQUIRE(r.error == OrderErrorCode::NONE);
}

TEST_CASE("cancel nonexistent id", "[edge]") {
    MatchingEngine e;
    auto r = e.cancelOrder(42);
    REQUIRE_FALSE(r.success);
    REQUIRE(r.error == OrderErrorCode::ORDER_NOT_FOUND);
}

TEST_CASE("double cancel", "[edge]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 10);
    e.cancelOrder(1);
    auto r = e.cancelOrder(1);
    REQUIRE_FALSE(r.success);
}

TEST_CASE("fill sweeps multiple levels correctly", "[edge]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 3);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 101, 3);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 102, 3);

    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 102, 5);

    REQUIRE(r.trades.size() == 2);
    REQUIRE(r.trades[0].quantity == 3);
    REQUIRE(r.trades[1].quantity == 2);

    auto snap = e.getOrderBook().getDepthSnapshot(5, Side::SELL);
    REQUIRE(snap.size() == 2);
    REQUIRE(snap[0].price == 101);
    REQUIRE(snap[0].quantity == 1);
}

TEST_CASE("modify with qty 0 is rejected", "[edge]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 10);
    auto r = e.modifyOrder(1, 100, 0);
    REQUIRE(r.error == OrderErrorCode::INVALID_QUANTITY);
    REQUIRE(e.getOrderBook().totalOrders() == 1);
}

TEST_CASE("modify nonexistent order", "[edge]") {
    MatchingEngine e;
    auto r = e.modifyOrder(999, 100, 10);
    REQUIRE(r.error == OrderErrorCode::ORDER_NOT_FOUND);
}

TEST_CASE("fifo within same price level", "[edge]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 2);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 3);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);

    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 4);

    REQUIRE(r.trades.size() == 2);
    REQUIRE(r.trades[0].sellOrderId == 1);
    REQUIRE(r.trades[0].quantity == 2);
    REQUIRE(r.trades[1].sellOrderId == 2);
    REQUIRE(r.trades[1].quantity == 2);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.ask->quantity == 6);
}

TEST_CASE("empty level gets cleaned up", "[edge]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 5);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE_FALSE(tob.ask.has_value());
    REQUIRE_FALSE(tob.bid.has_value());
}

TEST_CASE("ioc buy below market does nothing", "[edge]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 10);

    auto r = e.submitOrder(Side::BUY, OrderType::IOC, 98, 5);

    REQUIRE(r.trades.empty());
    REQUIRE(e.getOrderBook().totalOrders() == 1);
}
