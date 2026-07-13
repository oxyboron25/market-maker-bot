# Limit Order Book Matching Engine

Production-style limit order book (LOB) and matching engine in C++17, supporting standard order types with price-time priority matching. Built as a systems/DS project demonstrating low-latency design patterns.

## Architecture

```
┌──────────────────────────────────────────────────────┐
│                 MatchingEngine                        │
│   submitOrder / cancelOrder / modifyOrder             │
│         │                                            │
│   ┌─────▼──────┐                                     │
│   │  OrderBook  │                                     │
│   └─────┬──────┘                                     │
│         │                                            │
│    ┌────┴──────────┐    ┌──────────────┐            │
│    │     bids_     │    │    asks_     │            │
│    │ map<int64_t,  │    │ map<int64_t, │            │
│    │  IntrusiveList,│   │  IntrusiveList,│           │
│    │  std::greater> │   │  std::less>  │            │
│    │  (highest      │   │  (lowest     │            │
│    │   first)       │   │   first)     │            │
│    └────┬──────────┘    └──────┬───────┘            │
│         │                      │                     │
│    ┌────┴──────────────────────┴──────┐             │
│    │          orderIndex_             │             │
│    │ unordered_map<uint64_t, Location>│             │
│    │  Location = {price, side, ptr}   │             │
│    │  O(1) cancel via direct pointer  │             │
│    └──────────────────────────────────┘             │
│                                                      │
│    ┌──────────────────────────────┐                 │
│    │    IntrusiveList (per level) │                 │
│    │  Order* head <-> tail        │                 │
│    │  prev/next embedded in Order │                 │
│    │  O(1) push / pop / remove    │                 │
│    └──────────────────────────────┘                 │
│                                                      │
│    ┌──────────────────────────────┐                 │
│    │    SPSCQueue<T, N>           │                 │
│    │  Lock-free single-producer-  │                 │
│    │  single-consumer ring buffer │                 │
│    │  Cache-line padded head/tail │                 │
│    └──────────────────────────────┘                 │
└──────────────────────────────────────────────────────┘
```

## Order Types

| Type | Behavior |
|------|----------|
| **LIMIT** | Rests in book if not fully matchable; matches at limit price or better |
| **MARKET** | Matches immediately at any price; never rests; unfilled remainder discarded |
| **IOC** | Matches at limit price or better; unfilled remainder discarded |

## Matching Algorithm

Price-time priority:
1. New order matches against opposite side at best available price(s)
2. Within a price level, orders fill FIFO (arrival order)
3. Orders that cross the spread sweep multiple price levels
4. Unfilled LIMIT remainder rests in book; MARKET/IOC remainder is discarded

**Modify** is implemented as cancel + re-insert (order loses time priority — this is real exchange behavior, not a bug).

## Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `addOrder` (resting) | O(log N) | N = distinct price levels |
| `addOrder` (matching) | O(K) | K = orders matched |
| `cancelOrder` | O(1) | Hash map + intrusive list remove |
| `modifyOrder` | O(log N) | Cancel + re-insert |
| `getBestBid/Ask` | O(1) | `std::map::begin()` |
| `getDepthSnapshot(k)` | O(k) | Top-k levels |

## Build & Run

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run demo with sample CSV
./lob_main ../data/sample_orders.csv

# Run tests (31 test cases)
ctest --output-on-failure

# Run benchmark
./lob_bench
```

## Project Structure

```
include/
  order.hpp              # Order, Trade structs, Side/OrderType enums, OrderLocation
  price_level.hpp        # IntrusiveList (doubly-linked, zero heap alloc per node)
  order_book.hpp         # OrderBook with bids/asks maps + orderIndex
  matching_engine.hpp    # Facade: auto-ID assignment, clean API
  memory_pool.hpp        # Template freelist allocator for Order objects
  spsc_queue.hpp         # Lock-free SPSC ring buffer queue
src/
  order_book.cpp         # Core matching logic
  matching_engine.cpp    # Engine facade
  main.cpp               # CSV demo + depth snapshot + trade log
tests/
  test_order_book.cpp    # Book structure and query tests
  test_matching.cpp      # Matching algorithm tests (11 explicit test cases)
  test_edge_cases.cpp    # Validation + edge case tests
bench/
  benchmark_throughput.cpp  # Single-threaded + SPSC multi-threaded benchmark
data/
  sample_orders.csv      # 8-order hand-verifiable test feed
```

## Benchmark Results

**Single-threaded (direct submission, -O2):**

| Metric | 100K Orders | 1M Orders |
|--------|------------|-----------|
| Throughput | 7,098,549 orders/sec | 7,930,368 orders/sec |
| Wall time | 14.1 ms | 126.1 ms |

**SPSC queue (1 producer thread -> 1 matcher thread, per-op latency measured):**

| Metric | 500K Orders |
|--------|------------|
| Throughput | 250,318 orders/sec |
| Latency p50 | 1,886 ns |
| Latency p99 | 2,933 ns |
| Latency p99.9 | 14,667 ns |
| Wall time | 1,997.5 ms |

Tested on GCC 15.2.0 / Linux x86_64. SPSC throughput is lower because per-operation latency measurement adds timing overhead to every call.

### Optimization Notes

| Optimization | Impact |
|-------------|--------|
| **Intrusive doubly-linked list** | Eliminates per-node heap allocation for order queues. Each Order contains prev/next pointers directly, so push_back / pop_front / remove are pure pointer manipulation with zero allocator involvement. |
| **Memory pool (freelist)** | Pre-allocated fixed-size pool with O(1) allocate/deallocate. Avoids `new`/`delete` overhead in hot path. Ready for integration. |
| **noexcept hot path** | IntrusiveList methods and match functions marked `noexcept`. Eliminates exception handling overhead in the tightest loops. |
| **Lock-free SPSC queue** | Bounded ring buffer with cache-line-padded atomic head/tail indices. Enables concurrent order submission from client threads while keeping the book single-threaded — standard HFT architecture pattern. |

## Design Decisions

- **Integer prices (ticks)**: Avoids floating-point precision bugs in price comparison
- **Intrusive linked list**: Orders contain prev/next pointers directly — zero per-node heap allocation
- **O(1) cancel**: `orderIndex_` maps order ID to `{price, side, Order*}`, enabling direct pointer-based removal without scanning
- **Modify = cancel + reinsert**: Order loses time priority; this is intentional exchange-standard behavior
- **Self-trade policy**: Allowed (no client IDs in current implementation)
- **SPSC single-threaded book**: Matching runs on a single thread; multiple producer threads submit via lock-free queue — this is how real HFT systems avoid lock contention on book state

## Test Coverage

31 test cases covering:
- Basic order resting (buy/sell)
- Price-time priority preservation
- Partial fills (incoming > resting, incoming < resting)
- Multi-level sweep (order matches across multiple price levels)
- Market order with sufficient/insufficient liquidity
- IOC order with no match / partial match
- Cancel valid order, cancel non-existent, cancel already-filled
- Modify order (loses time priority — explicitly tested)
- Reject quantity=0, negative price, zero price
- Price level cleanup when empty
- Depth snapshot correctness
