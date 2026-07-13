#include <catch2/catch_test_macros.hpp>
#include "matching_engine.hpp"

TEST_CASE("crossing limit orders fill", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 10);
    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 5);

    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].price == 100);
    REQUIRE(r.trades[0].quantity == 5);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.ask->quantity == 5);
    REQUIRE_FALSE(tob.bid.has_value());
}

TEST_CASE("partial fill - incoming bigger", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);
    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 10);

    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].quantity == 5);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE_FALSE(tob.ask.has_value());
    REQUIRE(tob.bid->quantity == 5);
}

TEST_CASE("partial fill - incoming smaller", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 10);
    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 3);

    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].quantity == 3);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.ask->quantity == 7);
}

TEST_CASE("sweep across multiple price levels", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 101, 5);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 102, 5);

    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 102, 12);

    REQUIRE(r.trades.size() == 3);
    REQUIRE(r.trades[0].price == 100);
    REQUIRE(r.trades[1].price == 101);
    REQUIRE(r.trades[2].price == 102);
    REQUIRE(r.trades[2].quantity == 2);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.ask->price == 102);
    REQUIRE(tob.ask->quantity == 3);
}

TEST_CASE("market order eats through book", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 10);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 101, 10);

    auto r = e.submitOrder(Side::BUY, OrderType::MARKET, 0, 15);

    REQUIRE(r.trades.size() == 2);
    REQUIRE(r.trades[0].price == 100);
    REQUIRE(r.trades[0].quantity == 10);
    REQUIRE(r.trades[1].price == 101);
    REQUIRE(r.trades[1].quantity == 5);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.ask->quantity == 5);
}

TEST_CASE("market order hits thin book", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);

    auto r = e.submitOrder(Side::BUY, OrderType::MARKET, 0, 20);

    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].quantity == 5);
    REQUIRE_FALSE(e.getOrderBook().getTopOfBook().ask.has_value());
}

TEST_CASE("ioc no match just drops", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 105, 10);

    auto r = e.submitOrder(Side::BUY, OrderType::IOC, 100, 5);

    REQUIRE(r.trades.empty());
    REQUIRE(e.getOrderBook().totalOrders() == 1);
}

TEST_CASE("ioc partial match, rest drops", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 101, 10);

    auto r = e.submitOrder(Side::BUY, OrderType::IOC, 100, 8);

    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].price == 100);
    REQUIRE(r.trades[0].quantity == 5);
    REQUIRE_FALSE(e.getOrderBook().getTopOfBook().bid.has_value());
}

TEST_CASE("cancel resting order", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 10);
    auto cr = e.cancelOrder(1);

    REQUIRE(cr.success);
    REQUIRE_FALSE(e.getOrderBook().getTopOfBook().bid.has_value());
    REQUIRE(e.getOrderBook().totalOrders() == 0);
}

TEST_CASE("cancel nonexistent order", "[match]") {
    MatchingEngine e;
    auto r = e.cancelOrder(999);
    REQUIRE_FALSE(r.success);
    REQUIRE(r.error == OrderErrorCode::ORDER_NOT_FOUND);
}

TEST_CASE("cancel already filled order", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 10);

    auto cr = e.cancelOrder(1);
    REQUIRE_FALSE(cr.success);
    REQUIRE(cr.error == OrderErrorCode::ORDER_NOT_FOUND);
}

TEST_CASE("modify moves to back of new price level", "[match]") {
    MatchingEngine e;
    // order 1 at ask 100, order 2 at ask 101
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 101, 5);

    // move order 1 to 101, it goes behind order 2
    e.modifyOrder(1, 101, 5);

    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 101, 3);
    // should fill order 2 first (time priority at 101)
    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].sellOrderId == 2);
}

TEST_CASE("time priority - first in first out", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);
    e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 5);

    auto r = e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 3);

    REQUIRE(r.trades.size() == 1);
    REQUIRE(r.trades[0].sellOrderId == 1);

    auto tob = e.getOrderBook().getTopOfBook();
    REQUIRE(tob.ask->quantity == 7);
}

TEST_CASE("sell side sweep matches bids highest first", "[match]") {
    MatchingEngine e;
    e.submitOrder(Side::BUY, OrderType::LIMIT, 102, 5);
    e.submitOrder(Side::BUY, OrderType::LIMIT, 101, 5);
    e.submitOrder(Side::BUY, OrderType::LIMIT, 100, 5);

    auto r = e.submitOrder(Side::SELL, OrderType::LIMIT, 100, 12);

    REQUIRE(r.trades.size() == 3);
    REQUIRE(r.trades[0].price == 102);
    REQUIRE(r.trades[1].price == 101);
    REQUIRE(r.trades[2].price == 100);
}
