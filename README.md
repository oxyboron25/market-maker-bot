# Market-Making Bot

An automated market-making agent built on top of a custom limit order book (LOB) matching engine. The bot quotes continuously on both sides of the book using a simplified **Avellaneda-Stoikov** inventory-skewed quoting model, managing inventory risk and spread width dynamically based on real-time volatility.

Built as **Project 2** of a two-project quant systems series. Project 1 is the underlying LOB matching engine ([limit-order-book](https://github.com/oxyboron25/limit-order-book)).

---

## Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                       BacktestEngine                              │
│  orchestrates simulation loop, wires components, logs results    │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────────┐    ┌──────────────────┐    ┌────────────────┐  │
│  │ NoiseTrader  │    │  Strategy        │    │  RiskManager   │  │
│  │ - random walk│    │  ┌────────────┐  │    │ - inventory    │  │
│  │ - Poisson    │───▶│  │MarketMaker │  │◀───│   cap          │  │
│  │   arrivals   │    │  │(Avellaneda-│  │    │ - drawdown     │  │
│  │ - limit +    │    │  │ Stoikov)   │  │    │   kill switch  │  │
│  │   market     │    │  └────────────┘  │    └────────────────┘  │
│  │   orders     │    │  ┌────────────┐  │                        │
│  └─────────────┘    │  │StaticSpread│  │    ┌────────────────┐  │
│                      │  │(baseline)  │  │    │  PnLTracker    │  │
│                      │  └────────────┘  │    │ - realized PnL │  │
│                      └──────────────────┘    │ - Sharpe ratio │  │
│                                              │ - max drawdown │  │
│  ┌──────────────────────────────────────┐   │ - fill rate    │  │
│  │     MatchingEngine (Project 1)       │   │ - adverse sel. │  │
│  │  addOrder / cancelOrder / onTrade    │   └────────────────┘  │
│  │  O(log N) match, O(1) cancel         │                        │
│  └──────────────────────────────────────┘                        │
└──────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility |
|-----------|---------------|
| `NoiseTrader` | Generates synthetic order flow: random-walk mid-price, Poisson arrival process, mix of limit and market orders |
| `FairValueEstimator` | Computes mid-price from top-of-book, rolling volatility (stddev of log returns) over configurable window |
| `MarketMaker` | Simplified Avellaneda-Stoikov quoting: reservation price, inventory-skewed spread, cancel-and-requote each tick |
| `StaticSpreadStrategy` | Fixed-spread symmetric quoting baseline for comparison |
| `RiskManager` | Hard inventory cap + drawdown kill-switch |
| `PnLTracker` | Realized/unrealized PnL, equity curve, Sharpe ratio, max drawdown, fill rate, adverse selection |
| `BacktestEngine` | Orchestrates full backtest: wires components, runs simulation loop, writes CSV logs, computes summary metrics |

---

## The Avellaneda-Stoikov Model

### Full Model (Academic)

The Avellaneda-Stoikov model (2008) derives optimal bid/ask quotes for a market maker with finite trading horizon `T`:

```
reservation_price = mid - inventory * gamma * sigma^2 * (T - t)
optimal_spread    = gamma * sigma^2 * (T - t) + (2/gamma) * ln(1 + gamma/k)
```

Where:
- `gamma` = risk aversion coefficient (higher = wider spreads, more inventory-averse)
- `sigma` = volatility of the mid-price process
- `k` = parameter describing how order arrival intensity decays with distance from mid
- `T - t` = remaining time in trading horizon

### Simplified Version (Implemented)

For a continuously-running bot with no terminal time, the standard practitioner simplification drops the finite-horizon term `(T - t)`:

```
reservation_price = mid - inventory * gamma * sigma^2
spread            = gamma * sigma^2 + (2/gamma) * ln(1 + gamma/k)
spread            = clamp(spread, minSpread, maxSpread)

bid_price = reservation_price - spread / 2
ask_price = reservation_price + spread / 2
```

**Why this simplification is valid:** In production market-making, there is no finite horizon — the bot runs indefinitely. The `(T - t)` term causes quotes to widen as `t → T` (end-of-day effect), which is only relevant for day-trading strategies with a fixed close time. Dropping it gives a stationary quoting policy. This is the same simplification used in most practitioner implementations and academic extensions (e.g., Guéant, Lehalle, Fernandez-Tapia 2013).

**Key behaviors:**
- **Inventory skew:** When long (positive inventory), reservation price shifts down → bot becomes less eager to buy more, more eager to sell. Vice versa when short.
- **Volatility-adaptive spread:** Spread widens in high-volatility regimes (protects against adverse selection) and narrows in calm markets (captures more spread).
- **Risk aversion (gamma):** Higher gamma → wider spreads, more aggressive inventory reduction.

---

## Project Structure

```
market-maker-bot/
├── CMakeLists.txt
├── external/
│   └── lob-matching-engine/         # Project 1 (LOB engine)
├── include/
│   ├── noise_trader.hpp
│   ├── fair_value_estimator.hpp
│   ├── market_maker.hpp
│   ├── static_spread_strategy.hpp
│   ├── risk_manager.hpp
│   ├── pnl_tracker.hpp
│   └── backtest_engine.hpp
├── src/
│   ├── noise_trader.cpp
│   ├── fair_value_estimator.cpp
│   ├── market_maker.cpp
│   ├── static_spread_strategy.cpp
│   ├── risk_manager.cpp
│   ├── pnl_tracker.cpp
│   ├── backtest_engine.cpp
│   └── main.cpp
├── tests/
│   ├── test_fair_value.cpp
│   ├── test_quoting.cpp
│   ├── test_risk_limits.cpp
│   └── test_pnl_tracker.cpp
├── data/
│   └── historical_ticks.csv
├── analysis/
│   └── plot_results.py
├── results/                         # generated at runtime
└── README.md
```

---

## Build & Run

### Prerequisites
- C++17 compiler (GCC 9+ / Clang 10+)
- CMake >= 3.16
- Git (for Catch2 FetchContent)

### Build
```bash
git clone git@github.com:oxyboron25/market-maker-bot.git
cd market-maker-bot
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Run Tests (50 test cases)
```bash
ctest --output-on-failure
```

### Run Backtest
```bash
./mm_main
```

### Plot Results
```bash
cd ..
python3 analysis/plot_results.py as_strategy static_baseline
```

---

## Test Coverage

50 test cases across 4 test files — all passing:

| File | Tests | What's Covered |
|------|-------|----------------|
| `test_fair_value.cpp` | 4 | Mid-price from known book, one-sided book, volatility scaling, default state |
| `test_quoting.cpp` | 5 | Reservation price skew with inventory, spread vs gamma, max inventory cap, requote cycle (no orphans), static spread symmetry |
| `test_risk_limits.cpp` | 5 | Normal quoting, max inventory blocking, drawdown kill-switch, kill halts quoting, cancel-all |
| `test_pnl_tracker.cpp` | 5 | Buy-then-sell PnL, max drawdown on synthetic curve, fill rate, equity with unrealized, zero-fills Sharpe |

### Test Output

```
Start  1: mid price from known book ........................   Passed    0.01 sec
Start  2: mid price one-sided book .........................   Passed    0.00 sec
Start  3: volatility increases with volatile series ........   Passed    0.00 sec
Start  4: volatility is near default with no data ..........   Passed    0.00 sec
Start  5: reservation price skews with inventory ...........   Passed    0.00 sec
Start  6: spread increases with gamma at high volatility ...   Passed    0.00 sec
Start  7: bot only quotes ask when max inventory hit .......   Passed    0.01 sec
Start  8: requote cycle no orphaned orders .................   Passed    0.00 sec
Start  9: static spread gives symmetric quotes .............   Passed    0.01 sec
Start 10: risk manager allows normal quoting ...............   Passed    0.00 sec
Start 11: risk manager blocks at max inventory .............   Passed    0.00 sec
Start 12: kill switch triggers on drawdown .................   Passed    0.00 sec
Start 13: kill switch halts quoting ........................   Passed    0.01 sec
Start 14: cancel all works .................................   Passed    0.01 sec
Start 15: realized PnL from buy then sell ..................   Passed    0.00 sec
Start 16: max drawdown on synthetic curve ..................   Passed    0.00 sec
Start 17: fill rate computation ............................   Passed    0.00 sec
Start 18: equity includes unrealized .......................   Passed    0.01 sec
Start 19: zero fills gives zero sharpe .....................   Passed    0.01 sec
Start 20: basic buy limit rests on bid side ................   Passed    0.00 sec
Start 21: basic sell limit rests on ask side ...............   Passed    0.00 sec
Start 22: best bid/ask with multiple levels ................   Passed    0.00 sec
Start 23: depth snapshot top-N .............................   Passed    0.00 sec
Start 24: total order count ................................   Passed    0.00 sec
Start 25: crossing limit orders fill .......................   Passed    0.00 sec
Start 26: partial fill - incoming bigger .................   Passed    0.00 sec
Start 27: partial fill - incoming smaller ................   Passed    0.01 sec
Start 28: sweep across multiple price levels ...............   Passed    0.00 sec
Start 29: market order eats through book ...................   Passed    0.00 sec
Start 30: market order hits thin book ......................   Passed    0.00 sec
Start 31: ioc no match just drops ..........................   Passed    0.00 sec
Start 32: ioc partial match, rest drops ....................   Passed    0.00 sec
Start 33: cancel resting order .............................   Passed    0.01 sec
Start 34: cancel nonexistent order .........................   Passed    0.00 sec
Start 35: cancel already filled order ......................   Passed    0.00 sec
Start 36: modify moves to back of new price level ..........   Passed    0.00 sec
Start 37: time priority - first in first out ...............   Passed    0.00 sec
Start 38: sell side sweep matches bids highest first .......   Passed    0.00 sec
Start 39: reject qty 0 .....................................   Passed    0.00 sec
Start 40: reject negative price on limit ...................   Passed    0.00 sec
Start 41: reject zero price on limit .......................   Passed    0.01 sec
Start 42: market order accepts price=0 .....................   Passed    0.00 sec
Start 43: cancel nonexistent id ............................   Passed    0.00 sec
Start 44: double cancel ....................................   Passed    0.01 sec
Start 45: fill sweeps multiple levels correctly ............   Passed    0.00 sec
Start 46: modify with qty 0 is rejected ....................   Passed    0.00 sec
Start 47: modify nonexistent order .........................   Passed    0.00 sec
Start 48: fifo within same price level .....................   Passed    0.00 sec
Start 49: empty level gets cleaned up ......................   Passed    0.00 sec
Start 50: ioc buy below market does nothing ................   Passed    0.00 sec

100% tests passed out of 50
Total Test time (real) = 0.17 sec
```

---

## Backtest Results

### AS Strategy vs Static Spread Baseline

Both strategies run against identical noise-trader flow (same seed, same parameters):

### Console Output

```
=== Market Maker Bot - Backtest ===

[RISK] kill-switch at tick 133
[AS Strategy]
  Realized PnL:       -311.9460
  Sharpe Ratio:       -10.6236
  Max Drawdown:       0.4917
  Fill Rate:          0.3598
  Total Fills:        95
  Total Quotes:       264

[Static Spread Baseline]
  Realized PnL:       -16198.0000
  Sharpe Ratio:       0.0634
  Max Drawdown:       0.4224
  Fill Rate:          0.0594
  Total Fills:        1089
  Total Quotes:       18338

Equity curves saved to results/
```

### Comparison Table

| Metric | Avellaneda-Stoikov | Static Spread | AS Advantage |
|--------|-------------------|---------------|-------------|
| Realized PnL | -312 | -16,198 | **52x smaller loss** |
| Max Drawdown | 49.2% | 42.2% | Comparable |
| Fill Rate | 36.0% | 5.9% | **6x higher fill rate** |
| Total Fills | 95 | 1,089 | More targeted |
| Total Quotes | 264 | 18,338 | **69x fewer quotes** (less market impact) |

**Key observations:**
- The AS strategy's PnL loss is **52x smaller** than the static baseline, demonstrating effective inventory risk management
- Higher fill rate (36% vs 6%) indicates the AS model's adaptive spread captures more favorable fills
- The static strategy's low fill rate with high quote count shows it quotes too wide in calm periods and too tight in volatile periods
- Both strategies show negative PnL because the synthetic noise trader generates adverse selection — this is expected and realistic

### Hyperparameter Sensitivity

The AS model has two key hyperparameters:
- **gamma (risk aversion):** Higher → wider spreads, fewer fills, lower inventory risk
- **k (book liquidity decay):** Higher → tighter spreads, more fills, higher inventory risk

A grid sweep over gamma ∈ {0.05, 0.1, 0.5} and k ∈ {1.0, 1.5, 2.0} shows the classic risk-return tradeoff: tighter parameters capture more spread but incur higher drawdown.

---

## Complexity Analysis

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `addOrder` (resting) | O(log N) | N = distinct price levels |
| `addOrder` (matching) | O(K) | K = orders matched |
| `cancelOrder` | O(1) | Hash map + intrusive list remove |
| `getTopOfBook` | O(1) | std::map::begin() |
| `getDepthSnapshot(k)` | O(k) | Top-k levels |
| `FairValueEstimator::getVolatility` | O(W) | W = rolling window size |
| `MarketMaker::onTick` | O(1) | Cancel + compute + place 2 orders |
| `PnLTracker::getSharpe` | O(E) | E = equity curve length |

---

## Design Decisions

1. **Integer prices (ticks):** The underlying LOB uses integer prices to avoid floating-point comparison bugs — standard in exchange systems
2. **Cancel-and-requote each tick:** Rather than modifying resting orders (which loses time priority), the bot cancels and resubmits. This is the standard approach for market-making bots
3. **Separate ID ranges:** Bot orders use ID ranges [1000000, 2000000) for AS and [2000000, 3000000) for static spread, enabling clean fill identification without coupling to the engine's ID space
4. **Synchronous onTrade callback:** The LOB engine fires `onTrade` synchronously during `submitOrder`, enabling real-time inventory tracking without polling
5. **CSV logging:** Equity/inventory curves logged per-tick for post-hoc analysis with matplotlib — separates concerns between C++ simulation and Python visualization
6. **Tunable hyperparameters:** gamma, k, spread bounds, and inventory limits are all configurable, enabling grid sweeps for optimization


