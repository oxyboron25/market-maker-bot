#include "matching_engine.hpp"
#include "spsc_queue.hpp"
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>

struct BenchResult {
    double totalMs;
    double ordersPerSec;
    size_t totalOrders;
    size_t totalTrades;
};

struct OrderRequest {
    Side side;
    OrderType type;
    int64_t price;
    uint64_t quantity;
};

static constexpr size_t QUEUE_CAP = 65536;

std::vector<OrderRequest> generateOrders(size_t n, uint64_t seed = 42) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> qtyDist(1, 20);
    std::uniform_int_distribution<int> typeDist(0, 9);
    std::vector<OrderRequest> orders;
    orders.reserve(n);
    int64_t mid = 100;
    for (size_t i = 0; i < n; ++i) {
        OrderRequest o;
        o.side = (rng() % 2 == 0) ? Side::BUY : Side::SELL;
        int r = typeDist(rng);
        if (r < 6) o.type = OrderType::LIMIT;
        else if (r < 8) o.type = OrderType::MARKET;
        else o.type = OrderType::IOC;
        mid += (rng() % 3) - 1;
        if (mid < 90) mid = 90;
        if (mid > 110) mid = 110;
        o.price = o.type == OrderType::MARKET ? 0 : mid + (rng() % 5) - 2;
        o.quantity = qtyDist(rng);
        orders.push_back(o);
    }
    return orders;
}

BenchResult benchSingleThreaded(size_t n, uint64_t seed = 42) {
    MatchingEngine engine;
    auto orders = generateOrders(n, seed);
    size_t totalTrades = 0;

    auto t0 = std::chrono::high_resolution_clock::now();
    for (auto& o : orders) {
        auto r = engine.submitOrder(o.side, o.type, o.price, o.quantity);
        totalTrades += r.trades.size();
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    return {ms, (n / ms) * 1000.0, n, totalTrades};
}

struct MTResult {
    BenchResult bench;
    double p50ns;
    double p99ns;
    double p999ns;
};

MTResult benchSPSC(size_t n, uint64_t seed = 42) {
    MatchingEngine engine;
    SPSCQueue<OrderRequest, QUEUE_CAP> queue;
    auto orders = generateOrders(n, seed);
    size_t totalTrades = 0;

    std::vector<double> latencies;
    latencies.reserve(n);

    auto t0 = std::chrono::high_resolution_clock::now();

    std::thread consumer([&]() {
        size_t consumed = 0;
        while (consumed < n) {
            auto item = queue.pop();
            if (item) {
                auto opStart = std::chrono::high_resolution_clock::now();
                auto r = engine.submitOrder(item->side, item->type, item->price, item->quantity);
                auto opEnd = std::chrono::high_resolution_clock::now();
                latencies.push_back(std::chrono::duration<double, std::nano>(opEnd - opStart).count());
                totalTrades += r.trades.size();
                ++consumed;
            } else {
                std::this_thread::yield();
            }
        }
    });

    std::thread producer([&]() {
        for (size_t i = 0; i < n; ++i) {
            while (!queue.push(orders[i])) {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::sort(latencies.begin(), latencies.end());
    double p50 = latencies[n * 50 / 100];
    double p99 = latencies[n * 99 / 100];
    double p999 = latencies[std::min(n - 1, n * 999 / 1000)];

    return {{ms, (n / ms) * 1000.0, n, totalTrades}, p50, p99, p999};
}

void printST(const std::string& label, const BenchResult& r) {
    std::cout << "\n  " << label << std::endl;
    std::cout << "    Orders: " << r.totalOrders << "  Trades: " << r.totalTrades << std::endl;
    std::cout << "    Time:   " << std::fixed << std::setprecision(1) << r.totalMs << " ms" << std::endl;
    std::cout << "    Thru:   " << std::fixed << std::setprecision(0) << r.ordersPerSec << " orders/sec" << std::endl;
}

void printMT(const std::string& label, const MTResult& r) {
    std::cout << "\n  " << label << std::endl;
    std::cout << "    Orders: " << r.bench.totalOrders << "  Trades: " << r.bench.totalTrades << std::endl;
    std::cout << "    Time:   " << std::fixed << std::setprecision(1) << r.bench.totalMs << " ms" << std::endl;
    std::cout << "    Thru:   " << std::fixed << std::setprecision(0) << r.bench.ordersPerSec << " orders/sec" << std::endl;
    std::cout << "    p50:    " << std::fixed << std::setprecision(0) << r.p50ns << " ns" << std::endl;
    std::cout << "    p99:    " << std::fixed << std::setprecision(0) << r.p99ns << " ns" << std::endl;
    std::cout << "    p99.9:  " << std::fixed << std::setprecision(0) << r.p999ns << " ns" << std::endl;
}

int main() {
    std::cout << "=== LOB Matching Engine Benchmark ===" << std::endl;

    std::cout << "\n[Single-Threaded Baseline]" << std::endl;
    printST("100K orders", benchSingleThreaded(100000));
    printST("1M orders",   benchSingleThreaded(1000000));

    std::cout << "\n[SPSC Queue: 1 producer thread -> 1 matcher thread]" << std::endl;
    printMT("500K orders", benchSPSC(500000));

    return 0;
}
